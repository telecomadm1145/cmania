#pragma once
#include "Stopwatch.h"
#include <deque>
#include "InputEvent.h"
#include "InputHandler.h"
#include "Record.h"
class RecordInputHandler : public InputHandler {
public:
	Stopwatch* sw = 0;
	std::deque<InputEvent> rec;
	bool KeyStatus[128]{};
	RecordInputHandler() = default;
	RecordInputHandler(const Record& rec) : rec(rec.Events.begin(), rec.Events.end()) {}

	void LoadRecord(const Record& rec) {
		this->rec = std::deque<InputEvent>(rec.Events.begin(), rec.Events.end());
	}
	// 通过 InputHandler 继承
	virtual bool GetKeyStatus(int action) override {
		return KeyStatus[action];
	}
	virtual std::tuple<int, int> GetMousePosition() override {
		return std::tuple<int, int>();
	}
	virtual std::optional<InputEvent> PollEvent() override {
		if (rec.size() == 0)
			return {};
		if (sw->Elapsed() > rec.front().Clock) {
			auto res = rec.front();
			KeyStatus[res.Action] = res.Pressed;
			if (rec.size() > 0)
				rec.pop_front();
			return res;
		}
		return {};
	}
	virtual void SetClockSource(Stopwatch& sw) override {
		this->sw = &sw;
	}
	virtual void SetBinds(const std::vector<int>& KeyBinds) override {
	}
};