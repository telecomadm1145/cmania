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
	static VectorD catmullFindPoint(const VectorD& vec1, const VectorD& vec2, const VectorD& vec3, const VectorD& vec4, float t) {
		float num = t * t;
		float num2 = t * num;
		VectorD result;
		result.X = 0.5f * (2.0f * vec2.X + (-vec1.X + vec3.X) * t + (2.0f * vec1.X - 5.0f * vec2.X + 4.0f * vec3.X - vec4.X) * num + (-vec1.X + 3.0f * vec2.X - 3.0f * vec3.X + vec4.X) * num2);
		result.Y = 0.5f * (2.0f * vec2.Y + (-vec1.Y + vec3.Y) * t + (2.0f * vec1.Y - 5.0f * vec2.Y + 4.0f * vec3.Y - vec4.Y) * num + (-vec1.Y + 3.0f * vec2.Y - 3.0f * vec3.Y + vec4.Y) * num2);
		return result;
	}

	static std::vector<VectorD> ApproximateCatmull(const std::vector<PointD>& controlPoints) {
		std::vector<VectorD> list((controlPoints.size() - 1) * 50 * 2);
		for (size_t i = 0; i < controlPoints.size() - 1; i++) {
			VectorD right = (i > 0) ? controlPoints[i - 1] : controlPoints[i];
			VectorD vector = controlPoints[i];
			VectorD Vector2 = (i < controlPoints.size() - 1) ? (VectorD)controlPoints[i + 1] : VectorD{ vector.X + vector.X - right.X, vector.Y + vector.Y - right.Y };
			VectorD vector3 = (i < controlPoints.size() - 2) ? (VectorD)controlPoints[i + 2] : VectorD{ Vector2.X + Vector2.X - vector.X, Vector2.Y + Vector2.Y - vector.Y };
			for (int j = 0; j < 50; j++) {
				list.push_back(catmullFindPoint(right, vector, Vector2, vector3, j / 50.0f));
				list.push_back(catmullFindPoint(right, vector, Vector2, vector3, (j + 1) / 50.0f));
			}
		}
		return list;
	}
};