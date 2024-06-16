#pragma once

#ifdef __linux__
#include <string>
#include <functional>
#include "ConsoleInput.h"
// 初始化
std::string mouse_device, keyboard_device;
enum InputFeature {
	ONLY_ANSI = 0b00000001,
	ONLY_DEVICE = 0b00000010,
	BOTH = 0b00000100,
};
InputFeature feature = BOTH;
void Init() {
	static bool hasRunned = false;
	if (!hasRunned) {
		hasRunned = true;
		// TODO:使用一个界面来矫正鼠标和键盘设备
		mouse_device = "/dev/input/event7";
		keyboard_device = "/dev/input/event8";
		feature = BOTH;
	}
}
void EventLoop(std::function<void(int x, int y, MouseKeyEventArgs::Button b, bool pressed)> mousePress, std::function<void(ControlKeyState cks, bool down, ConsoleKey key, wchar_t chr, int rc)> keyPress,InputFeature withFeature = BOTH) {
    Init();
    
}


#endif