// Small INI parsing helpers shared by Host and GiftBox (comma lists, ints).
#pragma once

#include <string>
#include <vector>
#include <cstring>
#include <cctype>
#include <cstdlib>

namespace GiftBoxHost::Ini
{
	inline bool IsNoneToken(const std::string& s)
	{
		return s.empty() || _stricmp(s.c_str(), "none") == 0 || _stricmp(s.c_str(), "<none>") == 0;
	}

	inline std::string Trim(const std::string& s)
	{
		size_t a = 0, b = s.size();
		while (a < b && std::isspace((unsigned char)s[a])) ++a;
		while (b > a && std::isspace((unsigned char)s[b - 1])) --b;
		return s.substr(a, b - a);
	}

	inline std::vector<std::string> SplitList(const char* raw)
	{
		std::vector<std::string> out;
		std::string cur;
		for (const char* p = raw; ; ++p)
		{
			if (*p == ',' || *p == '\0')
			{
				std::string tok = Trim(cur);
				if (!IsNoneToken(tok)) out.push_back(tok);
				cur.clear();
				if (*p == '\0') break;
			}
			else cur.push_back(*p);
		}
		return out;
	}

	inline std::vector<int> SplitInts(const char* raw)
	{
		std::vector<int> out;
		for (const std::string& tok : SplitList(raw))
			out.push_back(atoi(tok.c_str()));
		return out;
	}
}
