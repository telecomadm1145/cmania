#pragma once
#include "Game.h"
class ILogger {
public:
	enum Level {
		Warn,
		Info,
		Error,
	};
	virtual void Log(Level level, std::string s) = 0;
	void LogWarn(const std::string& s) {
		Log(Level::Warn, s);
	}
	void LogInfo(const std::string& s) {
		Log(Level::Info, s);
	}
	void LogError(const std::string& s) {
		Log(Level::Error, s);
	}
};

GameComponent* MakeLogOverlay();