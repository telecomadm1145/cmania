#include "Debug.h"
#include "Game.h"
#include "LogOverlay.h"

#ifdef _WIN32
#include <Windows.h>
#endif

extern Game game;

void LogDebug(const std::string& str) {
#ifdef _DEBUG
	game.GetFeature<ILogger>().LogInfo(str);
#endif // _DEBUG
#ifdef _WIN32
	OutputDebugStringA(str.c_str());
	OutputDebugStringA("\r\n");
#endif
}
