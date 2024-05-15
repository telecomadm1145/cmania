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
#include <ranges>
#include <codecvt>
#include <locale>

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
	fd_set fds;
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

			int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
			flags |= O_NONBLOCK;
			fcntl(STDIN_FILENO, F_SETFL, flags);

			input_thread = new std::thread(&InputWorker, parent);
			input_thread->detach();
			LinuxSignalHandler lsh{};
			lsh.RegisterSIGWINCH([](int) {

			});
		}

		else if (strcmp(evt, "push") == 0) {
			struct PushEventArgs {
				const char* buf;
				size_t len;
			};
			auto pea = *(PushEventArgs*)evtargs;
			std::cout.write(pea.buf, pea.len);
		}
		else if (strcmp(evt, "fresize") == 0) {
			SendResize(parent);
		}
	}
	static void InputWorker(Game* parent) {
		while (1) {
			// Read a single character
			char c = getchar();
			if (c == EOF) {
				//struct timespec delay;
				//delay.tv_sec = 0;
				//delay.tv_nsec = 1000000; // 1毫秒 = 1,000,000纳秒

				//// 延迟一毫秒
				//nanosleep(&delay, NULL);
				continue;
			}
			auto vk = c;
			if (c == '\n') {
				vk = 13;
			}
			if (c == 127) {
				vk = (int)ConsoleKey::Backspace;
			}
			if (c >= 'a' && c <= 'z') {
				vk = c - 'a' + 'A';
			}
			if (c == '\033') {
				auto d = getchar();
				if (d == '[') {
					char k = getchar();
					if (k == 'A') {
						vk = (int)ConsoleKey::UpArrow;
					}
					else if (k == 'B') {
						vk = (int)ConsoleKey::DownArrow;
					}
					else if (k == 'C') {
						vk = (int)ConsoleKey::RightArrow;
					}
					else if (k == 'D') {
						vk = (int)ConsoleKey::LeftArrow;
					}
					else if (k == '1') {
						char fkey = getchar();
						if (fkey == '1') {
							vk = (int)ConsoleKey::F1;
						}
						else if (fkey == '2') {
							vk = (int)ConsoleKey::F2;
						}
						else if (fkey == '3') {
							vk = (int)ConsoleKey::F3;
						}
						else if (fkey == '4') {
							vk = (int)ConsoleKey::F4;
						}
						else if (fkey == '5') {
							vk = (int)ConsoleKey::F5;
						}
						else if (fkey == '6') {
							vk = (int)ConsoleKey::F6;
						}
						else if (fkey == '7') {
							vk = (int)ConsoleKey::F7;
						}
						else if (fkey == '8') {
							vk = (int)ConsoleKey::F8;
						}
						else if (fkey == '9') {
							vk = (int)ConsoleKey::F9;
						}
						else if (fkey == '0') {
							vk = (int)ConsoleKey::F10;
						}
						getchar();
					}
					else if (k == '2') {
						char fkey = getchar();
						if (fkey == '1') {
							vk = (int)ConsoleKey::F11;
						}
						else if (fkey == '2') {
							vk = (int)ConsoleKey::F12;
						}
						getchar();
					}
				}
				else if (d == 'O') {
					char fkey = getchar();
					if (fkey == 'P') {
						vk = (int)ConsoleKey::F1;
					}
					else if (fkey == 'Q') {
						vk = (int)ConsoleKey::F2;
					}
					else if (fkey == 'R') {
						vk = (int)ConsoleKey::F3;
					}
					else if (fkey == 'S') {
						vk = (int)ConsoleKey::F4;
					}
				}

				if (d != EOF)
					c = 0;
			}
			KeyEventArgs kea(0, 1, vk, c, 1);
			parent->Raise("key", kea);
			kea.Pressed = false;
			parent->Raise("key", kea);
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