#include "git_info.h"
#include "LogOverlay.h"
#include "MainMenuScreen.h"
#include "ScreenController.h"
#include "SettingsScreen.h"
#include "SongSelectScreen.h"
#include "AboutScreen.h"
#include "BeatmapDownloadingScreen.h"
#include "UI/UIUtils.h"

#ifdef _MSC_VER
#define _COMPILER_BANNER "Compiled with MSVC v" QUOTE(_MSC_VER)
#else
#define _COMPILER_BANNER "Compiled with Clang"
#endif

class MainMenuScreen : public Screen {
	bool is_name_exists = false;
	std::wstring input_buf;
	int selected_index = 0;
	const int MENU_COUNT = 4;

	virtual void Activate(bool y) override {
		if (y) {
			is_name_exists = !game->Settings["Name"].GetString().empty();
		}
	}
	virtual void Render(GameBuffer& buf) override {
		if (!is_name_exists) {
			buf.DrawString("输入您的大名(仅用于录像):", 0, 0, {}, {});
			buf.DrawString(input_buf, 0, 1, {}, {});
			return;
		}

		// Draw Title
		UI::DrawHeader(buf, " CMANIA - Console Rhythm Game", 2);

		int startY = 8;
		int btnH = 4;
		int btnW = 30;
		int startX = 5;

		UI::DrawButton(buf, "Play (Enter)", startX, startY, btnW, btnH, selected_index == 0);
		UI::DrawButton(buf, "Settings (O)", startX, startY + btnH, btnW, btnH, selected_index == 1);
		UI::DrawButton(buf, "Download Maps (D)", startX, startY + btnH * 2, btnW, btnH, selected_index == 2);
		UI::DrawButton(buf, "About (A)", startX, startY + btnH * 3, btnW, btnH, selected_index == 3);

		// Footer
		std::string footer = _COMPILER_BANNER "(git-" GIT_COMMIT_HASH "@" GIT_COMMIT_DATE ")\n";
		footer += "Original game by peppy( https://osu.ppy.sh )\n";
		footer += "Copyright 2023-2024 telecomadm1145";
		buf.DrawString(footer, 2, buf.Height - 4, UI::Color_TextDim, {});
	}
	virtual void Key(KeyEventArgs kea) override {
		if (kea.Pressed) {
			if (!is_name_exists) {
				if (kea.Key == ConsoleKey::Backspace) {
					if (input_buf.size() > 0) {
						input_buf.resize(input_buf.size() - 1);
					}
					return;
				}
				if (kea.Key == ConsoleKey::Enter) {
					auto str = Utf162Utf8(std::wstring{ input_buf.begin(), input_buf.end() });
					is_name_exists = true;
					game->Settings["Name"].SetString(str);
					game->Settings.Write();
				}
				if (kea.UnicodeChar >= 31) {
					input_buf.push_back(kea.UnicodeChar);
				}
				return;
			}

			// Navigation
			if (kea.Key == ConsoleKey::DownArrow) {
				selected_index = (selected_index + 1) % MENU_COUNT;
			}
			if (kea.Key == ConsoleKey::UpArrow) {
				selected_index = (selected_index - 1 + MENU_COUNT) % MENU_COUNT;
			}

			// Selection or Direct Hotkey
			if (kea.Key == ConsoleKey::Enter) {
				if (selected_index == 0)
					parent->Navigate(MakeSongSelectScreen());
				if (selected_index == 1)
					parent->Navigate(MakeSettingsScreen());
				if (selected_index == 2)
					parent->Navigate(MakeBeatmapDownloadingScreen());
				if (selected_index == 3)
					parent->Navigate(MakeAboutScreen());
			}

			// Hotkeys
			if (kea.Key == ConsoleKey::O) {
				parent->Navigate(MakeSettingsScreen());
			}
			if (kea.Key == ConsoleKey::A) {
				parent->Navigate(MakeAboutScreen());
			}
			if (kea.Key == ConsoleKey::D) {
				parent->Navigate(MakeBeatmapDownloadingScreen());
			}
		}
	}
};

Screen* MakeMainMenuScreen() {
	return new MainMenuScreen();
}
