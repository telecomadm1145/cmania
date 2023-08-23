#pragma once
#include "OsuBeatmap.h"
#include "OsuMods.h"

double OmCalculateDiff(const OsuBeatmap& beatmap, OsuMods om = OsuMods::None, int keys = 0);
