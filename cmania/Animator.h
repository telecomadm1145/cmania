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
template<auto K>
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
template<auto Min,auto Max,class Base>
class DurationRangeLimiter {
public:
	static inline auto Get(auto x) {
		return std::clamp(Base::Get(x),Min,Max);
	}
};
template <class EasingFunction, class EasingDurationCalculator, class Num = double, Num Inital = Num()>
class Transition {
private:
	Animator<EasingFunction, Num> animator;

public:
	Transition()
		: animator(Inital, Inital, 0, 0, 1) {}

	void SetValue(double clock, Num value) {
		if (animator.To != value) {
			animator.From = animator.GetCurrentValue(clock);
			animator.To = value;
			animator.Duration = EasingDurationCalculator::Get(animator.To - animator.From);
			animator.Start(clock);
		}
	}

	Num GetCurrentValue(double clock) {
		return animator.GetCurrentValue(clock);
	}
};