#pragma once
#include "InputHandler.h"
#include "Stopwatch.h"
#include "OsuMods.h"
#include "Record.h"
#include "SettingStorage.h"
#include "GameBuffer.h"
#include "ScoreProcessor.h"
#include "Beatmap.h"

class Ruleset;

class GameplayBase {
public:
	Ruleset* Ruleset = 0;
	InputHandler* GameInputHandler = 0;
	Beatmap* Beatmap;
	Stopwatch Clock;
	OsuMods Mods = OsuMods::None;
	bool GameEnded = false;
	bool GameStarted = false;
	Record GameRecord{};
	virtual void LoadSettings(BinaryStorage& settings) = 0;
	virtual void Load(::Ruleset* rul,::Beatmap* bmp) = 0;
	virtual Record GetAutoplayRecord() = 0;
	virtual void Update() = 0;
	virtual void Render(GameBuffer&) = 0;
	virtual void Skip() = 0;
	virtual double GetCurrentTime() = 0;
	virtual double GetDuration() = 0;
	virtual ScoreProcessorBase* GetScoreProcessor() = 0;
	virtual std::string GetBgPath() = 0;

public:
	virtual ~GameplayBase() {}
};