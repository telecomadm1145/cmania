#pragma once
#include <vector>
#include <map>
#include "OsuMods.h"
#include "Record.h"
#include "OsuStatic.h"
class ScoreProcessorBase {
public:
	unsigned int Combo = 0;
	unsigned int MaxCombo = 0;
	unsigned int BeatmapMaxCombo = 0;
	unsigned int AppliedHit = 0;
	double Rating = 0;
	double Mean = 0;
	double Error = 0;
	double RawError = 0;
	std::vector<double> Errors;
	std::map<HitResult, int> ResultCounter;
	unsigned long long RawAccuracy = 0;
	double Accuracy = 0;
	unsigned long long RawScore = 0;
	double Score = 0;
	Record* RulesetRecord = 0;
	virtual ~ScoreProcessorBase() {}
	virtual void SetDifficulty(double diff) = 0;
	virtual void SetMods(OsuMods mods) = 0;
	virtual double GetHealthIncreaseFor(HitResult res) = 0;
	virtual void SaveRecord() = 0;
};

template <class HitObject>
class ScoreProcessor : public ScoreProcessorBase {
public:
	virtual void ApplyBeatmap(double ref_rating) = 0;
	virtual HitResult ApplyHit(HitObject& mo, double err) = 0;
};