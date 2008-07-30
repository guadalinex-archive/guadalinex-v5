
#include "siteconfig.h"

#include "ptools.h"
#include <deque>
#include <vector>

class CTraffic
{
public:
	CTraffic(): m_dwTick_t0(0), m_bTickHistoricoValido(false) { } 
		
	static void SetDbFile(const std::tstring &dbFile) { m_db=dbFile; }
	static void SetVer(int nVerMj, int nVerMn, int nVerUp,
		const std::tstring &verName)
	{
		m_nVerMj=nVerMj;
		m_nVerMn=nVerMn;
		m_nVerUp=nVerUp;
		m_verName=verName;
	}
	
	void SetType(const std::tstring &tag) { m_typeTag=tag; }
	
	struct SHistorico
	{
		int mes, anno;
		UINT64 up, down;
	};
	static std::vector<SHistorico> GetConsumoHistorico(const std::tstring &tag=_T(""));
	SHistorico GetConsumoHistoricoPendiente();

	void ProcesaHistorico(DWORD up, DWORD down, DWORD tick, bool bFinCon);	
private:
	static std::tstring m_db, m_verName;
	static int m_nVerMj, m_nVerMn, m_nVerUp;
	
	std::tstring m_typeTag;
	
	DWORD m_dwTick_t0;
	DWORD m_dwTick_historico;
	bool m_bTickHistoricoValido;
	struct aPoint {
		DWORD tick;
		DWORD up;
		DWORD down;
		aPoint(DWORD t, DWORD u,  DWORD d):
		tick(t),up(u),down(d){}
	} ;
	std::deque<aPoint>	 m_pointsHist;	
};
