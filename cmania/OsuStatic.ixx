export module OsuStatic;
import <map>;

export class OsuStatic
{
public:
	enum class HitResult
	{
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
	enum class HitSoundType
	{
		None,
		Normal = 1,
		Whistle = 2,
		Finish = 4,
		Clap = 8,
		Slide = 16,
		SlideTick = 32,
		SlideWhistle = 64,
	};
	enum class HitObjectType
	{
		Circle = 1,
		Slider = 1 << 1,
		NewCombo = 1 << 2,
		Spinner = 1 << 3,
		ComboOffset = (1 << 4) | (1 << 5) | (1 << 6),
		Hold = 1 << 7
	};
	enum class SampleBank
	{
		None,
		Normal,
		Soft,
		Drum
	};
	enum class GameMode
	{
		Std,
		Taiko,
		Catch,
		Mania,
	};
	struct HitRange
	{
		HitResult Result;
		double Min;
		double Average;
		double Max;
	};
	enum class EffectFlags
	{
		None = 0,
		Kiai = 1,
		OmitFirstBarLine = 8
	};
	static constexpr float OBJECT_RADIUS = 64;
	static constexpr float BASE_SCORING_DISTANCE = 100;
	static constexpr double PREEMPT_MIN = 450;
	static constexpr float DEFAULT_DIFFICULTY = 5;
	static constexpr double FADE_OUT_DURATION = 200;
	static constexpr double FADE_OUT_SCALE = 1.5;
	static constexpr double DifficultyRange(double difficulty, double min, double mid, double max)
	{
		if (difficulty > 5)
			return mid + (max - mid) * (difficulty - 5) / 5;
		if (difficulty < 5)
			return mid - (mid - min) * (5 - difficulty) / 5;
		return mid;
	}
	static constexpr HitRange BaseHitRanges[] = {
		HitRange(HitResult::Perfect, 22.4, 19.4, 13.9),
		HitRange(HitResult::Great, 64, 49, 34),
		HitRange(HitResult::Good, 97, 82, 67),
		HitRange(HitResult::Ok, 127, 112, 97),
		HitRange(HitResult::Meh, 151, 136, 121),
		HitRange(HitResult::Miss, 188, 173, 158)
	};
	static std::map<HitResult, double> GetHitRanges(double od)
	{
		std::map<HitResult, double> res;
		for (auto x : BaseHitRanges)
		{
			res[x.Result] = DifficultyRange(od, x.Min, x.Average, x.Max);
		}
		return res;
	}
	static double DifficultyFadeIn(double preempt)
	{
		return 400 * std::min(1.0, preempt / PREEMPT_MIN);
	}
	static double DifficultyPreempt(double ar)
	{
		return DifficultyRange(ar, 1800, 1200, PREEMPT_MIN);
	}
	static double DifficultyScale(double cs)
	{
		return(1.0f - 0.7f * (cs - 5) / 5) / 2;
	}
};