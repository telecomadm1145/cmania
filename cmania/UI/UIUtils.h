#pragma once
#include "../GameBuffer.h"
#include <string>

namespace UI {
	// Colors
	static const Color Color_Primary = { 255, 0, 120, 215 };   // Brand Blue
	static const Color Color_Accent = { 255, 255, 64, 129 };   // Accent Pink
	static const Color Color_Text = { 255, 240, 240, 240 };	   // White-ish
	static const Color Color_TextDim = { 255, 160, 160, 160 }; // Grey
	static const Color Color_Bg = { 255, 20, 20, 20 };		   // Dark Grey
	static const Color Color_BgHover = { 255, 40, 40, 40 };	   // Lighter Grey

	inline void DrawHeader(GameBuffer& buf, const std::string& text, int y) {
		buf.FillRect(0, y, buf.Width, y + 3, { {}, Color_Primary, ' ' });
		buf.DrawString(text, 2, y + 1, Color_Text, {});
	}

	inline void DrawButton(GameBuffer& buf, const std::string& text, int x, int y, int w, int h, bool selected) {
		Color bg = selected ? Color_BgHover : Color{ 0, 0, 0, 0 };
		Color fg = selected ? Color_Accent : Color_TextDim;

		if (selected) {
			buf.FillRect(x, y, x + w, y + h, { {}, bg, ' ' });
			buf.DrawLineV(x, x, y + h, { {}, Color_Accent, ' ' }); // Selection indicator
		}

		int textX = x + 2;
		int textY = y + (h / 2); // Vertically centered roughly
		buf.DrawString(text, textX, textY, fg, {});
	}

	inline void DrawToggle(GameBuffer& buf, const std::string& label, bool value, int x, int y, int w, bool selected) {
		Color fg = selected ? Color_Text : Color_TextDim;
		buf.DrawString(label, x + 2, y, fg, {});

		std::string status = value ? "[ON] " : "[OFF]";
		Color statusColor = value ? Color_Primary : Color_TextDim;

		buf.DrawString(status, x + w - 8, y, statusColor, {});

		if (selected) {
			buf.DrawString(">", x, y, Color_Accent, {});
		}
	}

	inline void DrawProgressBar(GameBuffer& buf, int x, int y, int w, float progress, Color fg, Color bg) {
		buf.FillRect(x, y, x + w, y + 1, { {}, bg, ' ' });
		int fillW = (int)(w * progress);
		if (fillW > 0) {
			buf.FillRect(x, y, x + fillW, y + 1, { {}, fg, ' ' });
		}
	}
}
