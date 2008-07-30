
#include "Traffic.h"
#include <sqlite3.h>
#include <sstream>
#include <stdlib.h>

std::tstring CTraffic::m_db, CTraffic::m_verName;
int CTraffic::m_nVerMj, CTraffic::m_nVerMn, CTraffic::m_nVerUp;

void CTraffic::ProcesaHistorico(DWORD u, DWORD d, DWORD tick, bool bFinCon)
{	
	// a�adir los datos a la cola:
	if (tick!=0 && (u!=0 || d!=0))
		m_pointsHist.push_back(aPoint(tick, u, d));

	if (m_dwTick_t0==0)
		m_dwTick_t0 = tick;
	
	if (m_pointsHist.empty())
		return;

	DWORD dwTick;
	dwTick=!m_bTickHistoricoValido? m_dwTick_t0:m_dwTick_historico;

	DWORD up(0), down(0);

	const unsigned grano=60000*60; //*60;

	// acumular datos en tiempo 'grano':
	//
	if (m_pointsHist.back().tick-dwTick<grano && !bFinCon)
	{
		if (m_pointsHist.size()>1)
		{
			for (std::deque<aPoint>::iterator it=m_pointsHist.begin(); it!=m_pointsHist.end(); it++)
			{
				up+=it->up;
				down+=it->down;
			}
			dwTick=m_pointsHist.back().tick;
			m_pointsHist.clear();
			m_pointsHist.push_back(aPoint(dwTick,up,down));
		}
		return;
	}

	// calcular lo que hay listo para almacenar:
	//
	DWORD maxtick=0;
	bool bMaxTick=false;
	for (std::deque<aPoint>::reverse_iterator it=m_pointsHist.rbegin()+(!bFinCon? 1:0); it!=m_pointsHist.rend(); it++)
	{
		DWORD t=it->tick;
		up+=it->up;
		down+=it->down;
		if (!bMaxTick || int(t-maxtick)>0)
		{
			maxtick=t;
			bMaxTick=true;
		}
	}	

#define NO_TRAFFIC

	// almacenar :
	//
	// if (up!=0 || down!=0)
	//for (int ddd=0; ddd<10; ddd++)
	{
		sqlite3 *db;
		std::string dbfile = toUTF8(m_db);
		if (sqlite3_open(dbfile.c_str(), &db)==SQLITE_OK)
		{
			bool ok=true;

			bool bBeginTrans=sqlite3_exec(db, "BEGIN EXCLUSIVE TRANSACTION;", NULL,NULL,NULL)==SQLITE_OK;
			ok=ok&&bBeginTrans;

			if (ok)
			{
				std::stringstream ss;

#ifndef NO_TRAFFIC
				bool bTrafficCreate=ok&&sqlite3_exec(
						db, 
						"CREATE TABLE IF NOT EXISTS traffic (time DATETIME NOT NULL, up NUMERIC, down NUMERIC, millis NUMERIC, tag TEXT);",
						NULL,NULL,NULL)==SQLITE_OK;
				ok=ok&&bTrafficCreate;
#endif
				bool bMonthlyCreate=ok&&sqlite3_exec(
						db, 
						"CREATE TABLE IF NOT EXISTS monthly (year NUMERIC, month NUMERIC, up NUMERIC, down NUMERIC, millis NUMERIC, tag TEXT);",
						NULL,NULL,NULL)==SQLITE_OK;
				ok=bMonthlyCreate;

				bool bVersionCreate=sqlite3_exec(
						db, 
						"CREATE TABLE IF NOT EXISTS version (name TEXT UNIQUE, major NUMERIC, minor NUMERIC, updt NUMERIC);",
						NULL,NULL,NULL)==SQLITE_OK;
				ok=ok&&bVersionCreate;
				if (bVersionCreate)
				{
					ss.str("");
					ss<<"INSERT OR REPLACE INTO version(name,major,minor,updt) VALUES("
					  <<"'"<<toNarrowString(m_verName)<<"', "
					  <<m_nVerMj<<", "
					  <<m_nVerMn<<", "
					  <<m_nVerUp<<");";
					bool bVersionInsert=sqlite3_exec(db, ss.str().c_str(), NULL,NULL,NULL)==SQLITE_OK;
					ok=bVersionInsert;
				}

				DWORD time=maxtick-(!m_bTickHistoricoValido? m_dwTick_t0:m_dwTick_historico);
				if (time==0)
					time=1000;

#ifndef NO_TRAFFIC
				ss.str("");
				ss<<"INSERT INTO traffic(time, up, down, millis, tag) VALUES("
				  <<"date('now'), "
				  <<(int)up<<", "
				  <<(int)down<<", "
				  <<(int)time<<", ";
				ss<<"'"<<toNarrowString(m_typeTag)<<"'";
				ss<<");";

				bool bInsertTraffic=sqlite3_exec(db, ss.str().c_str(), NULL,NULL,NULL)==SQLITE_OK;
				ok=ok&&bInsertTraffic;
#endif

				sqlite3_stmt *stmt;

				ss.str("");
				//if (0) // ddd==0)
					ss<<"SELECT strftime(\"%m\", date('now', '-17 days')) as month, strftime(\"%Y\", date('now', '-17 days')) as year;";
				//else
				//{
				//	int days=abs(rand()*365*4/RAND_MAX);
				//	ss<<"SELECT strftime(\"%m\", date('now', '-17 days', ";
				//	ss<<"'-"<<days<<" days'";
				//	ss<<")) as month, strftime(\"%Y\", date('now', '-17 days', ";
				//	ss<<"'-"<<days<<" days'";
				//	ss<<")) as year;";
				//}
				bool bNow=sqlite3_prepare(db, ss.str().c_str(), -1, &stmt, NULL)==SQLITE_OK &&
					sqlite3_step(stmt)==SQLITE_ROW;
				ok=ok&&bNow;
				int month=0, year=0;
				if (bNow)
				{
					const char *m="";
					const char *y="";
					m=(const char *)sqlite3_column_text(stmt, 0);
					y=(const char *)sqlite3_column_text(stmt, 1);
					month=atoi(m);
					year=atoi(y);
					ok=sqlite3_finalize(stmt)==SQLITE_OK;
				}

				ss.str("");
				ss<<"SELECT * FROM monthly WHERE month="<<month<<" AND year="<<year;
				if (m_typeTag!=_T(""))
					ss<<" AND tag='"<<toNarrowString(m_typeTag)<<"'";
				ss<<";";
				bool bSelect=sqlite3_prepare(db, ss.str().c_str(), -1, &stmt, NULL)==SQLITE_OK;
				bool bSelectRow=bSelect && sqlite3_step(stmt)==SQLITE_ROW;
				if (bSelect)
					sqlite3_finalize(stmt);
				ok=ok&&bSelect;
				if (bSelectRow)
				{
					ss.str("");
					ss<<"UPDATE monthly SET up=up+"<<up<<", down=down+"<<down<<", millis=millis+"<<time<<" ";
					ss<<"WHERE month="<<month<<" AND year="<<year;
					if (m_typeTag!=_T(""))
						ss<<" AND tag='"<<toNarrowString(m_typeTag)<<"';";
	
					bool bUpdate=sqlite3_exec(db, ss.str().c_str(), NULL,NULL,NULL)==SQLITE_OK;
					ok=bUpdate;						
				}
				else
				{
					ss.str("");
					ss<<"INSERT INTO monthly(month, year, up, down, millis, tag) VALUES("
					  <<(int)month<<", "
					  <<(int)year<<", "
					  <<(int)up<<", "
					  <<(int)down<<", "
					  <<(int)time<<", ";
					ss<<"'"<<toNarrowString(m_typeTag)<<"'";
					ss<<");";
					
					bool bInsert=sqlite3_exec(db, ss.str().c_str(), NULL,NULL,NULL)==SQLITE_OK;
					ok=ok&&bInsert;
				}
				
				if (ok)
				{
					bool bCommit=ok&&sqlite3_exec(db, "COMMIT TRANSACTION;", NULL,NULL,NULL)==SQLITE_OK;
					ok=bCommit;

					if (bCommit)
					{
						// borrar datos viejos, y guardar el tick de ultimos datos almacenados:
						m_dwTick_historico=maxtick;
						m_bTickHistoricoValido=true;
						while (m_pointsHist.size()>0 && (int(m_pointsHist.begin()->tick-m_dwTick_historico)<=0 || bFinCon))
							m_pointsHist.erase(m_pointsHist.begin());
					}
				}
				else
					sqlite3_exec(db, "ROLLBACK TRANSACTION;", NULL,NULL,NULL);
			}
			sqlite3_close(db);
		}
	}

#undef NO_TRAFFIC



	//if (bFinCon)
	//{
	//	sqlite3 *db;
	//	std::string dbfile = toUTF8(std::tstring(theApp.GetPrivateFolder())
	//		.append(_T("\\")).append(BDHISTORICO_FILENAME));
	//	if (sqlite3_open(dbfile.c_str(), &db)==SQLITE_OK)
	//	{
	//		std::stringstream ss;
	//		ss.str("");
	//		ss<<"SELECT sum(t.millis), sum(t.up), sum(t.down) from TRAFFIC t";
	//		sqlite3_stmt *stmt; 
	//		if (sqlite3_prepare(db, ss.str().c_str(), -1, &stmt, NULL)==SQLITE_OK)
	//		{
	//			while (sqlite3_step(stmt)==SQLITE_ROW)
	//			{
	//				const unsigned char *sum=sqlite3_column_text(stmt, 0);
	//				const unsigned char *up=sqlite3_column_text(stmt, 1);
	//				const unsigned char *down=sqlite3_column_text(stmt, 2);
	//				TRACE("=========================\n");
	//				TRACE("FIN CON, DURATION: %d:%02d, up(%s), down(%s)\n", atoi((const char *)sum)/1000/60, atoi((const char *)sum)/1000%60, up, down);
	//				TRACE("=========================\n");
	//			}
	//			sqlite3_finalize(stmt);
	//		}
	//		sqlite3_close(db);
	//	}
	//}





}

CTraffic::SHistorico CTraffic::GetConsumoHistoricoPendiente()
{
	SHistorico h;
	h.anno=0;
	h.mes=0;
	h.up=0;
	h.down=0;
	sqlite3 *db;
	std::string dbfile = toUTF8(m_db);
	if (sqlite3_open(dbfile.c_str(), &db)==SQLITE_OK)
	{
		std::stringstream ss;
		ss<<"SELECT strftime(\"%m\", date('now', '-17 days')) as month, strftime(\"%Y\", date('now', '-17 days')) as year;";
		sqlite3_stmt *stmt; 
		if (sqlite3_prepare(db, ss.str().c_str(), -1, &stmt, NULL)==SQLITE_OK)
		{
			while (sqlite3_step(stmt)==SQLITE_ROW)
			{
				const unsigned char *month=sqlite3_column_text(stmt, 0);
				const unsigned char *year=sqlite3_column_text(stmt, 1);
				h.anno=atoi((const char *)year);
				h.mes=atoi((const char *)month);
				h.up=0;
				h.down=0;
				std::deque<aPoint>::iterator it;
				for (it=m_pointsHist.begin(); it!=m_pointsHist.end(); it++)
				{
					h.up+=it->up;
					h.down+=it->down;
				}
			}
			sqlite3_finalize(stmt);
		}
		sqlite3_close(db);
	}
	return h;
}

std::vector<CTraffic::SHistorico> CTraffic::GetConsumoHistorico(const std::tstring &tag)
{
	std::vector<SHistorico> hist;

	sqlite3 *db;
	std::string dbfile = toUTF8(m_db);
	if (sqlite3_open(dbfile.c_str(), &db)==SQLITE_OK)
	{
		std::stringstream ss;
		ss<<"SELECT up, down, month, year FROM monthly WHERE tag='"<<toNarrowString(tag)<<"' ORDER BY year, month;";
#if 0
		ss<<"SELECT sum(t.up), sum(t.down), strftime(\"%m\", date(time, '-17 days')) as month, strftime(\"%Y\", date(time, '-17 days')) as year FROM traffic t ";
		if (tag!=_T(""))
			ss<<"WHERE t.tag='"<<toNarrowString(tag)<<"' AND t.time>=date('now', '-2 years')";
		ss<<"GROUP BY strftime(\"%m\", date(t.time, '-17 days')), strftime(\"%Y\", date(time, '-17 days')) ORDER BY year, month ";
#endif
		sqlite3_stmt *stmt; 
		if (sqlite3_prepare(db, ss.str().c_str(), -1, &stmt, NULL)==SQLITE_OK)
		{
			while (sqlite3_step(stmt)==SQLITE_ROW)
			{
				const unsigned char *up=sqlite3_column_text(stmt, 0);
				const unsigned char *down=sqlite3_column_text(stmt, 1);
				const unsigned char *month=sqlite3_column_text(stmt, 2);
				const unsigned char *year=sqlite3_column_text(stmt, 3);
				SHistorico h;
#ifdef _WIN32
				h.up=_atoi64((const char *)up);
				h.down=_atoi64((const char *)down);
#else
				h.up=atoll((const char *)up);
				h.down=atoll((const char *)down);
#endif
				h.mes=atoi((const char *)month);
				h.anno=atoi((const char *)year);
				hist.push_back(h);
			}
			sqlite3_finalize(stmt);
		}
		sqlite3_close(db);
	}
	return hist;
}
