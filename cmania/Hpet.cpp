#include "Hpet.h"

#ifdef _WIN32
#include <windows.h>

#pragma comment(lib, "winmm.lib")


void BeginHighResClock() {
	timeBeginPeriod(1);
}
double HpetClock() {
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	return (double)now.QuadPart / freq.QuadPart * 1000;
}
#endif
#ifdef __linux__
#include <cstdint>
#include <ctime>
#include <stdexcept>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>
#include <chrono>
void BeginHighResClock() {
}
double HpetClock() {
	uint64_t now;
	struct timespec tp;
	if (syscall(SYS_clock_gettime, CLOCK_MONOTONIC, &tp) < 0) {
		std::chrono::high_resolution_clock::time_point tp = std::chrono::high_resolution_clock::now();
		now = std::chrono::duration_cast<std::chrono::microseconds>(tp.time_since_epoch()).count();
		if (now == 0) {
			std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
			now = std::chrono::duration_cast<std::chrono::microseconds>(tp.time_since_epoch()).count();
		}
		return double(now) / 1000.0;
	}
	// now输出毫秒
	now = tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
	return double(now);
}
#endif