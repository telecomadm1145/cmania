#include "ConsoleInput.h"
#include "Game.h"
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <optional>
#include <sys/select.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <thread>
#include <unistd.h>
#include <iostream>
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
			input_thread = new std::thread(&InputWorker, parent);
			input_thread->detach();
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

			// If the character is a mouse event, print the event type and coordinates
			//if (c == '\033') {
			//	//char seq[3];
			//	//read(STDIN_FILENO, seq, 3);
			//	//if (seq[0] == '[') {
			//	//	int button = seq[2] - '0';
			//	//	int x = seq[4] - '0';
			//	//	int y = seq[5] - '0';
			//	//	cout << "Mouse event: button=" << button << ", x=" << x << ", y=" << y << endl;
			//	//}
			//}
			auto vk = c;
			if (c == '\n')
			{
				vk = 13;
			}
			if (c == '\b')
			{
				vk = 8;
			}
			KeyEventArgs kea(0, 1, vk, c, 1);
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