#pragma once
#include <utility>
#include <map>
#include <string>
#include "GameBuffer.h"

enum class HitResult : unsigned int {
	None,
	// 0
	Miss,
	// 50
	Meh,
	// 100
	Ok,
	// 200
	Good,
	// 300
	Great,
	// 320
	Perfect,
	// 0
	SmallTickMiss,
	SmallTickHit,
	LargeTickMiss,
	LargeTickHit,
	SmallBonus,
	LargeBonus,
	IgnoreMiss,
	IgnoreHit,
};

enum class PathType : char {
	None = 0,
	Catmull = 'C',
	Bezier = 'B',
	Linear = 'L',
	PerfectCurve = 'P',
};

enum class HitSoundType : unsigned int {
	None,
	Normal = 1,
	Whistle = 2,
	Finish = 4,
	Clap = 8,
	Slide = 16,
	SlideTick = 32,
	SlideWhistle = 64,
};
enum class HitObjectType : unsigned int {
	Circle = 1,
	Slider = 1 << 1,
	NewCombo = 1 << 2,
	Spinner = 1 << 3,
	ComboOffset = (1 << 4) | (1 << 5) | (1 << 6),
	Hold = 1 << 7
};
enum class SampleBank : unsigned int {
	None,
	Normal,
	Soft,
	Drum
};
enum class GameMode : unsigned int {
	Std,
	Taiko,
	Catch,
	Mania,
};
struct HitRange {
	HitResult Result;
	double Min;
	double Average;
	double Max;
};
enum class EffectFlags : unsigned int {
	None = 0,
	Kiai = 1,
	OmitFirstBarLine = 8
};
constexpr inline float OBJECT_RADIUS = 64;
constexpr inline float BASE_SCORING_DISTANCE = 100;
constexpr inline double PREEMPT_MIN = 450;
constexpr inline float DEFAULT_DIFFICULTY = 5;
constexpr inline double FADE_OUT_DURATION = 200;
constexpr inline double FADE_OUT_SCALE = 1.5;
constexpr inline double DifficultyRange(double difficulty, double min, double mid, double max) {
	if (difficulty > 5)
		return mid + (max - mid) * (difficulty - 5) / 5;
	if (difficulty < 5)
		return mid - (mid - min) * (5 - difficulty) / 5;
	return mid;
}
constexpr inline HitRange BaseHitRanges[] = {
	HitRange(HitResult::Perfect, 22.4, 19.4, 13.9),
	HitRange(HitResult::Great, 64, 49, 34),
	HitRange(HitResult::Good, 97, 82, 67),
	HitRange(HitResult::Ok, 127, 112, 97),
	HitRange(HitResult::Meh, 151, 136, 121),
	HitRange(HitResult::Miss, 188, 173, 158)
};
inline std::map<HitResult, double> GetHitRanges(double od) {
	std::map<HitResult, double> res;
	for (auto& x : BaseHitRanges) {
		res[x.Result] = DifficultyRange(od, x.Min, x.Average, x.Max);
	}
	return res;
}
constexpr inline double DifficultyFadeIn(double preempt) {
	return 400 * std::min(1.0, preempt / PREEMPT_MIN);
}
constexpr inline double DifficultyPreempt(double ar) {
	return DifficultyRange(ar, 1800, 1200, PREEMPT_MIN);
}
constexpr inline double DifficultyScale(double cs) {
	return (1.0f - 0.7f * (cs - 5) / 5) / 2;
}
constexpr inline int CalcColumn(double xpos, int keys) {
	double begin = 512 / keys / 2;
	double mid = begin;
	for (int i = 0; i < keys; i++) {
		if (std::abs(mid - xpos - 1) < begin) {
			return i;
		}
		mid += begin * 2;
	}
#if _DEBUG
	__debugbreak();
#endif
	return 0;
}
inline std::string GetHitResultName(HitResult res) {
	switch (res) {
	case HitResult::Miss:
		return "Miss";
	case HitResult::Meh:
		return "Meh";
	case HitResult::Ok:
		return "Ok";
	case HitResult::Good:
		return "Good";
	case HitResult::Great:
		return "Great";
	case HitResult::Perfect:
		return "Perf";
	default:
		return "Unknown";
	}
}
inline std::string GetRulesetName(int id) {
	switch (id) {
	case 0:
		return "Std";
	case 1:
		return "Taiko";
	case 2:
		return "Catch";
	case 3:
		return "Mania";
	default:
		return "Unknown";
	}
}
inline std::string GetRulesetId(int id) {
	switch (id) {
	case 0:
		return "osustd";
	case 1:
		return "osutaiko";
	case 2:
		return "osucatch";
	case 3:
		return "osumania";
	default:
		return "unk";
	}
}
constexpr inline Color GetHitResultColor(HitResult res) {
	switch (res) {
	case HitResult::Miss:
		return { 255, 255, 0, 0 };
	case HitResult::Meh:
		return { 255, 255, 132, 0 };
	case HitResult::Ok:
		return { 255, 255, 192, 56 };
	case HitResult::Good:
		return { 255, 255, 255, 114 };
	case HitResult::Great:
		return { 255, 0, 192, 255 };
	case HitResult::Perfect:
		return { 255, 147, 228, 255 };
	default:
		return { 255, 255, 255, 255 };
	}
}
constexpr inline int GetBaseScore(HitResult res) {
	switch (res) {
	case HitResult::Miss:
		return 0;
	case HitResult::Meh:
		return 50;
	case HitResult::Ok:
		return 100;
	case HitResult::Good:
		return 200;
	case HitResult::Great:
		return 300;
	case HitResult::Perfect:
		return 320;
	default:
		return 0;
	}
}