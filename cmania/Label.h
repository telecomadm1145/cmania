#pragma once
#include "GameBuffer.h"
/// <summary>
/// 显示指定信息的控件(?)类
/// </summary>
class Label {
public:
	int X, Y;
	int Width, Height;
	Color Foreground;
	std::u32string Text;
	void Render(GameBuffer& buffer) {
		if (Width == 0 || Height == 0)
			return;
		int cx, cy;
		cy = Y;
		cx = X;
		bool cr = false;
		for (size_t i = 0; i < Text.size(); i++) {
			if (cy > Y + Height)
			{
				return;
			}
			unsigned int chr = Text[i];
			if (cr && chr == '\n') {
				continue;
			}
			if (chr == '\r' || chr == '\n') {
				cx = X;
				cy++;
				continue;
			}
			int sz = Measure(chr);
			if (sz <= 0)
				continue;
			if (cx + sz >= X + Width && cy == Y + Height) {
				buffer.SetPixel(X + Width - 1, cy, { Foreground, {}, '>' });
				return;
			}
			if (cx + sz > X + Width) {
				cx = X;
				cy++;
				i--;
				continue;
			}
			buffer.SetPixel(cx, cy, PixelData{ Foreground, {}, chr });
			for (size_t j = 1; j < sz; j++) {
				buffer.SetPixel(cx + j, cy, PixelData{ Foreground, {}, '\b' });
			}
			cx += sz;
		}
	}
};