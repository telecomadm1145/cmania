#pragma once
#include "OsuMods.h"
#include "Beatmap.h"

template <class HitObject>
class DifficultyCalculator {
public:
	double GetDifficulty(const Beatmap<HitObject>& objects,OsuMods mods);
};