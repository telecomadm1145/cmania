module;
#include <windows.h>
#pragma comment(lib,"winmm.lib")
export module Hpet;


export class HighPerformanceTimer
{
public:
	static void BeginHighResolutionSystemClock()
	{
		timeBeginPeriod(1);
	}
	static double GetMillisecond()
	{
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		return (double)now.QuadPart / freq.QuadPart * 1000;
	}
};