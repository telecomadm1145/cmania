export module DifficultyCalculator;
import OsuBeatmap;
import <algorithm>;

constexpr auto t1 = 6.0;
double GetTimeDiff(double t)
{
	if (t > 0.5)
		return t1 / t;
	if (t < 0)
		return 0;
	if (t <= 0.5)
		return t1 / 0.5;
}
int CalcColumn(double xpos, int keys)
{
	double begin = 512 / keys / 2;
	double mid = begin;
	for (int i = 0; i < keys; i++)
	{
		if (std::abs(mid - xpos - 1) < begin)
		{
			return i;
		}
		mid += begin * 2;
	}
	__debugbreak();
	return 0;
}
export double CalculateDiff(OsuBeatmap beatmap, int keys = 0)
{
	if (beatmap.HitObjects.size() < 10)
		return 0;
	if (keys == 0)
	{
		keys = (int)beatmap.CircleSize;
	}
	std::vector<double> diffs;
	for (int i = 0; i < beatmap.HitObjects.size() - 1; i++) {
		auto& ho = beatmap.HitObjects[i];

		double lastjudge = ho.StartTime;
		double timeDiff = 0;
		if (ho.EndTime != 0) {
			lastjudge = ho.EndTime;
		}
		double nearestSameColumnTime = -1;
		for (int j = i; j < beatmap.HitObjects.size() - 1; j++)
		{
			auto& ho2 = beatmap.HitObjects[j];
			if (CalcColumn(ho2.X, keys) == CalcColumn(ho.X, keys) && ho2.StartTime > ho.StartTime && ho2.StartTime > ho.EndTime)
			{
				nearestSameColumnTime = ho2.StartTime;
				break;
			}
		}
		if (nearestSameColumnTime != -1)
			timeDiff += GetTimeDiff(nearestSameColumnTime - lastjudge);
		double nearestTime = -1;
		for (int j = i; j < beatmap.HitObjects.size() - 1; j++)
		{
			auto& ho2 = beatmap.HitObjects[j];
			if (ho2.StartTime > ho.StartTime && ho2.StartTime > ho.EndTime)
			{
				nearestTime = ho2.StartTime;
				break;
			}
		}
		if (nearestTime != -1)
			timeDiff += GetTimeDiff(nearestTime - lastjudge);
		auto multi = 1.0;
		for (int j = 0; j < beatmap.HitObjects.size() - 1; j++)
		{
			auto& ho2 = beatmap.HitObjects[j];
			if (std::abs(ho2.StartTime - ho.StartTime) < 5)
			{
				if (&ho2 != &ho)
					multi *= 1.06;
			}
			if (ho2.EndTime != 0 && ho.StartTime >= ho2.StartTime && ho.StartTime <= ho2.EndTime)
			{
				if (&ho2 != &ho)
					multi *= 1.0 + (ho2.EndTime - ho2.StartTime) / 64000;
			}
		}
		if (ho.EndTime != 0)
		{
			multi *= 1.01 + (ho.EndTime - ho.StartTime) / 64000;
		}
		diffs.push_back(timeDiff * multi);
	}
	std::sort(diffs.begin(), diffs.end());
	double avgdiff = 0;
	size_t diffnum = diffs.size();
	double top20diff = 0;
	size_t top20num = diffs.size() * 0.2;
	for (size_t k = 0; k < diffnum; k++)
	{
		if (k >= diffnum - top20num)
		{
			top20diff += std::max(0.008,diffs[k]);
		}
		avgdiff += std::max(0.003,diffs[k]);
	}
	avgdiff /= diffnum;
	top20diff /= top20num;
	return (avgdiff + top20diff) * 100 * pow((double)keys / 4,0.2);
}