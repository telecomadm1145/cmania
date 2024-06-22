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

	static inline MouseCallback* mcb;
	static inline KeyboardCallback* kcb;
	static inline bool withMouseKeyEvent, withWheelKeyEvent, withKeyboardKeyEvent = false;
	static void sendOutEvent() {
		if (withMouseKeyEvent) {
			(*mcb)(meka.X, meka.Y, meka.MouseButton, meka.Pressed, false, WheelEventArgs::Direction(0));
			withMouseKeyEvent = false;
		}
		if (withKeyboardKeyEvent) {
			(*kcb)(kea.KeyState, kea.Pressed, kea.Key, kea.UnicodeChar, kea.RepeatCount);
			withKeyboardKeyEvent = false;
		}
		if (withWheelKeyEvent) {
			(*mcb)(0, 0, MouseKeyEventArgs::Button(0), false, true, wea.WheelDirection);
			withWheelKeyEvent = false;
		}
	}



	// 解析ansi鼠标输入的
	class mouseMatcher : public Matcher {
	public:
		bool solve(char nowchar) override {

		loop:
			enum class State {
				AssumeMouseEvent,
				MouseEventParse,
				BadEvent,
				InititalState,
				Transparent
			};
			static int flag = 0;
			static State state = State::InititalState;
			static std::array<char, 32> bufForMatcher = {};
			//[DEBUG]
			// for(auto i: bufForMatcher) {
			// 	std::cout << i<<" "<<std::flush;
			// }
			char prefix[] = { '\033', '[', '<' };
			switch (state) {
			case State::AssumeMouseEvent: {
				bufForMatcher[flag] = nowchar;
				flag++;
				if (nowchar == 'M' || nowchar == 'm') {
					state = State::MouseEventParse;
					goto loop;
				}
				return false;
			}
			case State::MouseEventParse: {

				auto s = std::string(bufForMatcher.begin() + 3, bufForMatcher.begin() + flag - 1);
				auto res = split(s, ';');
				int keyType, x, y = 0;
				bool pressed = bufForMatcher[flag - 1] == 'M';

				// 解析一下字符串
				try {
					keyType = std::stoi(res[0]);
					x = std::stoi(res[1]);
					y = std::stoi(res[2]);
				}
				catch (...) {
					state = State::BadEvent;
					goto loop;
				}
				switch (keyType) {
				// 先把鼠标滚轮的两个写了
				case 64: {
					wea.Delta = 120;
					withWheelKeyEvent = true;
					break;
				}
				case 65: {
					wea.Delta = -120;
					withWheelKeyEvent = true;
					break;
				}
				// 鼠标移动
				case 35: {
					meka.X = x;
					meka.Y=y;
					meka.MouseButton=bufForMatcher[31]==1?MouseKeyEventArgs::Button::Left:MouseKeyEventArgs::Button(0);
					meka.Pressed=false;
					withMouseKeyEvent = true;
					break;
				}
				//按着移动
				case 32:{
					meka.X = x;
					meka.Y=y;
					meka.MouseButton=bufForMatcher[31]==1?MouseKeyEventArgs::Button::Left:MouseKeyEventArgs::Button(0);
					meka.Pressed=true;
					withMouseKeyEvent = true;
					break;
				}
				// 鼠标点击
				// 左键
				case 0: {
					meka.X = x;
					meka.Y = y;
					meka.MouseButton = MouseKeyEventArgs::Button::Left;
					meka.Pressed = pressed;
					bufForMatcher[31]=pressed?1:0;
					withMouseKeyEvent = true;
					break;
				}
				// 中键
				case 1: {
					meka.X = x;
					meka.Y = y;
					meka.MouseButton = MouseKeyEventArgs::Button::Middle;
					meka.Pressed = pressed;
					withMouseKeyEvent = true;
					break;
				}
				// 右键
				case 2: {
					meka.X = x;
					meka.Y = y;
					meka.MouseButton = MouseKeyEventArgs::Button::Right;
					meka.Pressed = pressed;
					withMouseKeyEvent = true;
					break;
				}
				default: {
				}
				}
				bufForMatcher = {};
				flag = 0;
				state = State::InititalState;
				return false;
			}
			case State::BadEvent: {
				// 那么把之前拦截下来的字符重新丢到后面去
				state = State::Transparent;
				for (int i = 0; i < flag; i++) {
					stdinSolvePipeline::Solve(bufForMatcher[i]);
				}
				flag = 0;
				state = State::InititalState;
				return false;
			}
			case State::Transparent: {
				return true;
			}

			case State::InititalState: {
				if (flag == 0 && nowchar != '\033') {
					return true;
				}
				if (flag < 3) {
					if (nowchar == prefix[flag]) {
						bufForMatcher[flag] = nowchar;
						flag++;
						return false;
					}
					else if (nowchar != prefix[flag]) {
						bufForMatcher[flag] = nowchar;
						flag++;
						state = State::BadEvent;
						goto loop;
					}
				}
				else if (flag == 3) {
					state = State::AssumeMouseEvent;
					bufForMatcher[flag] = nowchar;
					flag++;
					return false;
				}
				break;
			}
			}
			return true;
		}
	};
	// 检测ansi键盘输入的
	class keyMatcher : public Matcher {
	public:
		bool solve(char nowchar) override {
			// 字母
			if (isalnum(nowchar) != 0) {
				kea.Key = keyMap[ANSItoKey[nowchar]];
				kea.UnicodeChar = nowchar;
				kea.KeyState = ControlKeyState(0);
				kea.RepeatCount = 0;
				kea.Pressed = true;
				withKeyboardKeyEvent = true;
			}
			// 回车
			else if (nowchar == '\n') {
				kea.Key = ConsoleKey::Enter;
				kea.UnicodeChar = nowchar;
				kea.KeyState = ControlKeyState(0);
				kea.RepeatCount = 0;
				kea.Pressed = true;
				withKeyboardKeyEvent = true;
			} // 上下左右

			else if (nowchar == 65) {
				kea.Key = ConsoleKey::UpArrow;
				kea.UnicodeChar = nowchar;
				kea.KeyState = ControlKeyState(0);
				kea.RepeatCount = 0;
				kea.Pressed = true;
				withKeyboardKeyEvent = true;
			}
			else if (nowchar == 66) {
				kea.Key = ConsoleKey::DownArrow;
				kea.UnicodeChar = nowchar;
				kea.KeyState = ControlKeyState(0);
				kea.RepeatCount = 0;
				kea.Pressed = true;
				withKeyboardKeyEvent = true;
			}
			else if (nowchar == 67) {
				kea.Key = ConsoleKey::RightArrow;
				kea.UnicodeChar = nowchar;
				kea.KeyState = ControlKeyState(0);
				kea.RepeatCount = 0;
				kea.Pressed = true;
				withKeyboardKeyEvent = true;
			}
			else if (nowchar == 68) {
				kea.Key = ConsoleKey::LeftArrow;
				kea.UnicodeChar = nowchar;
				kea.KeyState = ControlKeyState(0);
				kea.RepeatCount = 0;
				kea.Pressed = true;
				withKeyboardKeyEvent = true;
			}
			//退格
			else if (nowchar == 127) {
				kea.Key = ConsoleKey::Backspace;
				kea.UnicodeChar = nowchar;
				kea.KeyState = ControlKeyState(0);
				kea.RepeatCount = 0;
				kea.Pressed = true;
				withKeyboardKeyEvent = true;
			}
			
			return true;
		}
	};
	// 解析键盘输入的方向键的
	class arrowMatcher : public Matcher {
	public:
		bool solve(char nowchar) override {
			static bool transparent = false;
			static int flag = 0;
			if (transparent) {
				goto transparent;
			}
			if (flag == 2) {
				bool pass = true;
				kea = KeyEventArgs(0, true, 0, 0, 0);
				switch (nowchar) {
				case 'A': {
					kea.Key = ConsoleKey::UpArrow;
					break;
				}
				case 'B': {
					kea.Key = ConsoleKey::DownArrow;
					break;
				}
				case 'C': {

					kea.Key = ConsoleKey::RightArrow;
					break;
				}
				case 'D': {

					kea.Key = ConsoleKey::LeftArrow;
					break;
				}
				default: {
					pass = false;
					flag = 0;

					transparent = true;
					stdinSolvePipeline::Solve('\033');
					stdinSolvePipeline::Solve('[');
					transparent = false;
					return true;
				}
				}
				flag = 0;
				withKeyboardKeyEvent = pass ? true : false;
				return false;
			}
			if ((flag == 0 && nowchar == '\033') || (flag == 1 && nowchar == '[')) {
				flag++;
				return false;
			}
			else {
				if (flag == 1) {
					transparent = true;
					stdinSolvePipeline::Solve('\033');
					transparent = false;
					flag = 0;
					return true;
				}
				return true;
			}

		transparent:
			return true;
		}
	};
	static mouseMatcher mM;
	static keyMatcher kM;
	static arrowMatcher aM;
	// 注意matchers的顺序哦
	static inline std::vector<Matcher*> matchers = { &aM, &mM, &kM };

public:
	static void setCallBack();
	static void Solve(char nowchar) {
		for (auto& matcher : matchers) {
			if (!matcher->solve(nowchar)) {
				break;
			}
		}
		sendOutEvent();
	}
	static void SetCallback(MouseCallback* mcb, KeyboardCallback* kcb);
};


extern bool callbackSet;
void stdinCallback(evutil_socket_t fd, short event, void* arg);
// TODO: 增加合适的错误处理
extern event_base* base;
void EventLoop(MouseCallback mousePress, KeyboardCallback keyPressFromStdin, KeyboardCallback keyPressFromDevice, InputFeature withFeature = BOTH);


#endif