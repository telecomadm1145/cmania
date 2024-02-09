#include "GameBuffer.h"
#include <concepts>
template <std::integral Int, typename CharT>
inline void itoa_2(Int num, CharT* out) {
	int j = 0;
	if (num == 0) {
		out[j++] = '0';
		out[j++] = 0;
		return;
	}
	Int num2 = num;
	while (num2 > 0) {
		num2 /= 10;
		j++;
	}
	out[j] = 0;
	j--;
	while (num) {
		out[j--] = "0123456789"[num % 10];
		num /= 10;
	}
}
inline auto Sqr(auto x) {
	return x * x;
}
void GameBuffer::EnsureCapacity() {
	Width = o_w;
	Height = o_h;
	auto target = Width * Height;
	if (PixelBuffer.size() < target) {
		PixelBuffer.resize(target);
	}
}
void GameBuffer::_ResizeBuffer() {
	EnsureCapacity();
}
void GameBuffer::CheckBuffer() {
	if (dirty_buffer) {
		_ResizeBuffer();
		dirty_buffer = false;
	}
}
void GameBuffer::ResizeBuffer(int width, int height) {
	o_w = width;
	o_h = height;
	dirty_buffer = true;
}
void GameBuffer::HideCursor() {
	constexpr char buf[] = "\u001b[?25l";
	write(buf, sizeof(buf));
}
void GameBuffer::InitConsole() {
	HideCursor();
}
void GameBuffer::Clear() {
	CheckBuffer();
	for (size_t i = 0; i < PixelBuffer.size(); i++) {
		PixelBuffer[i] = {};
	}
}

void GameBuffer::Output() {
	CheckBuffer();
	if (outbuf == 0) {
		outbuf = new char[buf_cap];
	}
	buf_i = 0;
	Color LastFg{ 255, 255, 255, 255 };
	Color LastBg{ 255, 0, 0, 0 };
	if (Height <= 0 && Width <= 0)
		return;
	WriteBufferString("\u001b[H\u001b[48;2;0;0;0m\u001b[38;2;255;255;255m");
	for (size_t i = 0; i < Height; i++) {
		for (size_t j = 0; j < Width; j++) {
			PixelData dat = PixelBuffer[i * Width + j];
			if (dat.UcsChar == '\b')
				continue;
			if (dat.UcsChar == 0) {
				if (LastBg != Color{}) {
					WriteBufferString("\u001b[48;2;0;0;0m");
					LastBg = {};
				}
				WriteBufferChar(' ');
				continue;
			}
			int len = Measure(dat.UcsChar);
			if (len == 0)
				continue;
			if (len < 0 || dat.UcsChar < 32) {
				WriteBufferChar('?');
				continue;
			}
			dat.Background = Color::Blend(Color{ 255, 0, 0, 0 }, dat.Background);
			if (dat.Foreground != Color{})
				dat.Foreground = Color::Blend(dat.Background, dat.Foreground);
			else
				dat.Foreground = { 255, 255, 255, 255 };
			char commonbuf[4]{};
			if (LastFg != dat.Foreground) {
				WriteBufferString("\u001b[38;2;");
				itoa_2(dat.Foreground.Red, commonbuf);
				WriteBufferString(commonbuf);
				WriteBufferChar(';');
				std::memset(commonbuf, 0, 4);
				itoa_2(dat.Foreground.Green, commonbuf);
				WriteBufferString(commonbuf);
				WriteBufferChar(';');
				std::memset(commonbuf, 0, 4);
				itoa_2(dat.Foreground.Blue, commonbuf);
				WriteBufferString(commonbuf);
				WriteBufferChar('m');
				LastFg = dat.Foreground;
			}
			if (LastBg != dat.Background) {
				WriteBufferString("\u001b[48;2;");
				itoa_2(dat.Background.Red, commonbuf);
				WriteBufferString(commonbuf);
				WriteBufferChar(';');
				std::memset(commonbuf, 0, 4);
				itoa_2(dat.Background.Green, commonbuf);
				WriteBufferString(commonbuf);
				WriteBufferChar(';');
				std::memset(commonbuf, 0, 4);
				itoa_2(dat.Background.Blue, commonbuf);
				WriteBufferString(commonbuf);
				WriteBufferChar('m');
				LastBg = dat.Background;
			}
			char buf[4]{ 0 };
			int written = 0;
			Ucs4Char2Utf8(dat.UcsChar, buf, written);
			for (size_t i = 0; i < written; i++) {
				WriteBufferChar(buf[i]);
			}
		}
		WriteBufferChar('\n');
	}
	if (buf_i >= 1)
		buf_i--;
	write(outbuf, buf_i);
}

void GameBuffer::DrawCircle(float x, float y, float sz, float width, float whratio, PixelData pd) {
	const auto aa = 4;
	sz /= 2;
	double radius = sz / 2;
	double sr = Sqr(radius) * aa;
	int r2 = (int)(sz + 1) * aa;
	int r3 = (int)((sz + 1) * whratio * aa);
	PixelData pd2 = pd;
	pd2.Background = pd2.Background * (1.0 / aa);
	pd2.Foreground = pd2.Foreground * (1.0 / aa);
	for (int i = -r2; i <= r2; i++) // y
	{
		for (int j = -r3; j <= r3; j++) { // x
			float ry = y + i / aa;
			float rx = x + j / aa;
			double dist = Sqr((double)i / aa) + Sqr((double)j / whratio / aa);
			if (dist <= sr + width * aa && dist >= sr - width * aa) {
				SetPixelF(rx, ry, pd2);
			}
		}
	}
}

void GameBuffer::FillCircle(float x, float y, float sz, float whratio, PixelData pd, int) {
	sz /= 2;
	const int aa = 4;
	double radius = sz / 2;
	double sr = Sqr(radius) * aa;
	int r2 = (int)sz * aa;
	int r3 = (int)(sz * whratio * aa);
	PixelData pd2 = pd;
	pd2.Background = pd2.Background * (1.0 / aa);
	pd2.Foreground = pd2.Foreground * (1.0 / aa);
	for (int i = -r2; i <= r2; i++) // y
	{
		for (int j = -r3; j <= r3; j++) { // x
			if (i > -aa && i < 0)
				continue;
			if (j > -aa && j < 0)
				continue;
			float ry = y + i / aa;
			float rx = x + j / aa;
			double dist = Sqr((double)i / aa) + Sqr((double)j / whratio / aa);
			if (dist <= sr) {
				SetPixelF(rx, ry, pd2);
			}
		}
	}
}

void GameBuffer::DrawString(const std::u32string& text, int startX, int startY, Color fg, Color bg) {
	int x = startX;
	int y = startY;

	for (auto c : text) {
		if (c == '\n') {
			// Move to the next line
			x = startX;
			y++;
		}
		else {
			// Measure the width of the character
			if (c == '\t') {
				x++;
				continue;
			}
			int charWidth = Measure(c);
			if (x + charWidth <= b_right) {
				// Set the pixel at the current position
				SetPixel(x, y, PixelData{ fg, bg, c });
				if (charWidth > 1)
					for (int i = 1; i < charWidth; i++) {
						SetPixel(x + i, y, PixelData{ {}, {}, '\b' });
					}
				x += charWidth;
			}
		}
	}
}

PixelData GameBuffer::GetPixel(int x, int y) {
	if (x < Width && y < Height && y > -1 && x > -1) {
		return PixelBuffer[y * Width + x];
	}
	throw std::out_of_range("X or Y out of range.");
}

void GameBuffer::FillPolygon(const std::vector<PointI>& points, PixelData pd) {
	if (points.size() < 3)
		return;

	// Find the minimum and maximum y-coordinate values
	int minY = points[0].Y;
	int maxY = points[0].Y;
	for (const auto& point : points) {
		if (point.Y < minY)
			minY = point.Y;
		if (point.Y > maxY)
			maxY = point.Y;
	}

	// Iterate over each scanline
	for (int y = minY; y <= maxY; y++) {
		std::vector<int> intersectionX;

		// Find the intersections of the scanline with the polygon edges
		for (size_t i = 0; i < points.size(); i++) {
			const auto& p1 = points[i];
			const auto& p2 = points[(i + 1) % points.size()];

			if ((p1.Y <= y && p2.Y > y) || (p1.Y > y && p2.Y <= y)) {
				if (p1.Y != p2.Y) {
					int intersectionXValue = p1.X + (y - p1.Y) * (p2.X - p1.X) / (p2.Y - p1.Y);
					intersectionX.push_back(intersectionXValue);
				}
			}
		}

		// Sort the intersection points in ascending order
		std::sort(intersectionX.begin(), intersectionX.end());
		if (intersectionX.size() == 0)
			continue;
		// Fill the pixels between pairs of intersection points
		for (size_t i = 0; i < intersectionX.size() - 1; i += 2) {
			int startX = intersectionX[i];
			int endX = intersectionX[i + 1];
			DrawLineV(startX, endX, y, pd);
		}
	}
}

void GameBuffer::SetPixel(int x, int y, PixelData pd) {
	if ((x < Width && y < Height && y > -1 && x > -1) &&
		(x <= b_right && y <= b_bottom && y >= b_top && x >= b_left)) {
		auto& ref = PixelBuffer[y * Width + x];
		if (ref.UcsChar == '\b') {
			for (int i = x; i >= 0; i--) {
				auto& ref2 = PixelBuffer[y * Width + i];
				ref2.UcsChar = ' ';
				if (ref2.UcsChar != '\b') {
					break;
				}
			}
		}
		if (pd.UcsChar != '\1')
			ref.UcsChar = pd.UcsChar;
		ref.Background = Color::Blend(ref.Background, pd.Background);
		ref.Foreground = Color::Blend(ref.Foreground, pd.Foreground);
	}
}

void GameBuffer::SetPixelF(float x, float y, PixelData pd) {
	float x_i = 0.0;
	float x_f = std::modf(x, &x_i);
	float y_i = 0.0;
	float y_f = std::modf(y, &y_i);
	// Ratio

	// (1-x_f)*(1-y_f)	x_f*(1-y_f)
	// (1-x_f)*y_f		x_f*y_f
	SetPixel(x_i, y_i, pd * ((1 - x_f) * (1 - y_f)));
	SetPixel(x_i + 1, y_i, pd * (x_f * (1 - y_f)));
	SetPixel(x_i, y_i + 1, pd * ((1 - x_f) * y_f));
	SetPixel(x_i + 1, y_i + 1, pd * (x_f * y_f));
}

void GameBuffer::DrawLineH(float x, float y1, float y2, PixelData pd) {
	if (y1 > y2)
		std::swap(y1, y2);
	if (y1 == y2)
		SetPixel(x, y1, pd);
	y1 = std::max((float)0.0, y1);
	y2 = std::min(y2, (float)Height);
	for (int i = y1; i < y2; i++) {
		SetPixel(x, i, pd);
	}
}

void GameBuffer::DrawLineV(float x1, float x2, float y, PixelData pd) {
	if (x1 > x2)
		std::swap(x1, x2);
	float x1_i = 0.0;
	float x1_f = std::modf(x1, &x1_i);
	float x2_i = 0.0;
	float x2_f = std::modf(x2, &x2_i);
	float y_i = 0.0;
	float y_f = std::modf(y, &y_i);

	{
		float line0_weight = 1 - y_f;
		if (line0_weight > zero_v<float>) {
			PixelData pd2 = pd * line0_weight;
			if (x1_i == x2_i) {
				float weight = std::abs(x2_f - x1_f);
				SetPixel(x1_i, y_i, pd2 * weight);
			}
			else {
				float weight = 1 - x1_f;
				SetPixel(x1_i, y_i, pd2 * weight);
				float weight2 = x2_f;
				SetPixel(x2_i, y_i, pd2 * weight2);
				for (int i = x1_i + 1; i < x2_i; i++) {
					SetPixel(i, y_i, pd2);
				}
			}
		}
	}

	{
		float line1_weight = y_f;
		PixelData pd2 = pd * line1_weight;
		if (line1_weight > zero_v<float>) {
			if (x1_i == x2_i) {
				float weight = std::abs(x2_f - x1_f);
				SetPixel(x1_i, y_i + 1, pd2 * weight);
			}
			else {
				float weight = 1 - x1_f;
				SetPixel(x1_i, y_i + 1, pd2 * weight);
				float weight2 = x2_f;
				SetPixel(x2_i, y_i + 1, pd2 * weight2);
				for (int i = x1_i + 1; i < x2_i; i++) {
					SetPixel(i, y_i + 1, pd2);
				}
			}
		}
	}
}
/*
void GameBuffer::DrawLineH(int x, int y1, int y2, PixelData pd) {
	if (y1 > y2)
		std::swap(y1, y2);
	if (y1 == y2)
		SetPixel(x, y1, pd);
	y1 = std::max(0, y1);
	y2 = std::min(y2, Height);
	for (int i = y1; i < y2; i++) {
		SetPixel(x, i, pd);
	}
}

void GameBuffer::DrawLineV(int x1, int x2, int y, PixelData pd) {
	if (x1 > x2)
		std::swap(x1, x2);
	if (x1 == x2)
		SetPixel(x1, y, pd);
	x1 = std::max(0, x1);
	x2 = std::min(x2, Width);
	for (int i = x1; i < x2; i++) {
		SetPixel(i, y, pd);
	}
}
*/
void GameBuffer::FillRect(float left, float top, float right, float bottom, PixelData pd) {
	if (top > bottom)
		std::swap(top, bottom);
	left = std::clamp(left, 0.0f, (float)Width);
	right = std::clamp(right, 0.0f, (float)Width);
	top = std::clamp(top, 0.0f, (float)Height);
	bottom = std::clamp(bottom, 0.0f, (float)Height);
	float y1_i = 0.0;
	float y1_f = std::modf(top, &y1_i);
	float y2_i = 0.0;
	float y2_f = std::modf(bottom, &y2_i);
	if (y1_i == y2_i) {
		float weight = std::abs(y2_f - y1_f);
		DrawLineV(left, right, y1_i, pd * weight);
		return;
	}
	else {
		DrawLineV(left, right, y1_i, pd * (1 - y1_f));
		DrawLineV(left, right, y2_i, pd * (y1_f));
		for (int i = y1_i + 1; i < y2_i; i++) {
			DrawLineV(left, right, i, pd);
		}
	}
}

Color Color::Blend(Color a, Color b) {
	if (a == Color{})
		return b;
	if (b == Color{})
		return a;
	Color result;
	float alpha = static_cast<float>(b.Alpha) / 255.0f;
	float invAlpha = 1.0f - alpha;

	result.Red = static_cast<unsigned char>((a.Red * invAlpha) + (b.Red * alpha));
	result.Green = static_cast<unsigned char>((a.Green * invAlpha) + (b.Green * alpha));
	result.Blue = static_cast<unsigned char>((a.Blue * invAlpha) + (b.Blue * alpha));
	auto sum = a.Alpha + b.Alpha;
	if (sum > 255)
		sum = 255;
	result.Alpha = static_cast<unsigned char>(sum);

	return result;
}
