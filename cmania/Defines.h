#pragma once
#include <cmath>
template <class T>
struct Vector {
	T X = T{};
	T Y = T{};
	auto Length() {
		return sqrt(X * X + Y * Y);
	}
	auto SquaredLength() {
		return X * X + Y * Y;
	}
	Vector operator+(Vector b) {
		return { X + b.X, Y + b.Y };
	}
	Vector operator-(Vector b) {
		return { X - b.X, Y - b.Y };
	}
	Vector operator*(T b) {
		return { X * b, Y * b };
	}
	Vector operator/(T b) {
		return { X / b, Y / b };
	}
	T DotProduct(Vector b) {
		return X * b.X + Y * b.Y;
	}
	T CrossProduct(Vector b) {
		return X * b.Y - Y * b.X;
	}
	void Normalize() {
		T len = Length();
		X /= len;
		Y /= len;
	}
	void Rotate(double angle) {
		T newX = X * cos(angle) - Y * sin(angle);
		T newY = X * sin(angle) + Y * cos(angle);
		X = newX;
		Y = newY;
	}
	bool operator==(Vector b) const {
		return X == b.X && Y == b.Y;
	}

	bool operator!=(Vector b) const {
		return !(*this == b);
	}

	bool operator<(Vector b) const {
		return Length() < b.Length();
	}

	bool operator>(Vector b) const {
		return Length() > b.Length();
	}

	bool operator<=(Vector b) const {
		return Length() <= b.Length();
	}

	bool operator>=(Vector b) const {
		return Length() >= b.Length();
	}
};
template <class T>
struct Point {
	friend Vector<T>;
	Point() {}
	Point(T X, T Y) : X(X), Y(Y) {}
	Point(Vector<T> vec) : X(vec.X), Y(vec.Y) {
	}
	T X = T{};
	T Y = T{};

	auto Length() {
		return sqrt(X * X + Y * Y);
	}
	auto SquaredLength() {
		return X * X + Y * Y;
	}
	Point operator+(Point b) {
		return { X + b.X, Y + b.Y };
	}
	Point operator-(Point b) {
		return { X - b.X, Y - b.Y };
	}
	Point operator*(T b) {
		return { X * b, Y * b };
	}
	Point operator/(T b) {
		return { X / b, Y / b };
	}
	operator Vector<T>() {
		return { X, Y };
	}
	bool operator==(Point b) const {
		return X == b.X && Y == b.Y;
	}

	bool operator!=(Point b) const {
		return !(*this == b);
	}
};
using PointD = Point<double>;
using VectorD = Vector<double>;

#include <numeric>
inline double variance(double mean, const std::vector<double>& values) {
	if (values.size() < 2)
		return 0;

	auto sq_diff_sum = std::accumulate(values.begin(), values.end(), 0.0,
		[mean](double sum, double val) {
			double diff = val - mean;
			return sum + diff * diff;
		});

	return sq_diff_sum / (values.size() - 1);
}