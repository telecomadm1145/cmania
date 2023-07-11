export module Stopwatch;
import Hpet;

export class Stopwatch
{
	bool running = false;
	double ems = 0;
	double sms = 0;
public:
	void Start()
	{
		if (!running)
		{
			sms = HighPerformanceTimer::GetMilliseconds();
			running = true;
		}
	}
	void Stop()
	{
		if (running)
		{
			ems += HighPerformanceTimer::GetMilliseconds() - sms;
			sms = 0;
			running = false;
		}
	}
	void Restart()
	{
		ems = 0;
		if (running)
		{
			sms = HighPerformanceTimer::GetMilliseconds();
		}
	}
	double Elapsed() const
	{
		if (!running)
			return 0;
		return HighPerformanceTimer::GetMilliseconds() - sms + ems;
	}
};