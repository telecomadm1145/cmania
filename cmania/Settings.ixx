export module Settings;
import <any>;
import Game;
import <map>;
import <string>;
import <fstream>;

export struct SetEventArgs
{
	const char* Key;
	std::any Value;
};

export class SettingsService : public GameComponent
{
	std::map<std::string, std::any> storage;
	bool nested = false;
	// Í¨¹ý Component ¼Ì³Ð
	virtual void ProcessEvent(const char* evt, const void* evtargs) override
	{
		if (strcmp(evt, "require"))
		{
			auto name = (const char*)evtargs;
			if (storage[name].has_value())
			{
				nested = true;
				SetEventArgs sea{};
				sea.Key = name;
				sea.Value = storage[name];
				parent->Raise("set", sea);
				nested = false;
			}
		}
		if (strcmp(evt, "set"))
		{
			if (nested)
				return;
			auto sea = *(SetEventArgs*)evtargs;
			storage[sea.Key] = sea.Value;
		}
	}
};