#include "Debug.h"
#include "Game.h"
#include "LogOverlay.h"

extern Game game;

void LogDebug(const std::string& str) {
	game.GetFeature<ILogger>().LogError(str);
}
