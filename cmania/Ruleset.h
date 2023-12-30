#pragma once
#include "Gameplay.h"
class DifficultyInfoItem {
	std::string Text;
	double Value;
	double MinValue;
	double MaxValue;
	enum {
		ValueBar,
		Header
	} Type;
};
using DifficultyInfo = std::vector<DifficultyInfoItem>;
class Ruleset
{
public:
	virtual std::string Id() = 0;
	virtual std::string DisplayName() = 0;
	virtual Beatmap* LoadBeatmap(path beatmap_path) = 0;
	virtual GameplayBase* GenerateGameplay() = 0;
	virtual double CalculateDifficulty(Beatmap* bmp,OsuMods mods) = 0;
	virtual DifficultyInfo PopulateDifficultyInfo(Beatmap* bmp) = 0;
	virtual ~Ruleset() {

	}
};