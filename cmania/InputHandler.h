#pragma once
#include <tuple>
#include <optional>
#include "Stopwatch.h"
#include <vector>
#include "InputEvent.h"

class InputHandler {
public:
	virtual bool GetKeyStatus(int action) = 0;
	virtual std::tuple<int, int> GetMousePosition() = 0;
	virtual std::optional<InputEvent> PollEvent() = 0;
	virtual void SetClockSource(Stopwatch& sw) = 0;
	virtual void SetBinds(const std::vector<int>& KeyBinds) = 0;

public:
	virtual ~InputHandler() {
	}
};
