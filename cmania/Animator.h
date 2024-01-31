// 头文件
#pragma once
#include <functional>
#include <algorithm>
#include <cmath>
#include "Stopwatch.h"

// 线性缓动函数
class LinearEasingFunction {
public:
	static double Ease(double t) {
		return t;
	}
};

// 幂次缓动函数
template <double N>
class PowerEasingFunction {
public:
	static double Ease(double t) {
		return pow(t, N);
	}
};

// 三次方缓动函数
class CubicEasingFunction {
public:
	static double Ease(double t) {
		return t * t * t;
	}
};

// 缓出函数模板
template <class EasingFunction>
class EaseOut {
public:
	static double Ease(double t) {
		return 1.0 - EasingFunction::Ease(1.0 - t);
	}
};

// 动画器模板
template <class EasingFunction, class Num = double>
class Animator {
public:
	// 起始值
	Num From = 0;
	// 结束值
	Num To = 0;
	// 动画持续时间
	double Duration = 0;
	// 动画延迟时间
	double Offset = 0;
	// 动画时钟速度
	double Clockrate = 1;
	// 动画开始时间
	double StartTime = 1.0 / 0 * 0;

	// 更新动画
	template <class Setter>
	void Update(double clock, Setter setter) {
		if (clock > StartTime) {
			setter(GetCurrentValue(clock));
		}
	}

	// 获取当前动画值
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

	// 启动动画
	void Start(double clock) {
		StartTime = clock + Offset;
	}

	// 重置动画
	void Reset() {
		StartTime = 1.0 / 0 * 0;
	}

	// 构造函数
	Animator(Num From, Num To, double Duration, double Offset = 0, double Clockrate = 1)
		: From(From), To(To), Offset(Offset), Duration(Duration), Clockrate(Clockrate) {
	}
};

// 线性缓动持续时间计算器
template <auto K>
class LinearEasingDurationCalculator {
public:
	static inline auto Get(auto x) {
		return K * x;
	}
};

// 常量缓动持续时间计算器
template <auto V>
class ConstantEasingDurationCalculator {
public:
	static inline auto Get(auto x) {
		return V;
	}
};

// 持续时间范围限制器
template <auto Min, auto Max, class Base>
class DurationRangeLimiter {
public:
	static inline auto Get(auto x) {
		return std::clamp(Base::Get(x), Min, Max);
	}
};

// 过渡模板
template <class EasingFunction, class EasingDurationCalculator, class Num = double>
class Transition {
public:
	Transition()
		: from(Num()), to(Num()) {}

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

// 动态过渡
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