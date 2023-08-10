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
			for (size_t i = 0; i < rbg.Width() * rbg.Height() * 3; i += 3) {
				auto scan0 = rbg.Scan0();
				buffer.SetPixel(x, y, { {}, { 255, (unsigned char)(scan0[i + 0] / 2), (unsigned char)(scan0[i + 1] / 2), (unsigned char)(scan0[i + 2] / 2) }, ' ' });
				x++;
				if (x >= buffer.Width) {
					x = 0;
					y++;
				}
			}
		}
	}
};
