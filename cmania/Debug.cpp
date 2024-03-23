#include "Debug.h"
#include "Game.h"
#include "LogOverlay.h"

extern Game game;

void LogDebug(const std::string& str) {
#ifdef _DEBUG
	game.GetFeature<ILogger>().LogInfo(str);
#endif // _DEBUG
}
