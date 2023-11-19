#pragma once
#include "AudioManager.h"
#include "HitObject.h"
struct TaikoObject : public HitObject {
	enum {
		Don = 0,
		Kat = 1,// 0位
		Large = 2,// 1位
		Spinner = 4,// 2位
		Barline = 8,
		SliderTick = 16,
	} ObjectType;
	double EndTime;
	int RemainsHits;
	int TotalHits;
	// 以 1 为单位(
	double Velocity;
};