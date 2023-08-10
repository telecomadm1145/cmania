#pragma once
/// <summary>
/// 表示一个输入事件（可以是玩家输入也可以是录像输入）
/// </summary>
class InputEvent {
public:
	int Action;
	double Clock;
	bool Pressed;
	// Mouse
	int X;
	int Y;
};