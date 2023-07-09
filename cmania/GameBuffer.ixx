module;
#include <windows.h>
#pragma warning(disable:4996)
#pragma warning(disable:4005)
#pragma warning(disable:4267)
export module GameBuffer;
import AsyncConsole;
import Unicode;
import ConsoleText;
import <vector>;
import <string>;
import <algorithm>;

export class GameBuffer
{
public:
	struct Color
	{
		unsigned char Alpha;
		unsigned char Red;
		unsigned char Green;
		unsigned char Blue;
		bool operator==(Color b)
		{
			return *(int*)(this) == *(int*)&b;
		}
		static Color Blend(Color a, Color b)
		{
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
			result.Alpha = static_cast<unsigned char>(a.Alpha + (b.Alpha * invAlpha));

			return result;
		}
	};
	struct PixelData
	{
		Color Foreground;
		Color Background;
		unsigned int UcsChar;
	};
private:
	std::vector<PixelData> PixelBuffer;
public:
	int Width;
	int Height;
private:
	void EnsureCapacity()
	{
		auto target = Width * Height;
		if (PixelBuffer.size() < target)
		{
			PixelBuffer.resize(target);
		}
	}
	std::vector<char> outbuf;

	void ResizeConsoleBuffer()
	{
		// Retrieve the current console screen buffer information
		CONSOLE_SCREEN_BUFFER_INFO bufferInfo;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &bufferInfo);
		// Adjust the console screen buffer size
		COORD newSize;
		newSize.X = bufferInfo.srWindow.Right - bufferInfo.srWindow.Left + 1;
		newSize.Y = bufferInfo.srWindow.Bottom - bufferInfo.srWindow.Top + 1;
		SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), newSize);
	}
	void _ResizeBuffer()
	{
		CONSOLE_SCREEN_BUFFER_INFO bufferInfo;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &bufferInfo);
		Width = bufferInfo.srWindow.Right - bufferInfo.srWindow.Left + 1;
		Height = bufferInfo.srWindow.Bottom - bufferInfo.srWindow.Top + 1;
		EnsureCapacity();
		ResizeConsoleBuffer();
	}
	bool dirty_buffer = false;
	void CheckBuffer()
	{
		if (dirty_buffer)
		{
			_ResizeBuffer();
			dirty_buffer = false;
		}
	}
public:
	void ResizeBuffer()
	{
		dirty_buffer = true;
	}
private:
	void HideCursor()
	{
		WriteConsoleAsync("\u001b[?25l");
	}
public:
	void InitConsole()
	{
		SetConsoleOutputCP(65001);
		ResizeBuffer();
		HideCursor();
	}
private:
	void WriteBufferString(const char* str)
	{
		while (*str != '\0')
		{
			outbuf.emplace_back(*str);
			str++;
		}
	}
public:
	void Clear()
	{
		CheckBuffer();
		for (size_t i = 0; i < PixelBuffer.size(); i++)
		{
			PixelBuffer[i] = {};
		}
	}
	void Output()
	{
		CheckBuffer();
		outbuf.clear();
		Color LastFg{ 255,255,255,255 };
		Color LastBg{ 255,0,0,0 };
		if (Height <= 0 && Width <= 0)
			return;
		WriteBufferString("\u001b[48;2;0;0;0m\u001b[38;2;255;255;255m");
		for (size_t i = 0; i < Height; i++)
		{
			for (size_t j = 0; j < Width; j++)
			{
				PixelData dat = PixelBuffer[i * Width + j];
				if (dat.UcsChar == '\b')
					continue;
				if (dat.UcsChar == 0)
				{
					if (LastBg != Color{})
					{
						WriteBufferString("\u001b[48;2;0;0;0m");
						LastBg = {};
					}
					outbuf.emplace_back(' ');
					continue;
				}
				int len = Measure(dat.UcsChar);
				if (len == 0)
					continue;
				if (len < 0 || dat.UcsChar < 32)
				{
					outbuf.emplace_back('?');
					continue;
				}
				dat.Background = Color::Blend(Color{ 255,0,0,0 }, dat.Background);
				dat.Foreground = Color::Blend(Color{ 255,255,255,255 }, dat.Foreground);
				char commonbuf[4]{};
				if (LastFg != dat.Foreground)
				{
					WriteBufferString("\u001b[38;2;");
					itoa(dat.Foreground.Red, commonbuf, 10);
					WriteBufferString(commonbuf);
					outbuf.emplace_back(';');
					commonbuf[0] = 0; commonbuf[1] = 0; commonbuf[2] = 0; commonbuf[3] = 0;
					itoa(dat.Foreground.Green, commonbuf, 10);
					WriteBufferString(commonbuf);
					outbuf.emplace_back(';');
					commonbuf[0] = 0; commonbuf[1] = 0; commonbuf[2] = 0; commonbuf[3] = 0;
					itoa(dat.Foreground.Blue, commonbuf, 10);
					WriteBufferString(commonbuf);
					outbuf.emplace_back('m');
					LastFg = dat.Foreground;
				}
				if (LastBg != dat.Background)
				{
					WriteBufferString("\u001b[48;2;");
					itoa(dat.Background.Red, commonbuf, 10);
					WriteBufferString(commonbuf);
					outbuf.emplace_back(';');
					commonbuf[0] = 0; commonbuf[1] = 0; commonbuf[2] = 0; commonbuf[3] = 0;
					itoa(dat.Background.Green, commonbuf, 10);
					WriteBufferString(commonbuf);
					outbuf.emplace_back(';');
					commonbuf[0] = 0; commonbuf[1] = 0; commonbuf[2] = 0; commonbuf[3] = 0;
					itoa(dat.Background.Blue, commonbuf, 10);
					WriteBufferString(commonbuf);
					outbuf.emplace_back('m');
					LastBg = dat.Background;
				}
				char buf[4]{ 0 };
				int written = 0;
				Ucs4Char2Utf8(dat.UcsChar, buf, written);
				for (size_t i = 0; i < written; i++)
				{
					outbuf.emplace_back(buf[i]);
				}
			}
			outbuf.emplace_back('\n');
		}
		if (outbuf.size() >= 1)
			outbuf.erase(outbuf.end() - 1); //È¥³ýÄ©Î²µÄ»»ÐÐ
		WriteConsoleAsync(outbuf.data(), outbuf.size());
	}
	void DrawString(const std::string& text, int startX, int startY, Color fg, Color bg)
	{
		DrawString(Utf82Ucs4(text), startX, startY, fg, bg);
	}
	void DrawString(const std::wstring& text, int startX, int startY, Color fg, Color bg)
	{
		DrawString(Utf162Ucs4(text), startX, startY, fg, bg);
	}
	void DrawString(const std::u32string& text, int startX, int startY, Color fg, Color bg)
	{
		int x = startX;
		int y = startY;

		for (auto c : text)
		{
			if (c == '\n')
			{
				// Move to the next line
				x = startX;
				y++;
			}
			else
			{
				// Measure the width of the character
				int charWidth = Measure(c);
				if (x + charWidth < Width)
				{
					// Set the pixel at the current position
					SetPixel(x, y, PixelData{ fg,bg,c });
					if (charWidth > 1)
						for (size_t i = 1; i < charWidth; i++)
						{
							SetPixel(x + i, y, PixelData{ {},{},'\b' });
						}
					x += charWidth;
				}
			}
		}
	}
	PixelData GetPixel(int x, int y)
	{
		if (x < Width && y < Height && y > -1 && x > -1)
		{
			return PixelBuffer[y * Width + x];
		}
		throw std::exception("X or Y out of range.");
	}
	struct Point
	{
		int x;
		int y;
	};
	void FillPolygon(const std::vector<Point>& points, PixelData pd)
	{
		if (points.size() < 3)
			return;

		// Find the minimum and maximum y-coordinate values
		int minY = points[0].y;
		int maxY = points[0].y;
		for (const Point& point : points)
		{
			if (point.y < minY)
				minY = point.y;
			if (point.y > maxY)
				maxY = point.y;
		}

		// Iterate over each scanline
		for (int y = minY; y <= maxY; y++)
		{
			std::vector<int> intersectionX;

			// Find the intersections of the scanline with the polygon edges
			for (size_t i = 0; i < points.size(); i++)
			{
				const Point& p1 = points[i];
				const Point& p2 = points[(i + 1) % points.size()];

				if ((p1.y <= y && p2.y > y) || (p1.y > y && p2.y <= y))
				{
					if (p1.y != p2.y)
					{
						int intersectionXValue = p1.x + (y - p1.y) * (p2.x - p1.x) / (p2.y - p1.y);
						intersectionX.push_back(intersectionXValue);
					}
				}
			}

			// Sort the intersection points in ascending order
			std::sort(intersectionX.begin(), intersectionX.end());
			if (intersectionX.size() == 0)
				continue;
			// Fill the pixels between pairs of intersection points
			for (size_t i = 0; i < intersectionX.size() - 1; i += 2)
			{
				int startX = intersectionX[i];
				int endX = intersectionX[i + 1];
				DrawLineV(startX, endX, y, pd);
			}
		}
	}

	void SetPixel(int x, int y, PixelData pd)
	{
		if (x < Width && y < Height && y > -1 && x > -1)
		{
			auto& ref = PixelBuffer[y * Width + x];
			if (ref.UcsChar == '\b')
			{
				for (size_t i = x; i >= 0; i--)
				{
					auto& ref2 = PixelBuffer[y * Width + i];
					ref2.UcsChar = ' ';
					if (ref2.UcsChar != '\b')
					{
						break;
					}
				}
			}
			ref.UcsChar = pd.UcsChar;
			ref.Background = Color::Blend(ref.Background, pd.Background);
			ref.Foreground = Color::Blend(ref.Foreground, pd.Foreground);
		}
	}
	void DrawLineH(int x, int y1, int y2, PixelData pd)
	{
		if (y1 > y2)
			std::swap(y1, y2);
		if (y1 == y2)
			SetPixel(x, y1, pd);
		for (int i = y1; i < y2; i++)
		{
			SetPixel(x, i, pd);
		}
	}
	void DrawLineV(int x1, int x2, int y, PixelData pd)
	{
		if (x1 > x2)
			std::swap(x1, x2);
		if (x1 == x2)
			SetPixel(x1, y, pd);
		for (int i = x1; i < x2; i++)
		{
			SetPixel(i, y, pd);
		}
	}
	void DrawLine(int x1, int x2, int y1, int y2, PixelData pd)
	{
		if (y1 > y2)
			std::swap(y1, y2);
		if (x1 > x2)
			std::swap(x1, x2);
		auto dx = x2 - x1;
		auto dy = y2 - y1;
		if (dx == 0)
		{
			DrawLineH(x1, y1, y2, pd);
			return;
		}
		if (dy == 0)
		{
			DrawLineV(x1, x2, y1, pd);
		}
		auto k = (double)dy / dx;
		auto c = y2 - x2 * k;
		for (int i = y1; i <= y2; i++)
		{
			for (int j = x1; j <= x2; j++)
			{
				if (std::abs((j * k + c) - i) <= 0.5)
				{
					SetPixel(j, i, pd);
				}
			}
		}
	}
	void FillRect(int left, int top, int right, int bottom, PixelData pd)
	{
		if (left > right)
			std::swap(left, right);
		for (int i = left; i < right; i++)
		{
			DrawLineH(i, top, bottom, pd);
		}
	}
	void DrawRect(int left, int top, int right, int bottom, PixelData pd)
	{
		DrawLine(left, top, right - 1, top, pd);
		DrawLine(right, top, right, bottom - 1, pd);
		DrawLine(right, bottom, left + 1, bottom, pd);
		DrawLine(left, top - 1, left, bottom, pd);
	}
};