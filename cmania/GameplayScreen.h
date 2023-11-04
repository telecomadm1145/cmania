#pragma once
#include <string>
#include "ScreenController.h"
#include "OsuMods.h"
#include "OsuStatic.h"
#include "Record.h"

Screen* MakeGameplayScreen(const std::string& bmp_path, OsuMods mod,int mode);
Screen* MakeGameplayScreen(Record rec,const std::string& bmp_path,int mode);