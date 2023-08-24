#pragma once
#include "Beatmap.h"
#include "ManiaObject.h"
#include "OsuMods.h"

double CalculateDiff(const Beatmap<ManiaObject>& beatmap, OsuMods om, int keys);
