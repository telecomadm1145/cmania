#pragma once
#include "AudioManager.h"
#include "HitObject.h"
struct ManiaObject : public HitObject {
	AudioSample ssample;
	AudioStream ssample_stream;
	AudioSample ssamplew;
	AudioStream ssamplew_stream;
	int Column;
	bool Multi;
	bool HasHold;
	bool HoldBroken;
	double EndTime;
	double LastHoldOff = -1;
};