#pragma once
#include <tuple>
#include <optional>
#include "Stopwatch.h"
#include <vector>
#include "InputEvent.h"
/// <summary>
/// 表示一个输入处理器，可以提供用户输入也可以提供录像输入如果需要
/// </summary>
class InputHandler {
public:
	// 获取某个Action的状态（可以是鼠标也可以是键盘或者其他什么东西输入）
	virtual bool GetKeyStatus(int action) = 0;
	// 获取鼠标位置（可能没有）
	virtual std::tuple<int, int> GetMousePosition() = 0;
	// 拉取输入
	virtual std::optional<InputEvent> PollEvent() = 0;
	// 设置时钟来源
	virtual void SetClockSource(Stopwatch& sw) = 0;
	// 加载Action的键位绑定
	virtual void SetBinds(const std::vector<int>& KeyBinds) = 0;

public:
	virtual ~InputHandler() {
	}
};
