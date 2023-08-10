#include "Game.h"
#include "GameBuffer.h"
#include "ConsoleInput.h"
#include "BufferController.h"
#include <mutex>

class BufferController : public GameComponent {
	std::mutex mutex;
	GameBuffer buf;

public:
	BufferController() : buf([this](const char* buf, size_t len) {
							 struct
							 {
								 const char* buf_1;
								 size_t len;
							 } pushevt{ buf, len };
							 parent->Raise("push", pushevt);
						 }) {
	}
	// 通过 Component 继承
	virtual void ProcessEvent(const char* evt, const void* evtargs) {
		if (strcmp(evt, "start") == 0) {
			parent->Raise("fresize");
			buf.InitConsole();
		}
		if (strcmp(evt, "tick") == 0) {
			std::lock_guard lg(mutex);
			buf.Clear();
			parent->Raise("draw", buf);
			buf.Output();
		}
		if (strcmp(evt, "resize") == 0) {
			std::lock_guard lg(mutex);
			auto rea = *(ResizeEventArgs*)evtargs;
			buf.ResizeBuffer(rea.X, rea.Y);
		}
	}
};

GameComponent* MakeBufferController() {
	return new BufferController();
}
