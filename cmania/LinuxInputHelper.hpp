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
typedef std::function<void(int x, int y, MouseKeyEventArgs::Button b, bool pressed)> MouseCallback;
typedef std::function<void(ControlKeyState cks, bool down, ConsoleKey key, wchar_t chr, int rc)> KeyboardCallback;
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
std::unordered_map<int, char> ANSItoKey = {
	{ 'a', KEY_A }, { 'b', KEY_B }, { 'c', KEY_C }, { 'd', KEY_D }, { 'e', KEY_E }, { 'f', KEY_F },
	{ 'g', KEY_G }, { 'h', KEY_H }, { 'i', KEY_I }, { 'j', KEY_J }, { 'k', KEY_K }, { 'l', KEY_L },
	{ 'm', KEY_M }, { 'n', KEY_N }, { 'o', KEY_O }, { 'p', KEY_P }, { 'q', KEY_Q }, { 'r', KEY_R },
	{ 's', KEY_S }, { 't', KEY_T }, { 'u', KEY_U }, { 'v', KEY_V }, { 'w', KEY_W }, { 'x', KEY_X },
	{ 'y', KEY_Y }, { 'z', KEY_Z }
};

void keyboardDeviceCallback(evutil_socket_t fd, short event, void* arg) {
	// struct input_event ev;
	// auto* _kp = (std::function<void(ControlKeyState cks, bool down, ConsoleKey key, wchar_t chr, int rc)>*)arg;
	// auto kp = *_kp;
	// while (read(fd, &ev, sizeof(ev)) > 0) {
	// 	if (ev.type == EV_KEY) {
	// 		kp(ControlKeyState(0), ev.value == 1, keyMap[ev.code], keyToANSI[ev.code], 0);
	// 	}
	// }
}
// 用来辅助匹配stdin输入的类
class stdinHelper {
private:
	std::unordered_map<int, int> flagsOfEachMatcher = {};
	std::unordered_map<int, std::array<int, 128>> bufOfEachMatcher = {};
	std::vector<std::function<void(char now_char, std::array<int, 128>& bufForMatcher, int& flag, void* arg)>> matchers;

public:
	// 第一个函数是去匹配的函数，第二个是匹配到了之后要call的函数
	// call_back的args是用来传递一些杂数据的
	// 憨包看我的超级无敌warpper搞的参数传递
	void addMatcher(std::function<void(char, std::array<int, 128>& bufForMatcher, int& flag, std::function<void(std::string infomation, std::any body)>)> matcher, std::function<void(std::string infomation, std::any body, void* args)> call_back) {
		auto wrapper = [=](std::string infomation, std::any body, void* args) {
			call_back(infomation, body, args);
		};
		auto f = [=](char nowchar, std::array<int, 128>& bufForMatcher, int& flag, void* arg) {
			matcher(nowchar, bufForMatcher, flag, std::bind(wrapper, std::placeholders::_1, std::placeholders::_2, arg));
		};
		matchers.push_back(f);
		int number = matchers.size() - 1;
		flagsOfEachMatcher[number] = 0;
		bufOfEachMatcher[number] = { 0 };
	}
	void addToMatchBuffer(char nowchar, void* arg) {
		for (int i = 0; i < matchers.size(); i++) {
			auto f = matchers[i];
			f(nowchar, bufOfEachMatcher[i], flagsOfEachMatcher[i], arg);
		}
	}
};
stdinHelper stdinHelperInstance;

void initMatcherForStdin() {
	stdinHelperInstance.addMatcher([](char nowchar, std::array<int, 128>& bufForMatcher, int& flag, std::function<void(std::string infomation, std::any body)> call_back) {
		//[DEBUG]输出现在的字符的10进制和ansi
		//std::cout<<int(nowchar)<<" "<<nowchar<<" 	"<<"flag = "<<flag<<"   "<<std::flush;
		// 检测鼠标回报ansi序列的前缀
		char prefix[] = { '\033', '[', '<' };
		if (nowchar == prefix[flag] && flag < 3) {
			flag++;
			return;
		}if(nowchar!=prefix[flag]&& flag<3){
			flag=0;
			return;
			
		}
		else if(flag>=3&&nowchar==prefix[0]){
			flag = 0;
			return;
			// TODO:如果没有切换到utf8回报，那么要做好读取老式binary回报的准备
		}else if(flag==0){
			return;
		}
		// 不考虑序列不完整的情况了
		//如果能执行到这，代表下面都是鼠标序列了
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
			//std::cout<<std::endl<<res[0]<<" "<<res[1]<<" "<<res[2]<<std::endl;
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
			call_back("", mea);
			bufForMatcher = { 0 };
		} },
		[](std::string infomation, std::any body, void* args) {
			std::tuple<MouseCallback, KeyboardCallback>* t = (std::tuple<MouseCallback, KeyboardCallback>*)args;
			auto mousePress = std::get<0>(*t);
			auto mkea = std::any_cast<MouseKeyEventArgs>(body);
			mousePress(mkea.X, mkea.Y, mkea.MouseButton, mkea.Pressed);
		});
	// 检测ansi键盘输入的
	stdinHelperInstance.addMatcher([](char nowchar, std::array<int, 128>& bufForMatcher, int& flag, std::function<void(std::string infomation, std::any body)> call_back) {
		// 暂时只读取字母
		if ((nowchar >= 'a' && nowchar <= 'z') || (nowchar >= 'A' && nowchar <= 'Z')) {
			wchar_t w = nowchar;
			KeyEventArgs kea(0,true,ANSItoKey[std::tolower(nowchar)],w,0);
			call_back("", kea);
		}
		
	},
		[](std::string infomation, std::any body, void* args) {
			std::tuple<MouseCallback, KeyboardCallback>* t = (std::tuple<MouseCallback, KeyboardCallback>*)args;
			auto keyPress = std::get<1>(*t);
			auto kea= std::any_cast<KeyEventArgs>(body);
			keyPress(kea.KeyState, kea.Pressed, kea.Key, kea.UnicodeChar, kea.RepeatCount);

		});
}
bool stdinMatcherInited = false;
void stdinCallback(evutil_socket_t fd, short event, void* arg) {
	if (!stdinMatcherInited) {
		initMatcherForStdin();
		stdinMatcherInited = true;
	}
	char buf = 0;
	while (read(fd, &buf, 1) > 0) {
		stdinHelperInstance.addToMatchBuffer(buf, arg);
	}
}
// TODO: 增加合适的错误处理
event_base* base;
void EventLoop(MouseCallback mousePress, KeyboardCallback keyPressFromStdin, KeyboardCallback keyPressFromDevice, InputFeature withFeature = BOTH) {
	Init();
	evutil_socket_t fd_keyboard;
	event *keyboard_ev, *stdin_ev;
	base = event_base_new();
	if ((withFeature & ONLY_ANSI) == ONLY_ANSI) {
		std::tuple<MouseCallback, KeyboardCallback> t(mousePress, keyPressFromStdin);
		stdin_ev = event_new(base, fileno(stdin), EV_READ | EV_PERSIST, stdinCallback, &t);
		int r = event_add(stdin_ev, 0);
	}
	if ((withFeature & ONLY_DEVICE) == ONLY_DEVICE) {
		fd_keyboard = open(keyboard_device.c_str(), O_RDONLY | O_NONBLOCK);
		keyboard_ev = event_new(base, fd_keyboard, EV_READ | EV_PERSIST, keyboardDeviceCallback, &keyPressFromDevice);
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
		event_free(keyboard_ev);
		event_base_free(base);
		close(fd_keyboard);
	}
}


#endif