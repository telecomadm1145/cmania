#pragma once

#ifdef __linux__
#include <string>
#include <functional>
#include "ConsoleInput.h"
#include <event2/event.h>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>

// 初始化

std::string mouse_device, keyboard_device;
enum InputFeature {
	ONLY_ANSI = 0b00000001,
	ONLY_DEVICE = 0b00000010,
	BOTH = 0b00000011,
};
void Init() {
	static bool hasRunned = false;
	if (!hasRunned) {
		hasRunned = true;
		// TODO:使用一个界面来矫正鼠标和键盘设备
		mouse_device = "/dev/input/event7";
		keyboard_device = "/dev/input/event8";
	}
}
std::unordered_map<int,ConsoleKey> keyMap={
	{KEY_A,ConsoleKey::A},{KEY_B,ConsoleKey::B},{KEY_C,ConsoleKey::C},
	{KEY_D,ConsoleKey::D},{KEY_E,ConsoleKey::E},{KEY_F,ConsoleKey::F},
	{KEY_G,ConsoleKey::G},{KEY_H,ConsoleKey::H},{KEY_I,ConsoleKey::I},
	{KEY_J,ConsoleKey::J},{KEY_K,ConsoleKey::K},{KEY_L,ConsoleKey::L},
	{KEY_M,ConsoleKey::M},{KEY_N,ConsoleKey::N},{KEY_O,ConsoleKey::O},
	{KEY_P,ConsoleKey::P},{KEY_Q,ConsoleKey::Q},{KEY_R,ConsoleKey::R},
	{KEY_S,ConsoleKey::S},{KEY_T,ConsoleKey::T},{KEY_U,ConsoleKey::U},
	{KEY_V,ConsoleKey::V},{KEY_W,ConsoleKey::W},{KEY_X,ConsoleKey::X},
	{KEY_Y,ConsoleKey::Y},{KEY_Z,ConsoleKey::Z},{KEY_ENTER,ConsoleKey::Enter},
	{KEY_UP,ConsoleKey::UpArrow},{KEY_DOWN,ConsoleKey::DownArrow},
	{KEY_LEFT,ConsoleKey::LeftArrow},{KEY_RIGHT,ConsoleKey::RightArrow},
	{KEY_ESC,ConsoleKey::Escape},{KEY_SPACE,ConsoleKey::Spacebar},{KEY_BACK,ConsoleKey::Backspace},
	{KEY_F1,ConsoleKey::F1},{KEY_F2,ConsoleKey::F2},{KEY_F3,ConsoleKey::F3},
	{KEY_F4,ConsoleKey::F4},{KEY_F5,ConsoleKey::F5},{KEY_F6,ConsoleKey::F6},
	{KEY_F7,ConsoleKey::F7},{KEY_F8,ConsoleKey::F8},{KEY_F9,ConsoleKey::F9},
	{KEY_F10,ConsoleKey::F10},{KEY_F11,ConsoleKey::F11},{KEY_F12,ConsoleKey::F12},
	{KEY_TAB,ConsoleKey::Tab}
};
std::unordered_map<int,char> keyToANSI={
	{KEY_A,'a'},{KEY_B,'b'},{KEY_C,'c'},{KEY_D,'d'},{KEY_E,'e'},{KEY_F,'f'},
	{KEY_G,'g'},{KEY_H,'h'},{KEY_I,'i'},{KEY_J,'j'},{KEY_K,'k'},{KEY_L,'l'},
	{KEY_M,'m'},{KEY_N,'n'},{KEY_O,'o'},{KEY_P,'p'},{KEY_Q,'q'},{KEY_R,'r'},
	{KEY_S,'s'},{KEY_T,'t'},{KEY_U,'u'},{KEY_V,'v'},{KEY_W,'w'},{KEY_X,'x'},
	{KEY_Y,'y'},{KEY_Z,'z'}
};
void keyboardDeviceCallback(evutil_socket_t fd, short event, void* arg) {
	struct input_event ev;
	auto *_kp=(std::function<void(ControlKeyState cks, bool down, ConsoleKey key, wchar_t chr, int rc)>*)arg;
	auto kp=*_kp;
	while (read(fd, &ev, sizeof(ev)) > 0) {
		if(ev.type==EV_KEY){
			kp(ControlKeyState(0), ev.value == 1, keyMap[ev.code], keyToANSI[ev.code], 0);
		}
	}
}
void mouseDeviceCallback(evutil_socket_t fd, short event, void* arg) {
}
void stdinCallback(evutil_socket_t fd, short event, void* arg) {
}
//TODO: 增加合适的错误处理
event_base* base;
void EventLoop(std::function<void(int x, int y, MouseKeyEventArgs::Button b, bool pressed)> mousePress, std::function<void(ControlKeyState cks, bool down, ConsoleKey key, wchar_t chr, int rc)> keyPress, InputFeature withFeature = BOTH) {
	Init();

	evutil_socket_t fd_mouse, fd_keyboard;
	event *mouse_ev, *keyboard_ev, *stdin_ev;
	base = event_base_new();
	if ((withFeature & ONLY_ANSI) == ONLY_ANSI) {
		stdin_ev = event_new(base, fileno(stdin), EV_READ | EV_PERSIST, stdinCallback, nullptr);
		int r = event_add(stdin_ev, 0);
	}
	if ((withFeature & ONLY_DEVICE) == ONLY_DEVICE) {
		fd_mouse = open(mouse_device.c_str(), O_RDONLY | O_NONBLOCK);
		mouse_ev = event_new(base, fd_mouse, EV_READ | EV_PERSIST, mouseDeviceCallback, &mousePress);
		event_add(mouse_ev, 0);
		fd_keyboard = open(keyboard_device.c_str(), O_RDONLY | O_NONBLOCK);
		keyboard_ev = event_new(base, fd_keyboard, EV_READ | EV_PERSIST, keyboardDeviceCallback, &keyPress);
		event_add(keyboard_ev, 0);
	}
	std::atexit([]() { event_base_loopbreak(base); });
	// 循环
	event_base_dispatch(base);
	// 资源回收
	if ((withFeature & ONLY_ANSI) == ONLY_ANSI) {
		event_free(stdin_ev);
		event_base_free(base);
	}
	if ((withFeature & ONLY_DEVICE) == ONLY_DEVICE) {
		event_free(mouse_ev);
		event_free(keyboard_ev);
		event_base_free(base);
		close(fd_mouse);
		close(fd_keyboard);
	}
}


#endif