#pragma once
#include "Game.h"
#include "GameBuffer.h"
#include "ConsoleInput.h"

class Overlay {
public:
	virtual ~Overlay() = default;
	virtual void Render(GameBuffer& buf){};
	virtual void Key(KeyEventArgs kea){};
	virtual void MouseKey(MouseKeyEventArgs mkea){};
	virtual bool HitTest(int x, int y) { return false; };
	virtual void Tick(double fromRun){};
};


class Screen {
protected:
	class ScreenController* parent = 0;
	Game* game = 0;

public:
	virtual ~Screen() = default;
	void Init(class ScreenController* ptr, Game* ptr2) {
		parent = ptr;
		game = ptr2;
	}
	virtual void Render(GameBuffer& buf){};
	virtual void Tick(double fromRun){};
	virtual void Initalized(){};
	virtual void Key(KeyEventArgs kea){};
	virtual void Wheel(WheelEventArgs wea){};
	virtual void Move(MoveEventArgs mea){};
	virtual void Activate(bool){};
	virtual void Resize(){};
	virtual void MouseKey(MouseKeyEventArgs mkea){};
	virtual void ProcessEvent(const char* evt, const void* evtargs){};
};
class HotKeyHandler {
public:
	virtual ~HotKeyHandler() = default;
	/// <summary>
	/// 处理按键，返回 true 则拦截事件
	/// </summary>
	/// <param name="kea">按键事件</param>
	/// <returns>返回 true 则拦截事件</returns>
	virtual bool Key(KeyEventArgs kea) { return false; };
};

class ScreenController : public GameComponent {
	std::vector<Screen*> history;
	int history_index = 0;
	Screen* current = 0;

	std::vector<Overlay*> overlays; // Overlay 列表
	Overlay* topOverlay = nullptr;	// 顶层 Overlay

	std::vector<HotKeyHandler*> hotkeys;

	// 通过 Component 继承
	virtual void ProcessEvent(const char* evt, const void* evtargs) {
		if (strcmp(evt, "navigate") == 0) {
			Navigate((Screen*)evtargs);
			return;
		}
		if (current != 0) {
			current->ProcessEvent(evt, evtargs);
			if (strcmp(evt, "draw") == 0) {
				current->Render(*(GameBuffer*)evtargs);
			}
			if (strcmp(evt, "tick") == 0) {
				current->Tick(*(double*)evtargs);
			}
			if (topOverlay) {
				if (strcmp(evt, "draw") == 0) {
					topOverlay->Render(*(GameBuffer*)evtargs);
				}
				if (strcmp(evt, "key") == 0) {
					topOverlay->Key(*(KeyEventArgs*)evtargs);
					return;
				}
				if (strcmp(evt, "mousekey") == 0) {
					MouseKeyEventArgs& mkea = *(MouseKeyEventArgs*)evtargs;
					// 分发鼠标按键事件给 Overlays
					bool hitTest = false;
					for (auto overlay : overlays) {
						if (overlay->HitTest(mkea.X, mkea.Y)) {
							overlay->MouseKey(mkea);
							hitTest = true;
							topOverlay = overlay; // 设置顶层 Overlay
							break;				  // 如果命中 Overlay,则不再继续检测
						}
					}
					// 如果未命中任何 Overlay,则自动关闭所有 Overlay
					if (!hitTest && !mkea.Pressed) {
						for (auto overlay : overlays) {
							delete overlay;
						}
						overlays.clear();
						topOverlay = nullptr;
					}
					return;
				}
			}
			if (strcmp(evt, "key") == 0) {
				auto& kea = *(KeyEventArgs*)evtargs;
				auto hit = false;
				for (auto handler : hotkeys)
					if (handler->Key(kea))
						hit = true;

				if (!hit)
					current->Key(kea);
			}
			if (strcmp(evt, "wheel") == 0) {
				current->Wheel(*(WheelEventArgs*)evtargs);
			}
			if (strcmp(evt, "mousekey") == 0) {
				current->MouseKey(*(MouseKeyEventArgs*)evtargs);
			}
			if (strcmp(evt, "move") == 0) {
				current->Move(*(MoveEventArgs*)evtargs);
			}
			if (strcmp(evt, "resize") == 0) {
				current->Resize();
			}
		}
	}

public:
	template <typename T>
	void Navigate() {
		T* scr = new T();
		Navigate(scr);
	}
	void Navigate(Screen* scr) {
		if (scr) {
			scr->Init(this, parent);
			scr->Initalized();
		}
		if (current)
			current->Activate(false);
		current = scr;
		if (history_index < history.size()) {
			delete history[history_index];
			history[history_index] = scr;
		}
		else {
			history.insert(history.begin() + history_index, scr);
		}
		history_index++;
		if (current)
			current->Activate(true);
	}
	void GoForward() {
		history_index++;
		int ind = history_index - 1;
		if (ind < 0 || ind >= history.size())
			return;
		if (current)
			current->Activate(false);
		current = history[ind];
		if (current)
			current->Activate(true);
	}
	void Back() {
		history_index--;
		int ind = history_index - 1;
		if (ind < 0 || ind >= history.size())
			return;
		if (current)
			current->Activate(false);
		current = history[ind];
		if (current)
			current->Activate(true);
	}

	// 添加 Overlay 管理函数
	void AddOverlay(Overlay* overlay) {
		overlays.push_back(overlay);
		topOverlay = overlay;
	}

	void RemoveOverlay(Overlay* overlay) {
		auto it = std::find(overlays.begin(), overlays.end(), overlay);
		if (it != overlays.end()) {
			overlays.erase(it);
			delete overlay;
		}
	}
	void AddKeyHandler(HotKeyHandler* kh) {
		hotkeys.push_back(kh);
	}
};

inline GameComponent* MakeScreenController() {
	return new ScreenController();
}