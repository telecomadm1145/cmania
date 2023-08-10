#pragma once
#include <string>
#include <sstream>
#include <vector>

template<typename T>
std::vector<std::basic_string<T>> split(const std::basic_string<T>& str, T delimiter)
{
	std::vector<std::basic_string<T>> tokens;
	std::basic_stringstream<T> tokenStream(str);
	std::basic_string<T> token;

	while (std::getline(tokenStream, token, delimiter))
	{
		tokens.push_back(token);
	}

	return tokens;
}
template<typename T> std::basic_string<T> Trim(const std::basic_string<T>& str, T remove = ' ') {
	const auto first = str.find_first_not_of(remove);
	if (std::basic_string<T>::npos == first) {
		return str;
	}
	const auto last = str.find_last_not_of(remove);
	return str.substr(first, (last - first + 1));
}
template<typename T>
T ConvertFromString(const std::string& str) {
	T value;
	std::istringstream iss(str);
	iss >> value;
	return value;
}
inline bool EndsWith(const std::string& str, const std::string& ending) {
	if (ending.size() > str.size()) {
		return false;
	}

	return std::equal(ending.rbegin(), ending.rend(), str.rbegin());
}
inline bool StartsWith(const std::string& str, const std::string& starting) {
	if (starting.size() > str.size()) {
		return false;
	}

	return std::equal(starting.begin(), starting.end(), str.begin());
}
template<typename T>
bool IsEqualsNoCase(const std::basic_string<T>& str1, const std::basic_string<T>& str2) {
	if (str1.length() != str2.length()) {
		return false;
	}

	for (std::size_t i = 0; i < str1.length(); ++i) {
		if (std::tolower(str1[i]) != std::tolower(str2[i])) {
			return false;
		}
	}

	return true;
}
template<typename T>
bool FindNoCase(const std::basic_string<T>& str1, const std::basic_string<T>& str2) {
	if (str2.size() > str1.size())
		return false;
	std::size_t matches = 0;
	for (std::size_t i = 0; i < str1.length(); ++i) {
		if (matches >= str2.size() - 1)
			return true;
		if (std::tolower(str1[i]) != std::tolower(str2[matches])) 
		{
			matches = 0;
		}
		else
		{
			matches++;
		}
	}

	return matches >= str2.size() - 1;
}

template<typename T>
bool search(const std::basic_string<T>& a, const std::basic_string<T>& b)
{
	for (auto str : split<T>(b, ' '))
	{
		if (!str.empty() && FindNoCase(a, str))
			return true;
	}
	return false;
}
template<typename T>
bool search_meta(std::basic_string<T> arg, std::basic_string<T> meta1)
{
	return search(meta1, arg);
}
template<typename T>
bool search_meta(std::basic_string<T> arg, std::basic_string<T> meta1, std::basic_string<T> metas...)
{
	return search(meta1, arg) || search_meta(arg, metas);
}