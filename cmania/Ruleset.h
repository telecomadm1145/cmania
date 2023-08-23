#pragma once
#include "InputHandler.h"
#include "Stopwatch.h"
#include "OsuMods.h"
#include "Record.h"
#include "SettingStorage.h"
#include "GameBuffer.h"
#include "ScoreProcessor.h"
#include "Beatmap.h"
#include "Gameplay.h"
#include "RulesetRenderer.h"
#include "WorkingBeatmap.h"

class BeatmapInfo {
public:
	uint64_t RulesetId = 114514;
	std::string Title;
	std::string Background;
	std::string Audio;
	std::string Name;
	double PreviewPoint;
	double Length;
	BeatmapDifficultyInfo DifficultyInfo;
	std::string Path;
};

class Ruleset {
public:
	virtual uint64_t GetRulesetId() = 0;
	virtual void LoadSettings(BinaryStorage& settings) = 0;
	virtual void GetBeatmaps(std::string bmppath, std::vector<BeatmapInfo>& infos) = 0;
	virtual double CalculateDifficulty(std::string bmppath, OsuMods mods) = 0;
	virtual Gameplay* CreateGameplay(std::string bmppath) = 0;
	
public:
	virtual ~Ruleset() {}
};