#include "SongSelectScreen.h"
#include "ScreenController.h"
#include "SettingsScreen.h"
#include "MainMenuScreen.h"
#include "LogOverlay.h"
#include "git_info.h"

class MainMenuScreen : public Screen {
	bool is_name_exists = false;
	std::wstring input_buf;
	virtual void Activate(bool y) override {
		if (y) {
			is_name_exists = !game->Settings["Name"].GetString().empty();
		}
	}
	virtual void Render(GameBuffer& buf) {
		if (!is_name_exists) {
			buf.DrawString("输入您的大名(仅用于录像):", 0, 0, {}, {});
			buf.DrawString(input_buf, 0, 1, {}, {});
			return;
		}
		buf.DrawString("Cmania " GIT_LATEST_TAG "\n\n按下 Enter 进入选歌界面\n按下 O 键进行设置", 0, 0, {}, {});
		buf.DrawString("Compiled with MSVC v" QUOTE(_MSC_VER) "(git-" GIT_COMMIT_HASH "@" GIT_COMMIT_DATE ")\nOriginal game by peppy( https://osu.ppy.sh )\nCopyright 2023-2024 telecomadm1145( https://github.com/telecomadm1145/cmania )", 0, buf.Height - 3, {}, {});
		//buf.DrawString("Loading beatmap cache...", 0, 0, {}, {});
		//buf.DrawString("正在播放", 0, 0, {}, {});
	}
	virtual void Key(KeyEventArgs kea) {
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
			if (kea.Key == ConsoleKey::Enter) {
				parent->Navigate(MakeSongSelectScreen());
			}
			if (kea.Key == ConsoleKey::O) {
				parent->Navigate(MakeSettingsScreen());
			}
		}
	}
};

Screen* MakeMainMenuScreen() {
	return new MainMenuScreen();
}
