#pragma once
#include <vector>
#include "OsuStatic.h"
#include "HitObject.h"

struct TimingPoint
{
	double StartTime;
	double Bpm;
	double Multiplier;
	SampleBank SampleBank;
	int SampleSet;
	EffectFlags Effects = EffectFlags::None;
	int Volume;
};

struct BeatmapDifficultyInfo
{
	double HPDrainRate = 0;
	double CircleSize = 0;
	double OverallDifficulty = 0;
	double ApproachRate = 0;
	double SliderMultiplier = 0;
	double SliderTickRate = 0;
};

template <class HitObject>
using Beatmap = std::vector<HitObject>;