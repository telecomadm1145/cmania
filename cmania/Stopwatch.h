#pragma once
#include "Hpet.h"

class Stopwatch
{
	bool running = false;
	double ems = 0;
	double sms = 0;
	double rate = 1;
public:
	void Start()
	{
		if (!running)
		{
			sms = HpetClock();
			running = true;
		}
	}
	void SetRate(double newrate)
	{
		if (running)
		{
			ems += (HpetClock() - sms) * rate;
			sms = 0;
		}
		rate = newrate;
	}
	void Stop()
	{
		if (running)
		{
			ems += (HpetClock() - sms) * rate;
			sms = 0;
			running = false;
		}
	}
	void Reset()
	{
		ems = 0;
		if (running)
		{
			sms = HpetClock();
		}
	}
	void Offset(double off)
	{
		ems += off;
	}
	bool Running() const { return running; }
	double ClockRate() const { return rate; }
	double Elapsed() const
	{
		if (!running)
			return ems;
		return (HpetClock() - sms) * rate + ems;
	}
};