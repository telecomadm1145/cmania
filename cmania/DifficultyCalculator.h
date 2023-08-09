#pragma once
#include "OsuBeatmap.h"
#include "OsuMods.h"

double CalculateDiff(const OsuBeatmap& beatmap, OsuMods om = OsuMods::None, int keys = 0);
