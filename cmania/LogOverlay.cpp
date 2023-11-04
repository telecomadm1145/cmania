#include "LogOverlay.h"
#include "Hpet.h"
#include "GameBuffer.h"
#include <deque>
class LogOverlay : public GameComponent, public ILogger {
	struct LogEntry {
		enum LogLevel {
			Warn,
			Info,
			Error,
		} Level;
		std::string Text;
		double starttime;
	};
	std::deque<LogEntry> ents;
	virtual void ProcessEvent(const char* evt, const void* evtargs) {
		if (strcmp(evt, "start") == 0) {
			parent->RegisterFeature((ILogger*)this);
		}
		if (strcmp(evt, "tick") == 0) {
			if (ents.size() == 0)
				return;
			double time = HpetClock();
			auto t = time - ents.back().starttime;
			if (t > 1600) {
				ents.pop_back();
			}
		}
		if (strcmp(evt, "draw") == 0) {
			double time = HpetClock();
			auto& buf = *(GameBuffer*)evtargs;
			int y = buf.Height - 2;
			for (auto& ent : ents) {
				auto alpha = std::min(2-(time - ent.starttime) / 800.0,1.0);
				Color fore = { 255, 255, 255, 255 };
				Color back = { 180, 0, 0, 0 };
				if (ent.Level == LogEntry::Error)
				{
					fore = {255,255,40,40};
					back = {255,120,0,0};
				}
				else if (ent.Level == LogEntry::Warn) {
					fore = { 255, 230, 240, 20 };
					back = { 255, 120, 110, 10 };
				}
				else if (ent.Level == LogEntry::Info) {
					fore = { 255, 255, 255, 255 };
					back = { 255, 40, 40, 40 };
				}
				buf.DrawString(ent.Text, 0, y--, fore * alpha, back * alpha);
				if (y < 10)
					break;
			}
		}
	}

	// Í¨¹ý ILogger ¼Ì³Ð
	void Log(Level level, std::string s) override {
		double time = HpetClock();
		ents.push_front(LogEntry{ (LogEntry::LogLevel)level, s, time });
	}
};
GameComponent* MakeLogOverlay() {
	return new LogOverlay();
}