#include "Game.h"
#include "Defines.h"
#include "Ruleset.h"
#include "RulesetManager.h"

class RulesetManager : public GameComponent,public IRulesetManager {
	std::unordered_map<std::string, std::unique_ptr<Ruleset>> rulesets;

public:
	Ruleset& GetRuleset(std::string name) const
	{
		return *rulesets.at(name).get();
	}
	void Register(Ruleset* rul) override
	{
		if (rulesets.find(rul->Id()) != rulesets.end())
		{
			throw std::exception("Ruleset have registered before");
		}
		rulesets.insert({ rul->Id(), std::unique_ptr<Ruleset>(rul) });
	}
	void ProcessEvent(const char* evt, const void* evtargs)
	{
		if (strcmp(evt, "start"))
		{
			parent->RegisterFeature<IRulesetManager>(this);
		}
	}
};

GameComponent* MakeRulesetManager() {
	return new RulesetManager();
}
