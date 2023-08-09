#pragma once
#include <functional>
#include "Stopwatch.h"
class LinearEasingFunction {
public:
	static double Ease(double t) {
		return t;
	}
};
template <double N>
class PowerEasingFunction {
public:
	static double Ease(double t) {
		return pow(t, N);
	}
};
template <class EasingFunction>
class Animator {
public:
	double From = 0;
	double To = 0;
	double Duration = 0;
	double Offset = 0;
	double Clockrate = 1;
	double StartTime = 1.0 / 0 * 0;
	template <class Setter>
	void Update(double clock, Setter setter) {
		if (clock > StartTime) {
			double progress = (clock - StartTime) / Duration;
			if (progress > 1) {
				StartTime = 1.0 / 0 * 0;
				return;
			}
			double rate = Clockrate;
			setter(EasingFunction::Ease(progress) * (To - From) * rate + From);
		}
	}
	void Start(double clock) {
		StartTime = clock + Offset;
	}
	void Reset() {
		StartTime = 1.0 / 0 * 0;
	}

	Animator(double From, double To, double Duration, double Offset = 0, double Clockrate = 1)
		: From(From), To(To), Offset(Offset), Duration(Duration), Clockrate(Clockrate) {
	}
};