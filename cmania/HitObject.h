#pragma once
#include <vector>
#include "Linq.h"
struct HitObject {
	std::vector<AudioSample> samples;
	double StartTime;
	bool HasHit;

	void PlaySample() {
		ForEach(samples, [](AudioSample as) {
			auto stm = as->generateStream();
			stm->play();
			delete stm;
		});
	}
};