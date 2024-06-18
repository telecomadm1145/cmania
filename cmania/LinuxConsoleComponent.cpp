#include "ConsoleInput.h"
#include "Game.h"
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <functional>
#include <iostream>
#include <mutex>
#include <optional>
#include <sys/select.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <thread>
#include "signal.h"
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <iostream>
#include <ranges>
#include <codecvt>
#include <sys/capability.h>
#include <locale>
#include "LinuxInputHelper.hpp"
void test() {
}
class LinuxSignalHandler {
public:
	LinuxSignalHandler() {
		std::call_once(LinuxSignalHandler::flag, LinuxSignalHandler::Init);
	}
	void RegisterSIGWINCH(std::function<void(int)> func) {
		func_for_sigwinch = func;
	}
	void RegisterSIGINT(std::function<void(int)> func) {
		func_for_sigint = func;
	}

private:
	// TODO:使用模板搞成编译器静态数组可以优化性能
	static inline std::function<void(int)> func_for_sigwinch;
	static inline std::function<void(int)> func_for_sigint;
	static void Init() {
		// 注册
		signal(SIGWINCH, SIGWINCHHandler);
		signal(SIGINT, SIGINTHandler);
	}

	std::once_flag flag;
	static void SIGWINCHHandler(int signum) {
		func_for_sigwinch(signum);
	}
	static void SIGINTHandler(int signum) {
		func_for_sigint(signum);
	}
};
class LinuxConsoleComponent : public GameComponent {
private:
	std::thread* input_thread = nullptr;
	fd_set fds;
	cap_t caps = cap_get_proc();
	struct termios oldsetting, newsetting;
	void onExit() {
		tcsetattr(fileno(stdin), TCSANOW, &oldsetting);
		write(STDOUT_FILENO, "\e[?1003l\e[?1006l", 24);
		write(STDOUT_FILENO, "\e[?25h", 5);
		cap_free(caps);
	}
	void ProcessEvent(const char* evt, const void* evtargs) override {
		if (strcmp(evt, "start") == 0) {
			// 设置终端为非规范模式
			tcgetattr(fileno(stdin), &oldsetting);
			newsetting = oldsetting;
			newsetting.c_lflag &= ~(ICANON | ECHO);
			tcsetattr(fileno(stdin), TCSANOW, &newsetting);
			// 打开鼠标监听
			// 兼容多种终端
			// TODO:增加对\e[?1015h终端的支持
			write(STDOUT_FILENO, "\e[?1003h\e[?1006h", 24);
			// 启动CAP_SYS_ADMIN，于是可以监听输入

			// 关闭终端光标
			write(STDOUT_FILENO, "\e[?25l", 5);

			cap_value_t cap_sys_admin = CAP_SYS_ADMIN;
			cap_set_flag(caps, CAP_EFFECTIVE, 1, &cap_sys_admin, CAP_SET);
			cap_set_proc(caps);
			// 开始输入事件循环
			input_thread = new std::thread(&InputWorker, parent);
			input_thread->detach();
			// 事件
			LinuxSignalHandler lsh{};
			lsh.RegisterSIGWINCH([this](int signum) {
				SendResize(this->parent);
			});
			lsh.RegisterSIGINT([this](int signum) {
				std::exit(0);
			});
			std::atexit([]() { write(STDOUT_FILENO, "\e[?1003l\e[?1006l", 24); });
		}
		else if (strcmp(evt, "push") == 0) {
			// 将缓冲区中的字符输出到console
			struct PushEventArgs {
				const char* buf;
				size_t len;
			};
			auto pea = *(PushEventArgs*)evtargs;
			write(STDOUT_FILENO, pea.buf, pea.len);
		}
		else if (strcmp(evt, "fresize") == 0) {
			SendResize(parent);
		}
	}
	static void InputWorker(Game* parent) {
		auto mouseEvent = [&](int x, int y, MouseKeyEventArgs::Button b, bool pressed) {
			MouseKeyEventArgs mea(x, y, b, pressed);
			//std::cout<<"x "<<x<<"y "<<y<<"p"<<pressed<<std::endl;
			parent->Raise("mouse", mea);
		};
		auto keyboardStdinEvent = [&](ControlKeyState cks, bool down, ConsoleKey key, wchar_t chr, int rc) {

			KeyEventArgs kea{ (int)cks, down, (int)key, chr, rc };
			parent->Raise("key", kea);
		};
		auto KeyboardDeviceEvent = [](ControlKeyState cks, bool down, ConsoleKey key, wchar_t chr, int rc) {
		};
		EventLoop(mouseEvent, keyboardStdinEvent, KeyboardDeviceEvent);
	}
	static void SendResize(Game* parent) {
		struct winsize size;
		ioctl(STDIN_FILENO, TIOCGWINSZ, &size);
		ResizeEventArgs rea{ size.ws_col, size.ws_row };
		parent->Raise("resize", rea);
	}

public:
	~LinuxConsoleComponent() {
		// 把终端归为原来的设置
		// TODO:如果程序意外终止，这个地方可能不被执行到
		onExit();
		delete input_thread;
	}
};
GameComponent* MakeLinuxConsoleComponent() {
	return new LinuxConsoleComponent();
}