#include "Game.h"
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <optional>
#include <sys/select.h>
#include <termios.h>
#include <thread>
class LinuxConsoleComponent : public GameComponent {
private:
	std::thread* input_thread = nullptr;
	fd_set fds;
	struct termios oldsetting,newsetting;
	void ProcessEvent(const char* evt, const void* evtargs) override {
		if (strcmp(evt, "start") == 0) {
			// 设置终端为非规范模式
			tcgetattr(fileno(stdin), &oldsetting);
			newsetting = oldsetting;
			newsetting.c_lflag &= ~(ICANON | ECHO);
			tcsetattr(fileno(stdin), TCSANOW, &newsetting);
			FD_ZERO(&fds);
			FD_SET(fileno(stdin), &fds);
			input_thread=new std::thread(&InputWorker, parent);
			input_thread->detach();
			
		}
		else if (strcmp(evt, "push") == 0) {
		}
		else if (strcmp(evt, "fresize") == 0) {
		}
	}
	static void InputWorker(Game* parent) {
		while(1){
			int res=0;
			
			
		}
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