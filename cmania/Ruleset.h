﻿#pragma once
#include "InputHandler.h"
#include "Stopwatch.h"
#include "OsuMods.h"
#include "Record.h"
#include "SettingStorage.h"
#include "GameBuffer.h"
#include "ScoreProcessor.h"
#include "Beatmap.h"
class RulesetBase {
public:
	InputHandler* RulesetInputHandler = 0;
	Stopwatch Clock;
	OsuMods Mods = OsuMods::None;
	bool GameEnded = false;
	Record RulesetRecord{};
	virtual ~RulesetBase() {}
	virtual void LoadSettings(BinaryStorage& settings) = 0;
	virtual void Load(std::filesystem::path beatmap_path) = 0;
	virtual void Update() = 0;
	virtual void Render(GameBuffer& buffer) = 0;
	virtual void Pause() = 0;
	virtual void Resume() = 0;
	virtual void Skip() = 0;
	virtual double GetCurrentTime() = 0;
	virtual double GetDuration() = 0;
	virtual ScoreProcessorBase* GetScoreProcessor() = 0;
};
template <class HitObject>
class Ruleset : public RulesetBase {
public:
	ScoreProcessor<HitObject>* RulesetScoreProcessor = 0;
	Beatmap<HitObject> Beatmap;
	virtual ScoreProcessorBase* GetScoreProcessor() override {
		return RulesetScoreProcessor;
	}
};