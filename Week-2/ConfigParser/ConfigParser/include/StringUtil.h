#pragma once

#include <string>

inline std::string Trim(const std::string& s)
{
	const std::string whitespace = " \t\r\n";
	const auto first = s.find_first_not_of(whitespace);
	if (first == std::string::npos)
	{
		return "";
	}

	const auto last = s.find_last_not_of(whitespace);
	return s.substr(first, last - first + 1);
}
