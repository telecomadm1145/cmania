#pragma once
#include "HitObject.h"
#include "Defines.h"
#include "OsuStatic.h"
#include "Path.h"
#include "Linq.h"
/// <summary>
/// This impls a osu!std slider path.
/// </summary>
struct SliderPath {
	using SliderControlPoint = std::pair<PathType, PointD>;
	std::vector<PointD> calcedPath;
	std::vector<double> calcedLength;
	std::vector<SliderControlPoint> ControlPoints;
	double expectedLength;
	double actualLength;
	void CalcPath() {
		if (ControlPoints.size() == 0)
			return;
		size_t start = 0;
		PathType pt = PathType::None;
		for (size_t i = 0; i < ControlPoints.size() - 1; i++) {
			auto& cp = ControlPoints[i];
			if (cp.first != PathType::None) {
				if (pt != PathType::None) {
					CalcSubPath(start, i + 1, pt);
					start = i;
				}
				pt = cp.first;
			}
		}
		if (pt != PathType::None) {
			CalcSubPath(start, ControlPoints.size(), pt);
		}
	}
	void CalcSubPath(int start, int end, PathType type) {
		std::vector<PointD> temp;
		for (size_t i = start; i < end; i++) {
			temp.push_back(ControlPoints[i].second);
		}
		switch (type) {
		case PathType::Catmull:
			calcedPath > AddRange(PathApproximator::ApproximateCatmull(temp));
			break;
		case PathType::PerfectCurve:

		case PathType::Bezier:
			calcedPath > AddRange(PathApproximator::ApproximateBezier(temp));
			break;
		case PathType::Linear:
			calcedPath > AddRange(temp);
			break;
		default:
			break;
		}
	}
	void CalcLength() {
		actualLength = 0;
		if (calcedPath.size() == 0)
			return;
		for (size_t i = 0; i < calcedPath.size() - 1; i++) {
			VectorD diff = calcedPath[i + 1] - calcedPath[i];
			actualLength += diff.Length();
			calcedLength.push_back(actualLength);
		}
		if (!std::isnan(expectedLength) && expectedLength != actualLength) {
			 auto sz = ControlPoints.size();
			 if (sz >= 2) {
				// In osu-stable, if the last two control points of a slider are equal, extension is not performed.
				if (ControlPoints[sz - 1] == ControlPoints[sz - 2]) {
					calcedLength.push_back(expectedLength);
					return;
				}
			 }
			 calcedLength.resize(calcedLength.size() - 1);
			 int pathEndIndex = calcedPath.size() - 1;
			 if (actualLength > expectedLength) {
				while (calcedLength.size() > 0 && calcedLength[calcedLength.size() - 1] >= expectedLength) {
					calcedLength.resize(calcedLength.size() - 1);
					calcedPath.resize(pathEndIndex--);
				}
			 }
			 if (pathEndIndex <= 0) {
				actualLength = 0;
				calcedLength.push_back(0);
				return;
			 }
			 VectorD dir = calcedPath[pathEndIndex] - calcedPath[pathEndIndex - 1];
			 dir.Normalize();
			 auto l = 0.0;
			 if (calcedLength.size() >= 1)
				l = calcedLength[calcedLength.size() - 1];
			 calcedPath[pathEndIndex] = calcedPath[pathEndIndex - 1] + dir * (expectedLength - l);
			 calcedLength.push_back(expectedLength);
			 actualLength = expectedLength;
		}
	}
	size_t IndexOfDistance(double d) {
		int i = 1;
		for (auto d2 : calcedLength) {
			if (d2 >= d)
				return i;
			i++;
		}
		return 0;
	}
	VectorD InterpolateVertices(int i, double d) {
		if (calcedPath.size() == 0)
			return {};

		if (i <= 0)
			return *calcedPath.begin();
		if (i >= calcedPath.size())
			return *calcedPath.end();

		VectorD p0 = calcedPath[i - 1];
		VectorD p1 = calcedPath[i];
		VectorD dir = p1-p0;
		dir.Normalize();
		double d0 = calcedLength[i - 1];
		if (d0 - d < 0.1)
			return p0;
		return p1 - dir * (d0 - d);
	}
	double ProgressToDistance(double progress) {
		return progress * actualLength;
	}
	VectorD PositionAt(double progress) {
		auto d = ProgressToDistance(progress);
		return InterpolateVertices(IndexOfDistance(d), d);
	}
	SliderPath(const std::vector<SliderControlPoint>& points, double length) : ControlPoints(points), expectedLength(length) {
		CalcPath();
		CalcLength();
	}
	static SliderPath Parse(std::string s, PointD orig, double length) {
		std::vector<SliderControlPoint> pts;
		PathType pt = PathType::None;
		bool first = 0;
		int startx = 0;
		int starty = 0;
		int status = 0;
		for (size_t i = 0; i < s.size(); i++) {
			if (s[i] >= '0' && s[i] <= '9') {
				if (status == 0) {
					startx = i;
					status = 1;
				}
				if (status == 2) {
					starty = i;
					status = 3;
				}
			}
			else {
				switch (status) {
				case 0: {
					if (s[i] >= 'A' && s[i] <= 'Z') {
						pt = (PathType)s[i];
					}
					break;
				case 1: {
					status = 2;
					break;
				}
				case 3: {
					if (!first) {
						pts.push_back(SliderControlPoint{ pt, orig });
						first = 1;
						pt = PathType::None;
					}
					auto ptr = s.data();
					SliderControlPoint scp{};
					scp.first = pt;
					scp.second = { (double)atoi(ptr + startx), (double)atoi(ptr + starty) };
					pts.push_back(scp);
					status = 0;
					pt = PathType::None;
					break;
				}
				}
				default:
					break;
				}
			}
		}
		if (status == 3) {
			if (!first) {
				pts.push_back(SliderControlPoint{ pt, orig });
				first = 1;
				pt = PathType::None;
			}
			auto ptr = s.data();
			SliderControlPoint scp{};
			scp.first = pt;
			scp.second = { (double)atoi(ptr + startx), (double)atoi(ptr + starty) };
			pts.push_back(scp);
			status = 0;
			pt = PathType::None;
		}
		return { pts, length };
	}
};
struct Event {
	double StartTime;
	enum {
		Tick,
		Repeat,
	} EventType;
	PointD Location;
};
struct Rect {
	int x1, y1, x2, y2;
};
struct StdObject : public HitObject {
	PointD Location;
	bool HasHold;
	bool HoldBroken;
	AudioSample ssample;
	AudioStream ssample_stream;
	AudioSample ssamplew;
	AudioStream ssamplew_stream;
	double EndTime;
	double LastHoldOff = -1;
	double Velocity = 1;
	int RepeatCount = 0;
	size_t NextEventIndex = 0;
	std::vector<Event> Events;
	SliderPath* Path = 0;
	std::vector<PointI> BodyPolygon;
	Rect CachedViewport = {};
};