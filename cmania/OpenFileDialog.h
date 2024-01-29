#pragma once
#include "ScreenController.h"
#include <functional>
#include <filesystem>

Screen* PickFile(std::string prompt, std::function<void(std::filesystem::path)> callback, std::function<void()> oncancel = {}, bool pickfolder = false, std::filesystem::path def = {});