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
#include <csignal>
#include <string>
#include "String.h"
// 初始化

extern std::string keyboard_device;
typedef std::function<void(int x, int y, MouseKeyEventArgs::Button b, bool pressed, bool wheel, WheelEventArgs::Direction wd)> MouseCallback;
typedef std::function<void(ControlKeyState cks, bool down, ConsoleKey key, wchar_t chr, int rc)> KeyboardCallback;
enum InputFeature {
	ONLY_ANSI = 0b00000001,
	ONLY_DEVICE = 0b00000010,
	BOTH = 0b00000011,
};
void Init();
extern std::unordered_map<int, ConsoleKey> keyMap;
extern std::unordered_map<int, char> keyToANSI;
extern std::unordered_map<int, char> ANSItoKey;

void keyboardDeviceCallback(evutil_socket_t fd, short event, void* arg);
// 用来辅助匹配stdin输入的类
class stdinSolvePipeline {
private:
	class Matcher {
	public:
		virtual bool solve(char nowchar) = 0;
	};

	static inline MouseKeyEventArgs meka = MouseKeyEventArgs(0, 0, MouseKeyEventArgs::Button(0), false);
	static inline WheelEventArgs wea;
	static inline KeyEventArgs kea = KeyEventArgs(0, 0, 0, 0, 0);
	/*
	关于publicBuf的约定
	[0]: 1->考虑鼠标事件 不作为键盘事件
	 */
	static inline std::array<int, 16> publicBuf;

	static inline MouseCallback mcb;
	static inline KeyboardCallback kcb;
	static inline bool withMouseKeyEvent, withWheelKeyEvent, withKeyboardKeyEvent = false;
	static void sendOutEvent() {
		if (withMouseKeyEvent) {
			mcb(meka.X, meka.Y, meka.MouseButton, meka.Pressed, false, WheelEventArgs::Direction(0));
		}
		if (withKeyboardKeyEvent) {
			kcb(kea.KeyState, kea.Pressed, kea.Key, kea.UnicodeChar, kea.RepeatCount);
		}
		if (withWheelKeyEvent) {
			mcb(0, 0, MouseKeyEventArgs::Button(0), false, true, wea.WheelDirection);
		}
	}



	// 解析ansi鼠标输入的
	class mouseMatcher : public Matcher {
	public:
		bool solve(char nowchar) override {
			static int flag = 0;
			return true;
			// static std::array<int,32> bufForMatcher = {};
			// char prefix[] = { '\033', '[', '<' };
			//  if (nowchar == prefix[flag] && flag < 3) {
			//  	flag++;
			//  	return false;
			//  }
			//  if (nowchar != prefix[flag] && flag < 3) {
			//  	flag = 0;
			//  	return;
			//  }
			//  else if (flag >= 3 && nowchar == prefix[0]) {
			//  	flag = 0;
			//  	return;
			//  	// TODO:如果没有切换到utf8回报，那么要做好读取老式binary回报的准备
			//  }
			//  else if (flag == 0) {
			//  	return;
			//  }
			//  // 不考虑序列不完整的情况了
			//  // 如果能执行到这，代表下面都是鼠标序列了
			//  //  flag在3-11之间，全直接push到buf
			//  bufForMatcher[flag - 3] = nowchar;
			//  flag++;
			//  // 字符为'M'或'm'即为结束
			//  if (nowchar == 'M' || nowchar == 'm') {
			//  	// 解析
			//  	// MouseKeyEventArgs mea(x, y, MouseKeyEventArgs::Button::Left, true);
			//  	flag = 0;

			// 	// 按照;分割
			// 	auto j = std::string(bufForMatcher.begin(), bufForMatcher.end());
			// 	auto res = split(j, ';');
			// 	std::cout << std::endl
			// 			  << res[0] << " " << res[1] << " " << res[2] << std::endl;
			// 	int x, y = 0;
			// 	try {
			// 		x = std::stoi(res[1]);
			// 		y = std::stoi(res[2].substr(0, res[2].size() - 1));
			// 	}
			// 	catch (std::exception e) {
			// 		std::cerr << e.what() << std::endl;
			// 		flag = 0;
			// 		return false;
			// 	}
			// 	bool isWheelEvent = false;
			// 	bool pressed = nowchar == 'M';
			// 	MouseKeyEventArgs::Button b;
			// 	bool wheelUp = true;
			// 	switch (std::stoi(res[0])) {
			// 	case 0:
			// 		b = MouseKeyEventArgs::Button::Left;
			// 		// 标记一下左键按下过了
			// 		// TODO:这里只支持一下左键的长按拖拽哦
			// 		if (pressed) {
			// 			bufForMatcher[31] = 1;
			// 		}
			// 		else {
			// 			bufForMatcher[31] = 0;
			// 		};
			// 		break;
			// 	case 1:
			// 		b = MouseKeyEventArgs::Button::Middle;
			// 		break;
			// 	case 2:
			// 		b = MouseKeyEventArgs::Button::Right;
			// 		break;
			// 	case 35:
			// 		if (bufForMatcher[31] == 1) {
			// 			b = MouseKeyEventArgs::Button::Left;
			// 			pressed = true;
			// 		}
			// 		else {
			// 			b = MouseKeyEventArgs::Button(0);
			// 			pressed = false;
			// 		}
			// 		break;
			// 	case 64:
			// 		isWheelEvent = true;
			// 		wheelUp = true;
			// 		break;
			// 	case 65:
			// 		isWheelEvent = true;
			// 		wheelUp = false;
			// 		break;
			// 	default:
			// 		break;
			// 	}
			// 	bufForMatcher = { 0 };
		}
	};
	// 检测ansi键盘输入的
	class keyMatcher : public Matcher {
	public:
		bool solve(char nowchar) override {
			// 遇到不能处理的情况都return
			// 遇到可以匹配上的则设置事件，设置事件flag
			if (publicBuf[0] == 1) {
				return true;
			}
			// 字母
			if (isalnum(nowchar)) {
				kea.Key = keyMap[ANSItoKey[nowchar]];
				kea.UnicodeChar = nowchar;
				kea.KeyState = ControlKeyState(0);
				kea.RepeatCount = 0;
				kea.Pressed = true;
			}
			withKeyboardKeyEvent = true;
			return true;
		}
	};

	static mouseMatcher mM;
	static keyMatcher kM;
	// 注意matchers的顺序哦
	static inline std::vector<Matcher*> matchers = {&mM,&kM};

public:
	static void setCallBack();
	static void Solve(char nowchar) {
		for (auto& matcher : matchers) {
			if (matcher->solve(nowchar)) {
				break;
			}
		}
		sendOutEvent();
	}
	static void SetCallback(MouseCallback mcb, KeyboardCallback kcb);
};


extern bool callbackSet;
void stdinCallback(evutil_socket_t fd, short event, void* arg);
// TODO: 增加合适的错误处理
extern event_base* base;
void EventLoop(MouseCallback mousePress, KeyboardCallback keyPressFromStdin, KeyboardCallback keyPressFromDevice, InputFeature withFeature = BOTH);


#endif