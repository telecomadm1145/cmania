#include "LinuxInputHelper.hpp"
stdinSolvePipeline::mouseMatcher stdinSolvePipeline::mM;
stdinSolvePipeline::keyMatcher stdinSolvePipeline::kM;
stdinSolvePipeline::arrowMatcher stdinSolvePipeline::aM;
void Init() {
	static bool hasRunned = false;
	if (!hasRunned) {
		hasRunned = true;
		// TODO:使用一个界面来矫正鼠标和键盘设备
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
	{ KEY_TAB, ConsoleKey::Tab }, { KEY_0, ConsoleKey::D0 }, { KEY_1, ConsoleKey::D1 },
	{ KEY_2, ConsoleKey::D2 }, { KEY_3, ConsoleKey::D3 }, { KEY_4, ConsoleKey::D4 },
	{ KEY_5, ConsoleKey::D5 }, { KEY_6, ConsoleKey::D6 }, { KEY_7, ConsoleKey::D7 },
	{ KEY_8, ConsoleKey::D8 }, { KEY_9, ConsoleKey::D9 }
};
std::unordered_map<int, char> keyToANSI = {
	{ KEY_A, 'a' },
	{ KEY_B, 'b' },
	{ KEY_C, 'c' },
	{ KEY_D, 'd' },
	{ KEY_E, 'e' },
	{ KEY_F, 'f' },
	{ KEY_G, 'g' },
	{ KEY_H, 'h' },
	{ KEY_I, 'i' },
	{ KEY_J, 'j' },
	{ KEY_K, 'k' },
	{ KEY_L, 'l' },
	{ KEY_M, 'm' },
	{ KEY_N, 'n' },
	{ KEY_O, 'o' },
	{ KEY_P, 'p' },
	{ KEY_Q, 'q' },
	{ KEY_R, 'r' },
	{ KEY_S, 's' },
	{ KEY_T, 't' },
	{ KEY_U, 'u' },
	{ KEY_V, 'v' },
	{ KEY_W, 'w' },
	{ KEY_X, 'x' },
	{ KEY_Y, 'y' },
	{ KEY_Z, 'z' },
	{ KEY_0, '0' },
	{ KEY_1, '1' },
	{ KEY_2, '2' },
	{ KEY_3, '3' },
	{ KEY_4, '4' },
	{ KEY_5, '5' },
	{ KEY_6, '6' },
	{ KEY_7, '7' },
	{ KEY_8, '8' },
	{ KEY_9, '9' }
};
std::unordered_map<int, char> ANSItoKey = {
	{ 'a', KEY_A },
	{ 'b', KEY_B },
	{ 'c', KEY_C },
	{ 'd', KEY_D },
	{ 'e', KEY_E },
	{ 'f', KEY_F },
	{ 'g', KEY_G },
	{ 'h', KEY_H },
	{ 'i', KEY_I },
	{ 'j', KEY_J },
	{ 'k', KEY_K },
	{ 'l', KEY_L },
	{ 'm', KEY_M },
	{ 'n', KEY_N },
	{ 'o', KEY_O },
	{ 'p', KEY_P },
	{ 'q', KEY_Q },
	{ 'r', KEY_R },
	{ 's', KEY_S },
	{ 't', KEY_T },
	{ 'u', KEY_U },
	{ 'v', KEY_V },
	{ 'w', KEY_W },
	{ 'x', KEY_X },
	{ 'y', KEY_Y },
	{ 'z', KEY_Z },
	{ '0', KEY_0 },
	{ '1', KEY_1 },
	{ '2', KEY_2 },
	{ '3', KEY_3 },
	{ '4', KEY_4 },
	{ '5', KEY_5 },
	{ '6', KEY_6 },
	{ '7', KEY_7 },
	{ '8', KEY_8 },
	{ '9', KEY_9 }
};
std::string keyboard_device;
void stdinSolvePipeline::SetCallback(MouseCallback* mcb, KeyboardCallback* kcb) {
	stdinSolvePipeline::mcb = mcb;
	stdinSolvePipeline::kcb = kcb;
}
void EventLoop(MouseCallback mousePress, KeyboardCallback keyPressFromStdin, KeyboardCallback keyPressFromDevice, InputFeature withFeature) {

	Init();
	evutil_socket_t fd_keyboard;
	event *keyboard_ev, *stdin_ev;
	base = event_base_new();
	if ((withFeature & ONLY_ANSI) == ONLY_ANSI) {
		std::tuple<MouseCallback*, KeyboardCallback*> t(&mousePress, &keyPressFromStdin);
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
event_base* base;
bool callbackSet = false;
void stdinCallback(evutil_socket_t fd, short event, void* arg) {
	if (!callbackSet) {
		auto t = (std::tuple<MouseCallback*, KeyboardCallback*>*)arg;
		callbackSet = true;
		stdinSolvePipeline::SetCallback(std::get<0>(*t), std::get<1>(*t));
	}
	char t;
	while (read(fd, &t, 1) > 0) {
		stdinSolvePipeline::Solve(t);
	}
}
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