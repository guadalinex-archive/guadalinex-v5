#ifndef _VCARD_H
#define _VCARD_H

#include <string>
#include <vector>
#include "ptools.h"

class CVCard
{
public:
	void fromString(const std::tstring &data);
	std::tstring toString();

	std::tstring getDetail(const std::tstring &detail) const
	{
		for (int i=0; i<(int)details.size(); i++)
			if (details[i].name==detail || details[i].name.find(detail)==0)
				return details[i].value;
		return _T("");
	}
	std::vector<std::tstring> getDetails(const std::tstring &detail) const
	{
		std::vector<std::tstring> tels;
		for (int i=0; i<(int)details.size(); i++)
			if (details[i].name==detail || details[i].name.find(detail)==0)
			{
				tels.push_back(details[i].value);
			}
		return tels;
	}
	std::tstring getDetailTag(const std::tstring &detail) const
	{
		for (int i=0; i<(int)details.size(); i++)
			if (details[i].name==detail || details[i].name.find(detail)==0)
				return details[i].name;
		return _T("");
	}
	std::vector<std::tstring> getDetailTags(const std::tstring &detail) const
	{
		std::vector<std::tstring> tags;
		for (int i=0; i<(int)details.size(); i++)
			if (details[i].name==detail || details[i].name.find(detail)==0)
			{
				tags.push_back(details[i].name);
			}
		return tags;
	}

	static bool QuotedPrintableDecode(std::string &string);
private:
	void normalize();

	struct SDetail
	{
		std::tstring name, value;
	};

	std::vector<SDetail> details;
};

#endif // _VCARD_H
