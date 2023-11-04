#include <thread>
#include "Game.h"
#include "Hpet.h"
#include "TickSource.h"
#include "LogOverlay.h"

class TickSource : public GameComponent {
	// 通过 Component 继承
	virtual void ProcessEvent(const char* evt, const void* evtargs) {
		if (strcmp(evt, "start") == 0) {
			BeginHighResClock();
			std::thread thread(TickingWorker, parent);
			thread.detach();
		}
	}
	static void TickingWorker(Game* parent) {
		double last_tick = 0;
		while (true) {
			int fps = parent->Settings["MyCompSuck"].Get<bool>() ? 120 : 1000;
			try {
				parent->Raise("tick", last_tick = HpetClock());
			}
			catch (const std::exception& ex) {
				parent->GetFeature<ILogger>().LogError(ex.what());
			}
			catch (...) {
				parent->GetFeature<ILogger>().LogError("Error during tick.");
			}
			if (fps < 480 && fps > 1) {
				auto now = HpetClock();
				auto offset = 1000.0 / fps - (now - last_tick);
				if (offset > 1) {
					std::this_thread::sleep_for(std::chrono::milliseconds((int)offset));
				}
			}
		}
	}
};

GameComponent* MakeTickSource() {
	return new TickSource();
}
