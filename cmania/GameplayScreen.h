#pragma once
#include <string>
#include "ScreenController.h"
#include "OsuMods.h"
#include "OsuStatic.h"
#include "Record.h"
#include "Ruleset.h"

Screen* MakeGameplayScreen(Ruleset* rul,const std::string& bmp_path, OsuMods mod,int mode);
Screen* MakeGameplayScreen(Ruleset* rul, Record rec, const std::string& bmp_path, int mode);