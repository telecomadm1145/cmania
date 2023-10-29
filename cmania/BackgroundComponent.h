#pragma once
#include "GameBuffer.h"
#include "StbImage.h"
#include <filesystem>
class BackgroundComponent {
	Image rbg;
	Image bg;

public:
	void LoadBackground(std::filesystem::path path) {
		auto pth = path.string();
		bg = Image(pth.c_str());
		rbg = Image{};
	}
	void LoadBackground(std::string str) {
		bg = Image(str.c_str());
		rbg = Image{};
	}
	void LoadBackground(const char* str) {
		bg = Image(str);
		rbg = Image{};
	}
	void Render(GameBuffer& buffer) {
		if (bg.Scan0() != 0) {
			if (rbg.Scan0() == 0 || rbg.Height() != buffer.Height || rbg.Width() != buffer.Width) {
				rbg = bg.Resize(buffer.Width, buffer.Height);
			}
			int x = 0;
			int y = 0;
			Color clr;
			for (size_t i = 0; i < rbg.Width() * rbg.Height() * 3; i += 3) {
				auto scan0 = rbg.Scan0();
				Color clr2{ 255, (unsigned char)(scan0[i + 0] / 5), (unsigned char)(scan0[i + 1] / 5), (unsigned char)(scan0[i + 2] / 5) };
				if (clr.Difference(clr2) > 0.01)
				{
					clr = clr2;
				}
				buffer.SetPixel(x, y, { {}, clr, ' ' });
				x++;
				if (x >= buffer.Width) {
					x = 0;
					y++;
				}
			}
		}
	}
};
