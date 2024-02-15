#pragma once
#include <vector>
#include "AudioManager.h"
#include "Linq.h"
struct HitObject {
	std::vector<AudioSample> samples;
	double StartTime = 0;
	bool HasHit = false;

	void PlaySample() {
		ForEach(samples, [](AudioSample as) {
			auto stm = as->generateStream();
			stm->play();
			delete stm;
		});
	}
};