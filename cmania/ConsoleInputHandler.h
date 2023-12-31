﻿#pragma once
#include <mutex>
#include <map>
#include <queue>
#include "Stopwatch.h"
#include "ConsoleInput.h"
#include "InputEvent.h"
#include "InputHandler.h"

class ConsolePlayerInputHandler : public InputHandler {
	std::mutex mtx;
	std::map<int, ConsoleKey> KeyBinds;
	std::queue<InputEvent> events;
	std::tuple<int, int> pos = {-999,-999};
	Stopwatch* sw;
	bool keyMap[128]{}; // 标示按键状态
public:
	virtual bool GetKeyStatus(int action) override {
		std::lock_guard<std::mutex> lock(mtx);
		return keyMap[action];
	}
	virtual std::tuple<int, int> GetMousePosition() override {
		return pos;
	}
	virtual void SetClockSource(Stopwatch& sw) override {
		this->sw = &sw;
	}
	virtual std::optional<InputEvent> PollEvent() override {
		std::lock_guard<std::mutex> lock(mtx);
		if (events.empty())
			return std::nullopt;
		auto e = events.front();
		events.pop();
		return e;
	}

	void OnKeyEvent(ConsoleKey ck, bool pressed) {
		int action = -1;
		for (size_t i = 0; i < 18; i++) {
			if (KeyBinds[i] == ck) {
				action = i;
			}
		}
		if (action == -1)
			return;
		if (keyMap[action] ^ pressed) {
			keyMap[action] = pressed;
			InputEvent evt{};
			evt.Action = action;
			evt.Pressed = pressed;
			evt.Clock = sw->Elapsed();
			events.push(evt);
		}
	}

	void OnMouseKey(MouseKeyEventArgs mkea) {
		int action = -1;
		for (size_t i = 0; i < 18; i++) {
			if (KeyBinds[i] == (ConsoleKey)mkea.MouseButton) {
				action = i;
			}
		}
		if (action == -1)
			return;
		if (keyMap[action] ^ mkea.Pressed) {
			keyMap[action] = mkea.Pressed;
			InputEvent evt{};
			evt.Action = action;
			evt.Pressed = mkea.Pressed;
			evt.Clock = sw->Elapsed();
			evt.X = mkea.X;
			evt.Y = mkea.Y;
			events.push(evt);
		}
	}
	double lastrec = -1e300;
	void OnMouseMove(MoveEventArgs mea) {
		double t = sw->Elapsed();
		if (t > lastrec + 20) {
			pos = { mea.X, mea.Y };
			InputEvent evt{};
			evt.Action = -1;
			evt.Pressed = 0;
			lastrec = evt.Clock = t;
			evt.X = mea.X;
			evt.Y = mea.Y;
			events.push(evt);
		}
	}
	virtual void SetBinds(const std::vector<int>& KeyBinds) override {
		int i = 0;
		for (auto bind : KeyBinds) {
			this->KeyBinds[i++] = (ConsoleKey)bind;
		}
	}
};