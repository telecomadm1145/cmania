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
	// Linux 上没有类似于 `timeBeginPeriod()` 的函数来提高时钟分辨率。
}

double HpetClock() {
	uint64_t now;
	struct timespec tp;
	if (syscall(SYS_clock_gettime, CLOCK_MONOTONIC, &tp) < 0) {
		throw std::runtime_error("Failed to get HPET clock");
	}

	// `tp.tv_sec` 和 `tp.tv_nsec` 分别表示秒和纳秒。
	now = tp.tv_sec * 1000000000 + tp.tv_nsec;

	// 将纳秒转换为毫秒。
	return double(now) / 1000000.0;
}
#endif