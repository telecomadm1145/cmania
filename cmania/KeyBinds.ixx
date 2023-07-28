export module KeyBinds;
import <vector>;
import "ConsoleInput.h";

export std::vector<ConsoleKey> key_binds = {
		ConsoleKey::A,ConsoleKey::S,ConsoleKey::D,ConsoleKey::F,
		ConsoleKey::Spacebar,
		ConsoleKey::J,ConsoleKey::K,ConsoleKey::L,ConsoleKey::Separator ,
		ConsoleKey::Q,ConsoleKey::W,ConsoleKey::E,ConsoleKey::R,
		ConsoleKey::M,
		ConsoleKey::U,ConsoleKey::I,ConsoleKey::I,ConsoleKey::P
};
export std::vector<ConsoleKey> GetKeyBinds(int keys)
{
	if (keys == 0)
		return {};
	std::vector<ConsoleKey> res;

	auto half = keys <= 9 ? keys / 2 : keys / 2 / 2;
	for (size_t i = 4 - half; i < 4; i++)
	{
		res.push_back(key_binds[i]);
	}
	if (keys <= 9 ? (keys % 2 != 0) : (keys % 4 != 0))
	{
		res.push_back(key_binds[4]);
	}
	for (size_t i = 5; i < 5 + half; i++)
	{
		res.push_back(key_binds[i]);
	}
	if (keys > 9)
	{
		for (size_t i = 9 + 4 - half; i < 9 + 4; i++)
		{
			res.push_back(key_binds[i]);
		}
		if (keys % 4 != 0)
		{
			res.push_back(key_binds[13]);
		}
		for (size_t i = 9 + 5; i < 9 + 5 + half; i++)
		{
			res.push_back(key_binds[i]);
		}
	}
	return res;
}