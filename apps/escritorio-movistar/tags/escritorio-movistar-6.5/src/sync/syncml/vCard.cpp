
#include "stdafx.h"
#include "vCard.h"
#include <sstream>
#include "ptools.h"

void CVCard::fromString(const std::tstring &data)
{
	details.clear();

	//std::tstring data2(data);
	//std::tstring::size_type pos;
	//pos = data2.find(_T("<![CDATA["));
	//if (pos!=std::tstring::npos)
	//{
	//	std::tstring::size_type pos2;
	//	pos2 = data2.find(_T("]]>"));
	//	if (pos2!=std::tstring::npos && pos<pos2)
	//	{
	//		const int taglen=strlen("<![CDATA[");
	//		data2 = data.substr(pos+taglen, pos2-pos+taglen);
	//	}
	//	else
	//		return;
	//}

	std::tstringstream ss;
	ss.str(data); // 2);

	std::tstring name, value;
	bool getName=true;
	const std::string encquote = ";ENCODING=QUOTED-PRINTABLE:";
	unsigned int encqpos=0;
	bool encquoted = false;
	bool lasteq = false;
	while (!ss.eof())
	{
		tchar c;
		ss.read(&c, 1);
		// ss>>c;
		if (!encquoted)
		{
			if (c==encquote[encqpos] || c==encquote[(encqpos=0)])
			{
				if (++encqpos>=encquote.length())
					encquoted = true;
			}
		}
		if (c==_T('\n') || c==_T('\r'))
		{
			if (!encquoted || !lasteq)
			{
				if (name!=_T(""))
				{
					details.push_back(SDetail());
					details.back().name = name;
					details.back().value = value;
				}
				name=value=_T("");
				getName=true;
			}
			else if (encquoted && lasteq)
			{
				if (value.length()>0 && value[value.length()-1]=='=')
					value.erase(value.end()-1);
			}
			continue;
		}
		else if (getName && c==_T(':'))
		{
			getName = false;
		}
		else
		{
			lasteq  = c=='=';
			(getName? name:value) += c;
		}
	}
	normalize();
}

std::tstring CVCard::toString()
{
	std::tstringstream ss;
	normalize();
	for (int d=0; d<(int)details.size(); d++)
		ss << details[d].name << _T(':') << details[d].value << std::endl;
	return ss.str();
}

void CVCard::normalize()
{
	int n;
	bool foundBegin = false;
	for (n=0; n<(int)details.size(); n++)
	{
		if (details[n].name == _T("BEGIN") &&
			details[n].value == _T("VCARD"))
		{
			foundBegin = true;
			if (n != 0)
			{
				SDetail detail(details[n]);
				details.erase(details.begin()+n);
				details.insert(details.begin(), detail);
				break;
			}
		}
	}
	if (!foundBegin)
	{
		SDetail detail;
		detail.name = _T("BEGIN");
		detail.value = _T("VCARD");
		details.insert(details.begin(), detail);
	}

	bool foundEnd = false;
	for (n=0; n<(int)details.size(); n++)
	{
		if (details[n].name == _T("END") &&
			details[n].value == _T("VCARD"))
		{
			foundEnd = true;
			if (n != details.size()-1)
			{
				SDetail detail(details[n]);
				details.erase(details.begin()+n);
				details.push_back(detail);
				break;
			}
		}
	}
	if (!foundEnd)
	{
		SDetail detail;
		detail.name = _T("END");
		detail.value = _T("VCARD");
		details.push_back(detail);
	}

	bool foundVersion = false;
	for (n=0; n<(int)details.size(); n++)
	{
		if (details[n].name == _T("VERSION"))
		{
			foundVersion = true;
			if (n != 1)
			{
				SDetail detail(details[n]);
				details.erase(details.begin()+n);
				details.insert(details.begin()+1, detail);
				break;
			}
		}
	}
	if (!foundVersion)
	{
		SDetail detail;
		detail.name = _T("VERSION");
		detail.value = _T("1.2");
		details.insert(details.begin()+1, detail);
	}
}

bool CVCard::QuotedPrintableDecode(std::string &s)
{
	bool ok=true;
	std::string s2;
	for (int n=0; n<(int)s.length(); )
	{
		char c=s[n];
		if (c=='=')
		{
			if (n+2<=(int)s.length())
			{
				std::string s3;
				s3+=s[n+1];
				s3+=s[n+2];
				char h[3];
				h[0]=s3[0];
				h[1]=s3[1];
				h[2]=0;
				int i;
				if (sscanf(h, "%X", &i)==1)
				{
					char c2((unsigned)i);
					s2.append(&c2, 1);
				}
				else
					ok = false;
			}
			n+=3;
		}
		else
		{
			s2+=c;
			n++;
		}
	}
	s = s2;
	return ok;
}
