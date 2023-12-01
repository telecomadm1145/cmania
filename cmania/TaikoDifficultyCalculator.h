#pragma once
#include "Beatmap.h"
#include "TaikoObject.h"
#include <algorithm>
#include "EnumFlag.h"
#include "OsuMods.h"
inline double GetTimeDiff(double t) {
	if (t > 0.5)
		return 6 / t;
	if (t < 0)
		return GetTimeDiff(-t);
	return 6 / 0.5;
}
double CalculateDiff(const Beatmap<TaikoObject>& beatmap, OsuMods mods) {
	if (beatmap.size() < 10)
		return 0;
	double drain_time = 0;
	std::vector<double> diffs;
	auto objs = beatmap;
	std::sort(objs.begin(), objs.end(), [](auto& a, auto& b) { return a.StartTime < b.StartTime; });
	for (int i = 0; i < objs.size() - 1; i++) {
		auto& ho = objs[i];
		if (HasFlag(ho.ObjectType, TaikoObject::Barline) || HasFlag(ho.ObjectType, TaikoObject::Spinner) ||
			HasFlag(ho.ObjectType, TaikoObject::SliderTick) || HasFlag(ho.ObjectType, TaikoObject::Slider)) {
			continue;
		}
		auto large = HasFlag(ho.ObjectType, TaikoObject::Large);
		double lastjudge = ho.StartTime;
		double timeDiff = 0;
		double nearestTime = -1;
		for (int j = i; j < objs.size() - 1; j++) {
			auto& ho2 = objs[j];
			if (ho2.StartTime > (lastjudge + 1)) {
				nearestTime = ho2.StartTime;
				auto between = abs(ho2.StartTime - ho.StartTime);
				if (between < 500)
					drain_time += between;
				break;
			}
		}
		if (nearestTime != -1)
			timeDiff += GetTimeDiff(nearestTime - lastjudge);
		double multi = 1;
		if (large)
			multi *= 1.5;
		diffs.push_back(timeDiff * multi);
	}
	return (std::accumulate(diffs.begin(), diffs.end(), 0.0) / diffs.size()) * 100;
}