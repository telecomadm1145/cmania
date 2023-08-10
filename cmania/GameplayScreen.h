#pragma once
#include <string>
#include "ScreenController.h"
#include "OsuMods.h"
#include "OsuStatic.h"
#include "Record.h"

Screen* MakeGameplayScreen(const std::string& bmp_path, OsuMods mod);
Screen* MakeGameplayScreen(Record rec,const std::string& bmp_path);