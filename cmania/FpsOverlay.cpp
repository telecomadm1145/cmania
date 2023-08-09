#include "Game.h"
#include "GameBuffer.h"
#include <string>
#include "Hpet.h"
#include "FpsOverlay.h"

class FpsOverlay : public GameComponent {
	// Í¨¹ý Component ¼Ì³Ð
	virtual void ProcessEvent(const char* evt, const void* evtargs) {
		static int tickcount = 0;
		static double lastcount = 0;
		static double lasttick = 0;
		static int fps = 0;
		if (strcmp(evt, "tick") == 0) {
			double time = *(double*)evtargs;
			if (time - lastcount >= 1000) {
				fps = (int)round(tickcount / (time - lastcount) * 1000);
				lastcount = time;
				tickcount = 0;
			}
			tickcount++;
		}
		if (strcmp(evt, "draw") == 0) {
			double time = HpetClock();
			auto& buf = *(GameBuffer*)evtargs;
			auto fpstext = "FPS:" + std::to_string(fps);
			auto ltc = std::to_string(time - lasttick);
			ltc.resize(6);
			auto latencytext = ltc + "ms";
			lasttick = time;
			auto width = std::max(fpstext.size(), latencytext.size());
			buf.DrawString(fpstext + "\n" + latencytext, buf.Width - 1 - width, buf.Height - 3, { 240, 0, 170, 255 }, {});
		}
	}
};

GameComponent* MakeFpsOverlay() {
	return new FpsOverlay();
}
