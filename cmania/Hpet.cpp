#include <windows.h>
#include "Hpet.h"
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