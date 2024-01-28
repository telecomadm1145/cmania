#pragma once
#include "Gameplay.h"
class DifficultyInfoItem {
public:
	std::string Text;
	double Value;
	double MinValue;
	double MaxValue;
	enum {
		ValueBar,
		Header,
		PlainText,
		PlainValue,
		Header2,
	} Type;
	std::string Text2;
	static DifficultyInfoItem MakeValueBar(std::string Header,double Value,double MinValue = 0,double MaxValue = 9999)
	{
		return { Header, Value, MinValue, MaxValue, ValueBar };
	}
	static DifficultyInfoItem MakeValue(std::string Header, double Value) {
		return { Header, Value, 0, 0, DifficultyInfoItem::PlainValue };
	}
	static DifficultyInfoItem MakeHeader(std::string Header) {
		return { Header, 0, 0, 0, DifficultyInfoItem::Header };
	}
	static DifficultyInfoItem MakeHeader2(std::string Header) {
		return { Header, 0, 0, 0, DifficultyInfoItem::Header2 };
	}
	static DifficultyInfoItem MakeText(std::string Header,std::string Text) {
		return { Header, 0, 0, 0, DifficultyInfoItem::PlainText, Text };
	}
};
using DifficultyInfo = std::vector<DifficultyInfoItem>;
class Ruleset {
public:
	virtual void Init(BinaryStorage& settings) = 0;
	virtual std::string Id() = 0;
	virtual std::string DisplayName() = 0;
	virtual Beatmap* LoadBeatmap(path beatmap_path,bool load_samples = true) = 0;
	virtual GameplayBase* GenerateGameplay() = 0;
	virtual double CalculateDifficulty(Beatmap* bmp,OsuMods mods) = 0;
	virtual DifficultyInfo PopulateDifficultyInfo(Beatmap* bmp) = 0;
	virtual ~Ruleset() {

	}
};