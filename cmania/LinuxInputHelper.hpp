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
#include <any>
#include <linux/input-event-codes.h>
#include <ranges>
#include <string>
#include "String.h"
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
std::unordered_map<int, ConsoleKey> keyMap = {
	{ KEY_A, ConsoleKey::A }, { KEY_B, ConsoleKey::B }, { KEY_C, ConsoleKey::C },
	{ KEY_D, ConsoleKey::D }, { KEY_E, ConsoleKey::E }, { KEY_F, ConsoleKey::F },
	{ KEY_G, ConsoleKey::G }, { KEY_H, ConsoleKey::H }, { KEY_I, ConsoleKey::I },
	{ KEY_J, ConsoleKey::J }, { KEY_K, ConsoleKey::K }, { KEY_L, ConsoleKey::L },
	{ KEY_M, ConsoleKey::M }, { KEY_N, ConsoleKey::N }, { KEY_O, ConsoleKey::O },
	{ KEY_P, ConsoleKey::P }, { KEY_Q, ConsoleKey::Q }, { KEY_R, ConsoleKey::R },
	{ KEY_S, ConsoleKey::S }, { KEY_T, ConsoleKey::T }, { KEY_U, ConsoleKey::U },
	{ KEY_V, ConsoleKey::V }, { KEY_W, ConsoleKey::W }, { KEY_X, ConsoleKey::X },
	{ KEY_Y, ConsoleKey::Y }, { KEY_Z, ConsoleKey::Z }, { KEY_ENTER, ConsoleKey::Enter },
	{ KEY_UP, ConsoleKey::UpArrow }, { KEY_DOWN, ConsoleKey::DownArrow },
	{ KEY_LEFT, ConsoleKey::LeftArrow }, { KEY_RIGHT, ConsoleKey::RightArrow },
	{ KEY_ESC, ConsoleKey::Escape }, { KEY_SPACE, ConsoleKey::Spacebar }, { KEY_BACK, ConsoleKey::Backspace },
	{ KEY_F1, ConsoleKey::F1 }, { KEY_F2, ConsoleKey::F2 }, { KEY_F3, ConsoleKey::F3 },
	{ KEY_F4, ConsoleKey::F4 }, { KEY_F5, ConsoleKey::F5 }, { KEY_F6, ConsoleKey::F6 },
	{ KEY_F7, ConsoleKey::F7 }, { KEY_F8, ConsoleKey::F8 }, { KEY_F9, ConsoleKey::F9 },
	{ KEY_F10, ConsoleKey::F10 }, { KEY_F11, ConsoleKey::F11 }, { KEY_F12, ConsoleKey::F12 },
	{ KEY_TAB, ConsoleKey::Tab }
};
std::unordered_map<int, char> keyToANSI = {
	{ KEY_A, 'a' }, { KEY_B, 'b' }, { KEY_C, 'c' }, { KEY_D, 'd' }, { KEY_E, 'e' }, { KEY_F, 'f' },
	{ KEY_G, 'g' }, { KEY_H, 'h' }, { KEY_I, 'i' }, { KEY_J, 'j' }, { KEY_K, 'k' }, { KEY_L, 'l' },
	{ KEY_M, 'm' }, { KEY_N, 'n' }, { KEY_O, 'o' }, { KEY_P, 'p' }, { KEY_Q, 'q' }, { KEY_R, 'r' },
	{ KEY_S, 's' }, { KEY_T, 't' }, { KEY_U, 'u' }, { KEY_V, 'v' }, { KEY_W, 'w' }, { KEY_X, 'x' },
	{ KEY_Y, 'y' }, { KEY_Z, 'z' }
};
void keyboardDeviceCallback(evutil_socket_t fd, short event, void* arg) {
	struct input_event ev;
	auto* _kp = (std::function<void(ControlKeyState cks, bool down, ConsoleKey key, wchar_t chr, int rc)>*)arg;
	auto kp = *_kp;
	while (read(fd, &ev, sizeof(ev)) > 0) {
		if (ev.type == EV_KEY) {
			kp(ControlKeyState(0), ev.value == 1, keyMap[ev.code], keyToANSI[ev.code], 0);
		}
	}
}
void mouseDeviceCallback(evutil_socket_t fd, short event, void* arg) {
}
class stdinHelper {
private:
	std::unordered_map<int, int> flagsOfEachMatcher = {};
	std::unordered_map<int, std::array<int, 128>> bufOfEachMatcher = {};
	std::vector<std::function<void(char now_char, std::array<int, 128>& bufForMatcher, int& flag)>> matchers;

public:
	void addMatcher(std::function<void(char, std::array<int, 128>& bufForMatcher, int& flag, std::function<void(std::string infomation, std::any body)>)> matcher, std::function<void(std::string infomation, std::any body)> call_back) {
		auto f = std::bind(matcher, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, call_back);
		matchers.push_back(f);
		int number = matchers.size() - 1;
		flagsOfEachMatcher[number] = 0;
		bufOfEachMatcher[number] = { 0 };
	}
	void addToMatchBuffer(char nowchar) {
		for (int i = 0; i < matchers.size(); i++) {
			matchers[i](nowchar, bufOfEachMatcher[i], flagsOfEachMatcher[i]);
		}
	}
};
stdinHelper stdinHelperInstance;


void stdinCallback(evutil_socket_t fd, short event, void* arg) {
	// 鼠标终端报告
	stdinHelperInstance.addMatcher([](char nowchar, std::array<int, 128>& bufForMatcher, int& flag, std::function<void(std::string infomation, std::any body)> call_back) {
		//[DEBUG]输出现在的字符的10进制和ansi
		//std::cout<<int(nowchar)<<" "<<nowchar<<" 	"<<std::flush;
		// 检测鼠标回报ansi序列的前缀
		char prefix[] = { '\033', '[', '<' };
		if (nowchar == prefix[flag] && flag < 3) {
			flag++;
			return;
		}
		else if(flag>=3&&nowchar==prefix[0]){
			flag = 0;
			// TODO:如果没有切换到utf8回报，那么要做好读取老式binary回报的准备
		}
		// 不考虑序列不完整的情况了
		//  flag在3-11之间，全直接push到buf
		bufForMatcher[flag - 3] = nowchar;
		flag++;
		// 字符为'M'或'm'即为结束
		if (nowchar == 'M' || nowchar == 'm') {
			// 解析
			// MouseKeyEventArgs mea(x, y, MouseKeyEventArgs::Button::Left, true);
			flag = 0;
			
			// 按照;分割
			auto j=std::string(bufForMatcher.begin(), bufForMatcher.end());
			auto res=split(j, ';');
			std::cout<<res[0]<<" "<<res[1]<<" "<<res[2]<<std::endl;
			int x=std::stoi(res[1]);
			int y=std::stoi(res[2].substr(0,res[2].size()-1));
			bool pressed=nowchar=='M';
			MouseKeyEventArgs::Button b;
			switch(std::stoi(res[0])){
				case 0:
					b = MouseKeyEventArgs::Button::Left;
					//标记一下左键按下过了
					//TODO:这里只支持一下左键的长按拖拽哦
					if (pressed) {
						bufForMatcher[127] = 1;
					}else{bufForMatcher[127] = 0;};
					break;
				case 1:
					b = MouseKeyEventArgs::Button::Middle;
					break;
				case 2:
					b = MouseKeyEventArgs::Button::Right;
					break;
				case 35:
					if(bufForMatcher[127]==1){
						b = MouseKeyEventArgs::Button::Left;
						pressed = true;
				
					}else{
						b = MouseKeyEventArgs::Button(0);
						pressed = false;
					}
					break;
				default:
					break;
			}

			MouseKeyEventArgs mea(x, y, b, pressed);
			call_back("mouse", mea);
			bufForMatcher = { 0 };
		} },
		[&](std::string infomation, std::any body) {
			auto mkea = std::any_cast<MouseKeyEventArgs>(body);
			auto* _kp = (std::function<void(int x, int y, MouseKeyEventArgs::Button b, bool pressed)>*)arg;
			auto kp = *_kp;
			kp(mkea.X,mkea.Y, mkea.MouseButton, mkea.Pressed);
		});
	char buf = 0;
	while (read(fd, &buf, 1) > 0) {
		stdinHelperInstance.addToMatchBuffer(buf);
	}
}
// TODO: 增加合适的错误处理
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