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
// class LinuxInput {
// private:
// 	static inline std::once_flag flag=std::once_flag();
// 	static void enableTerminalMouse(){
// 		write(STDOUT_FILENO, "\e[?1000h", 8);
// 	}
// 	static void disableTerminalMouse(){
// 		write(STDOUT_FILENO, "\e[?1000l", 8);
// 	}
// 	static void Init() {
// 		enableTerminalMouse();
// 	}

// public:
// 	LinuxInput() {
// 		std::call_once(LinuxInput::flag, LinuxInput::Init);
// 	}
// 	~LinuxInput() {
// 		disableTerminalMouse();
// 	}
// };
#define EVENT_POOL_SIZE 1024
class InputProcessor {


public:
	enum class EventType {
		Keyboard_Press,
		Keyboard_Release,
		Mouse_Press,
		Mouse_Release,

	};
	struct Event {
		EventType type;
		wchar_t C;
		int x, y;
		int mouse_botton;
	};
	enum Feature {
		ONLY_ANSI = 0b00000001,
		ONLY_DEVICE = 0b00000010,
		BOTH = 0b00000100,
	};
	static std::size_t Process(char* c_in, int len, Event*& eventpool) {
		char* c = buffer_enabled ? [&]() -> char* {
			char* temp = new char[len + buffer_used_len];
			memcpy(temp, buffer, buffer_used_len);
			memcpy(temp + buffer_used_len, c_in, len);
			return temp;
		}()
			: c_in;
		int remain = len;
		int event_pool_flag = 0;
	loop:
		// 如果是缓冲区已经处理完毕，那么delete掉
		if (buffer_used_len == remain) {
			delete[] c;
			buffer_used_len = 0;
			buffer_enabled = false;
		}
		if (remain != 1)
			return 0;
		if (remain >= 1) {
			// 按照utf8的处理模式
			int char_len = (c[0] & 0x80) == 0 ? 1 : (c[0] & 0xE0) == 0xC0 ? 2
												: (c[0] & 0xF0) == 0xE0	  ? 3
																		  : 4;
			if (char_len == 0)
				exit(0); // TODO:what the hell?
			// 如果长度不够
			if (remain < char_len)
				goto fuck;
			remain -= char_len;
			// 如果是一个字符
			if (char_len == 1) {
				eventpool[event_pool_flag].type = EventType::Keyboard_Press;
				memcpy(&eventpool[event_pool_flag].C, c, char_len);
				event_pool_flag++;
				// 如果还有剩余字符
				if (remain) {
					goto loop;
				}
			}
			// 如果是一个字符
		}
		// 处理控制字符
		if (c[0] == '\e') {
			if (c[1] == '[') {
				if (c[2] == 'A') {
					eventpool[event_pool_flag].type = EventType::Mouse_Press;
					eventpool[event_pool_flag].mouse_botton = c[3] - 32;
					eventpool[event_pool_flag].x = c[4] - 32;
					eventpool[event_pool_flag].y = c[5] - 32;
					return event_pool_flag;
				}
			}
		}
	// 后面都是一无所获
	fuck:
		// 那么先放进缓冲区
		if (buffer_enabled && buffer_used_len >= 4)
			throw std::runtime_error("console解析出现问题了，部分字符不符合任意的utf8");
		if (buffer_enabled) {
			// 把新的复制到后面，然后len加上
			memcpy(buffer + buffer_used_len, c + (len - remain), remain);
			buffer_used_len += remain;
			return 0;
		}
		memcpy(buffer, c + (len - remain), remain);
		return 0;
	}
	static void SetDeviceFileDescriptor(int fd) {
	}
	static void EnableFeature(Feature f) {
		switch ((int)f) {
		case (int)Feature::ONLY_ANSI:
			break;
		case (int)Feature::ONLY_DEVICE:
			if (InputProcessor::device == 0) {
			}
			break;
		case (int)Feature::BOTH:
			break;
		default:
			break;
		}
	}

private:
	static inline int device = 0;
	static inline std::array<Event, EVENT_POOL_SIZE> eventpool;
	static inline std::once_flag flag = std::once_flag();
	static inline std::size_t buffer_used_len = 0;
	static inline char buffer[1024] = {};
	static inline Feature feature = Feature::ONLY_ANSI;
	static inline bool buffer_enabled = false;
};
class LinuxSignalHandler {
public:
	LinuxSignalHandler() {
		std::call_once(LinuxSignalHandler::flag, LinuxSignalHandler::Init);
	}
	void RegisterSIGWINCH(std::function<void(int)> func) {
		func_for_sigwinch = func;
	}

private:
	// TODO:使用模板搞成编译器静态数组可以优化性能
	static inline std::function<void(int)> func_for_sigwinch;
	static void Init() {
		signal(SIGWINCH, SIGWINCHHandler);
	}

	std::once_flag flag;
	static void SIGWINCHHandler(int signum) {
		func_for_sigwinch(signum);
	}
};
class LinuxConsoleComponent : public GameComponent {
private:
	std::thread* input_thread = nullptr;
	static inline fd_set fds;
	struct termios oldsetting, newsetting;
	void ProcessEvent(const char* evt, const void* evtargs) override {
		if (strcmp(evt, "start") == 0) {
			// 设置终端为非规范模式
			tcgetattr(fileno(stdin), &oldsetting);
			newsetting = oldsetting;
			newsetting.c_lflag &= ~(ICANON | ECHO);
			tcsetattr(fileno(stdin), TCSANOW, &newsetting);
			FD_ZERO(&fds);
			FD_SET(fileno(stdin), &fds);
			input_thread = new std::thread(&InputWorker, parent);
			input_thread->detach();
			// 事件
			LinuxSignalHandler lsh{};
			lsh.RegisterSIGWINCH([this](int signum) {
				SendResize(this->parent);
			});
		}

		else if (strcmp(evt, "push") == 0) {
			struct PushEventArgs {
				const char* buf;
				size_t len;
			};
			auto pea = *(PushEventArgs*)evtargs;
			write(fileno(stdin), pea.buf, pea.len);
		}
		else if (strcmp(evt, "fresize") == 0) {
			SendResize(parent);
		}
	}
	static void InputWorker(Game* parent) {
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 5000;
		while (1) {
			int res = 0;
			res = select(1, &fds, NULL, NULL, &tv);

			if (res < 0) {
				throw std::runtime_error("select failed");
			}
			else if (res == 0) {
				continue;
			}
			else {
				if (FD_ISSET(fileno(stdin), &fds)) {
					char buf[1024];
					int len = read(fileno(stdin), buf, 1024);
					if (len > 0) {
						InputProcessor::Event* e;
						int i = InputProcessor::Process(buf, len, e);
						if (i > 0) {
							// 遍历每一个事件
							for (int now = 0; now < i; now++) {
								switch (e[now].type) {
								case InputProcessor::EventType::Mouse_Press:
									// MouseKeyEventArgs meka(e[now].x, e[now].y, 1);
									// parent->Raise("mousekey", meka);
									// break;
								case InputProcessor::EventType::Keyboard_Press:
									KeyEventArgs kea(0, true, 0, e[now].C, 0);
									parent->Raise("key", kea);
									break;

								}
							}
						}
					}
				}
			}
		}
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
		tcsetattr(fileno(stdin), TCSANOW, &oldsetting);
		delete input_thread;
	}
};
GameComponent* MakeLinuxConsoleComponent() {
	return new LinuxConsoleComponent();
}