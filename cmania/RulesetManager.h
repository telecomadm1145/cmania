#pragma once
#include "Game.h"
#include "Ruleset.h"
class IRulesetManager
{
public:
	virtual Ruleset& GetRuleset(std::string name) const = 0;
	/// <summary>
	/// 注册，这将把 Ruleset 的控制权转交给 RulesetManager
	/// </summary>
	/// <param name="rul"></param>
	virtual void Register(Ruleset* rul) = 0;
};

GameComponent* MakeRulesetManager();