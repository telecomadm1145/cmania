export module OsuMods;

// High word: Keys
export enum class OsuMods
{
	None		= 0b0,
	Easy		= 0b1,
	NoFall		= 0b10,
	HalfTime	= 0b100,
	Nightcore	= 0b1000,
	Hardrock	= 0b10000,
	Hidden		= 0b100000,
	FadeOut		= 0b1000000,
	Coop		= 0b10000000,
	Auto		= 0b100000000,
	Mirror		= 0b1000000000,
	NoJump		= 0b10000000000,
};