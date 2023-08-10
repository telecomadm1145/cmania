#pragma once
#include <vector>
#include "ConsoleInput.h"

extern std::vector<ConsoleKey> key_binds;
inline std::vector<ConsoleKey> GetKeyBinds(int keys)
{
	if (keys == 0)
		return {};
	std::vector<ConsoleKey> res;

	auto half = keys <= 9 ? keys / 2 : keys / 2 / 2;
	for (int i = 4 - half; i < 4; i++)
	{
		res.push_back(key_binds[i]);
	}
	if (keys <= 9 ? (keys % 2 != 0) : (keys % 4 != 0))
	{
		res.push_back(key_binds[4]);
	}
	for (int i = 5; i < 5 + half; i++)
	{
		res.push_back(key_binds[i]);
	}
	if (keys > 9)
	{
		for (int i = 9 + 4 - half; i < 9 + 4; i++)
		{
			res.push_back(key_binds[i]);
		}
		if (keys % 4 != 0)
		{
			res.push_back(key_binds[13]);
		}
		for (int i = 9 + 5; i < 9 + 5 + half; i++)
		{
			res.push_back(key_binds[i]);
		}
	}
	return res;
}