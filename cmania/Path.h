#pragma once
#include <vector>
#include <stack>
#include "Defines.h"

struct PathApproximator {
	static PointD CalculateBezierPoint(const std::vector<PointD>& controlPoints, double t) {
		int n = controlPoints.size() - 1;
		double x = 0.0;
		double y = 0.0;

		for (int i = 0; i <= n; i++) {
			double coeff = std::pow(1 - t, n - i) * std::pow(t, i);
			x += coeff * controlPoints[i].X;
			y += coeff * controlPoints[i].Y;
		}
		return { x, y };
	}
	static std::vector<PointD> ApproximateBezier(const std::vector<PointD>& controlPoints) {
		std::vector<PointD> result;
		const int numSegments = 200;
		double segmentSize = 1.0 / numSegments;
		for (size_t i = 0; i < numSegments; i++) {
			double t = i * segmentSize;
			result.push_back(CalculateBezierPoint(controlPoints, t));
		}
		return result;
	}
};