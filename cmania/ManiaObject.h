#pragma once
#include "AudioManager.h"
#include "HitObject.h"
struct ManiaObject : public HitObject {
	bool Multi;
	bool HasHold;
	bool HoldBroken;
	int Column;
	AudioSample ssample;
	AudioStream ssample_stream;
	AudioSample ssamplew;
	AudioStream ssamplew_stream;
	double EndTime;
	double LastHoldOff = -1;
};