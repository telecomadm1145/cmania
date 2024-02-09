#pragma once
#include "EnumFlag.h"
#include <string>

// High word: Keys
enum class OsuMods
{
	None = 0b0,
	Easy = 0b1,
	NoFall = 0b10,
	HalfTime = 0b100,
	Nightcore = 0b1000,
	Hardrock = 0b10000,
	Hidden = 0b100000,
	FadeOut = 0b1000000,
	//JumpHelper= 0b10000000,
	Auto = 0b100000000,
	Relax = 0b1000000000,
	Mirror = 0b10000000000,
	Random = 0b100000000000,
	NoJump = 0b1000000000000,
	NoHold = 0b10000000000000,
	Coop = 0b100000000000000,
	//NoBg		= 0b1000000000000000,

	Random1K = Random | (1ULL << 16),
	Random2K = Random | (2ULL << 16),
	Random3K = Random | (3ULL << 16),
	Random4K = Random | (4ULL << 16),
	Random5K = Random | (5ULL << 16),
	Random6K = Random | (6ULL << 16),
	Random7K = Random | (7ULL << 16),
	Random8K = Random | (8ULL << 16),
	Random9K = Random | (9ULL << 16),
};
inline double GetModScale(OsuMods mods)
{
	auto score_mod = 1.0;
	if (HasFlag(mods, OsuMods::FadeOut))
		score_mod += 0.06;
	if (HasFlag(mods, OsuMods::Hidden))
		score_mod += 0.06;
	if (HasFlag(mods, OsuMods::Hardrock))
		score_mod += 0.06;
	if (HasFlag(mods, OsuMods::Nightcore))
		score_mod += 0.12;
	if (HasFlag(mods, OsuMods::Easy))
		score_mod *= 0.5;
	if (HasFlag(mods, OsuMods::HalfTime))
		score_mod *= 0.5;
	if (HasFlag(mods, OsuMods::NoFall))
		score_mod *= 0.5;
	if (HasFlag(mods, OsuMods::Relax))
		score_mod *= 0.1;
	return score_mod;
}
inline double GetPlaybackRate(OsuMods mods)
{
	auto rate = 1.0;
	if (HasFlag(mods, OsuMods::Nightcore))
		rate *= 1.5;
	if (HasFlag(mods, OsuMods::HalfTime))
		rate *= 0.75;
	return rate;
}
constexpr inline std::string GetModsAbbr(OsuMods mods) {
	std::string s = "";
	if (HasFlag(mods, OsuMods::Auto))
		s += "AT";
	if (HasFlag(mods, OsuMods::Relax))
		s += "RX";
	if (HasFlag(mods, OsuMods::Hardrock))
		s += "HR";
	if (HasFlag(mods, OsuMods::Easy))
		s += "EZ";
	if (HasFlag(mods, OsuMods::Nightcore))
		s += "NC";
	if (HasFlag(mods, OsuMods::HalfTime))
		s += "HT";
	if (HasFlag(mods, OsuMods::FadeOut))
		s += "FO";
	if (HasFlag(mods, OsuMods::Hidden))
		s += "HD";
	if (HasFlag(mods, OsuMods::NoFall))
		s += "NF";
	if (s.empty()) {
		return "NM";
	}
	return s;
}