#pragma once

void rtrimwsv(std::wstring_view& sv)
{
	size_t end = sv.find_last_not_of(L' ');
	if (end != std::wstring_view::npos)
		sv.remove_suffix(sv.size() - end - 1);
}

// https://www.bfilipek.com/2018/07/string-view-perf-followup.html
std::vector<std::wstring_view> splitwsv(std::wstring_view strv, wchar_t delim = L' ')
{
	std::vector<std::wstring_view> output;
	size_t first = 0;

	while (first < strv.size())
	{
		const auto second = strv.find_first_of(delim, first);

		if (first != second)
			output.emplace_back(strv.substr(first, second - first));

		if (second == std::string_view::npos)
			break;

		first = second + 1;
	}

	return output;
}

std::vector<std::wstring> splitws(std::wstring_view strv, wchar_t delim = L' ')
{
	std::vector<std::wstring> output;
	size_t first = 0;

	while (first < strv.size())
	{
		const auto second = strv.find_first_of(delim, first);

		if (first != second)
			output.emplace_back(strv.substr(first, second - first));

		if (second == std::string_view::npos)
			break;

		first = second + 1;
	}

	return output;
}
