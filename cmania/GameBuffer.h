#pragma once
#include "Unicode.h"
#include "ConsoleText.h"
#include "Defines.h"
#include <vector>
#include <string>
#include <algorithm>
#include <functional>
#include <utility>
struct Color {
	unsigned char Alpha;
	unsigned char Red;
	unsigned char Green;
	unsigned char Blue;
	bool operator==(Color b) {
		return *(int*)(this) == *(int*)&b;
	}
	static Color Blend(Color a, Color b);
	Color operator-(Color b) {
		return Color{ Alpha, (unsigned char)(Red - b.Red), (unsigned char)(Green - b.Green), (unsigned char)(Blue - b.Blue) };
	}
	Color operator+(Color b) {
		return Color{ Alpha, (unsigned char)(Red + b.Red), (unsigned char)(Green + b.Green), (unsigned char)(Blue + b.Blue) };
	}
	template <class T>
	Color operator*(T x) {
		return Color{ (unsigned char)(Alpha * x), Red, Green, Blue };
	}
	double Difference(Color b) {
		return (std::abs((Red - b.Red) / 255.0) + std::abs((Blue - b.Blue) / 255.0) + std::abs((Green - b.Green) / 255.0)) / 3.0;
	}
};
template <class T>
Color operator*(T a, Color b) {
	return b * a;
}
struct PixelData {
	Color Foreground;
	Color Background;
	unsigned int UcsChar;
};
class GameBuffer {
private:
	std::vector<PixelData> PixelBuffer;

public:
	int Width;
	int Height;
	std::function<void(const char*, size_t)> write;
	template <class func>
	GameBuffer(func write) : write(write) {
	}

private:
	void EnsureCapacity();
	void _ResizeBuffer();
	bool dirty_buffer = false;
	void CheckBuffer();

public:
	void ResizeBuffer(int width, int height);
	void HideCursor();

public:
	void InitConsole();

private:
	char* outbuf = 0;
	size_t buf_cap = 114514;
	size_t buf_i = 0;
	void WriteBufferChar(char c) {
		if (buf_i >= buf_cap) {
			auto newbuf = new char[buf_cap * 2];
			std::memcpy(newbuf, outbuf, buf_cap);
			buf_cap *= 2;
			delete outbuf;
			outbuf = newbuf;
		}
		outbuf[buf_i++] = c;
	}
	void WriteBufferString(const char* str) {
		while (*str != '\0') {
			WriteBufferChar(*str);
			str++;
		}
	}

public:
	void Clear();
	void Output();
	void DrawString(const std::string& text, int startX, int startY, Color fg, Color bg) {
		DrawString(Utf82Ucs4(text), startX, startY, fg, bg);
	}
	void DrawString(const std::wstring& text, int startX, int startY, Color fg, Color bg) {
		DrawString(Utf162Ucs4(text), startX, startY, fg, bg);
	}
	void DrawCircle(int x, int y, double sz, double width, double whratio, PixelData pd);


public:
	void FillCircle(int x, int y, double sz, double whratio, PixelData pd, int aa = 4);
	void DrawString(const std::u32string& text, int startX, int startY, Color fg, Color bg);
	PixelData GetPixel(int x, int y);
	void FillPolygon(const std::vector<PointI>& points, PixelData pd);

	void SetPixel(int x, int y, PixelData pd);
	void DrawLineH(int x, int y1, int y2, PixelData pd);
	void DrawLineV(int x1, int x2, int y, PixelData pd);
	void DrawLine(int x1, int x2, int y1, int y2, PixelData pd);
	void FillRect(int left, int top, int right, int bottom, PixelData pd);
};