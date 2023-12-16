#pragma once
#include <functional>
#include <algorithm>
#include <cmath>
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
class CubicEasingFunction {
public:
	static double Ease(double t) {
		return t * t * t;
	}
};
template <class EasingFunction>
class EaseOut {
public:
	static double Ease(double t) {
		return 1.0 - EasingFunction::Ease(1.0 - t);
	}
};
template <class EasingFunction, class Num = double>
class Animator {
public:
	Num From = 0;
	Num To = 0;
	double Duration = 0;
	double Offset = 0;
	double Clockrate = 1;
	double StartTime = 1.0 / 0 * 0;
	template <class Setter>
	void Update(double clock, Setter setter) {
		if (clock > StartTime) {
			setter(GetCurrentValue(clock));
		}
	}
	Num GetCurrentValue(double clock) {
		if (clock <= StartTime) {
			return From;
		}
		double progress = (clock - StartTime) / Duration;
		if (progress > 1 || isnan(progress)) {
			StartTime = 1.0 / 0 * 0;
			return To;
		}
		return EasingFunction::Ease(progress) * Clockrate * (To - From) + From;
	}
	void Start(double clock) {
		StartTime = clock + Offset;
	}
	void Reset() {
		StartTime = 1.0 / 0 * 0;
	}

	Animator(Num From, Num To, double Duration, double Offset = 0, double Clockrate = 1)
		: From(From), To(To), Offset(Offset), Duration(Duration), Clockrate(Clockrate) {
	}
};
template <auto K>
class LinearEasingDurationCalculator {
public:
	static inline auto Get(auto x) {
		return K * x;
	}
};
template <auto V>
class ConstantEasingDurationCalculator {
public:
	static inline auto Get(auto x) {
		return V;
	}
};
template <auto Min, auto Max, class Base>
class DurationRangeLimiter {
public:
	static inline auto Get(auto x) {
		return std::clamp(Base::Get(x), Min, Max);
	}
};
template <class EasingFunction, class EasingDurationCalculator, class Num = double, Num Inital = Num()>
class Transition {
public:
	Transition()
		: from(Inital), to(Inital) {}

	void SetValue(double clock, Num new_value) {
		if (to != new_value) {
			from = GetCurrentValue(clock);
			to = new_value;
			start_time = clock;
		}
	}

	Num GetCurrentValue(double clock) {
		if (clock < start_time) {
			return from;
		}
		double progress = (clock - start_time) / EasingDurationCalculator::Get(std::abs(to - from));
		if (progress > 1 || isnan(progress)) {
			return to;
		}
		return EasingFunction::Ease(progress) * (to - from) + from;
	}

private:
	Num from{};
	Num to{};
	double start_time = 1.0 / 0.0 * 0.0;
};
template <class Num = double>
class DynamicsTransition {
public:
	DynamicsTransition() {}

	Num GetCurrentValue(double clock, Num to) {
		auto dur = clock - start_time;
		start_time = clock;
		if (dur > 200) {
			return orig = to;
		}
		if (dur >= 0) {
			orig += (to - orig) * dur / 250;
		}
		return orig;
	}

private:
	Num orig = {};
	double start_time = 1.0 / 0.0 * 0.0;
};