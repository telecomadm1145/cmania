#pragma once
#include "Ruleset.h"
#include "OmDifficultyCalculator.h"

class OsuManiaRuleset : Ruleset {
	virtual uint64_t GetRulesetId() override {
		return 3;
	}
	virtual void LoadSettings(BinaryStorage& settings) override {
	}
	virtual void GetBeatmaps(std::string bmppath, std::vector<BeatmapInfo>& infos) override {
	}
	virtual double CalculateDifficulty(std::string bmppath, OsuMods mods) override {
		auto ifs = std::ifstream(bmppath);
		OsuBeatmap ob = OsuBeatmap::Parse(ifs);
		return OmCalculateDiff(ob, mods);
	}
	virtual Gameplay* CreateGameplay(std::string bmppath) override {
	}
};