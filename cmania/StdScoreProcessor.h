#pragma once
#include "ScoreProcessor.h"
#include "StdObject.h"
#include "Linq.h"
#include "Defines.h"

class StdScoreProcessor : public ScoreProcessor<StdObject> {
	std::map<HitResult, double> hit_ranges;
	double reference_rating = 1;
	double pp_m = 1;
	double score_m = 1;

public:
	virtual void ApplyBeatmap(double ref_rating) override {
		reference_rating = pow(ref_rating, 3);
	}
	virtual void SetMods(OsuMods mods) override {
		score_m = GetModScale(mods);
		mods = RemoveFlag(mods, OsuMods::HalfTime);
		mods = RemoveFlag(mods, OsuMods::Nightcore);
		pp_m = GetModScale(mods);
	}
	virtual void SetDifficulty(double od) override {
		hit_ranges = GetHitRanges(od);
	}
	StdScoreProcessor() {
		ResultCounter[HitResult::Great];
		ResultCounter[HitResult::Ok];
		ResultCounter[HitResult::Meh];
		ResultCounter[HitResult::Miss];
		hit_ranges = GetHitRanges(0);
	}
	// 通过 ScoreProcessor 继承
	virtual HitResult ApplyHit(StdObject& mo, double err) override {
		auto is_hold = mo.EndTime != 0;
		if (mo.HasHit && !is_hold) {
			return HitResult::None;
		}
		HitResult res = HitResult::None;
		if (std::isnan(err)) {
			res = HitResult::Miss;
		}
		else {
			ResultCounter > ForEach([&](const auto& item) {
				if (std::abs(err) < hit_ranges[item.first]) {
					res = item.first;
				}
			});
		}
		if (res > HitResult::None) {
			if (mo.HasHit) {
				if (res != HitResult::Miss) {
					mo.HasHold = true;
				}
			}
			if (is_hold && res == HitResult::Miss) {
				mo.HoldBroken = true;
			}
			mo.HasHit = true;
			Combo++;

			if (res == HitResult::Miss)
				Combo = 0;
			MaxCombo = std::max(Combo, MaxCombo);

			RawAccuracy += std::min(GetBaseScore(res), GetBaseScore(HitResult::Great));
			AppliedHit++;

			Accuracy = (double)RawAccuracy / AppliedHit / GetBaseScore(HitResult::Great);

			Rating = reference_rating * std::pow(((double)MaxCombo / BeatmapMaxCombo), 0.3) * pow(Accuracy, 1.3) * pow(pp_m, 1.2) * pow(0.95, ResultCounter[HitResult::Miss]);

			RawScore += GetBaseScore(res);

			if (res != HitResult::Miss) {
				RawError += err;
				Errors.push_back(err);
				Mean = (double)RawError / Errors.size();
				Error = variance(Mean, Errors);
			}

			Score = (((double)RawScore / BeatmapMaxCombo / GetBaseScore(HitResult::Great)) * 0.7 + ((double)MaxCombo / BeatmapMaxCombo) * 0.3) * score_m;

			ResultCounter[res]++;
		}
		return res;
	}
	virtual double GetHealthIncreaseFor(HitResult res) override {
		return 0.0;
	}

	// 通过 ScoreProcessor 继承
	virtual void SaveRecord() override {
		RulesetRecord->Rating = Rating;
		RulesetRecord->Mean = Mean;
		RulesetRecord->Error = Error;
		RulesetRecord->Score = Score;
		RulesetRecord->Accuracy = Accuracy;
		RulesetRecord->ResultCounter = ResultCounter;
		RulesetRecord->MaxCombo = MaxCombo;
		RulesetRecord->BeatmapMaxCombo = BeatmapMaxCombo;
	}
};