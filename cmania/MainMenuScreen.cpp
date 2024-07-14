#include "git_info.h"
#include "LogOverlay.h"
#include "MainMenuScreen.h"
#include "ScreenController.h"
#include "SettingsScreen.h"
#include "SongSelectScreen.h"
#include "AboutScreen.h"
// #include "VolumeOverlay.h"

#ifdef _MSC_VER
#define _COMPILER_BANNER "Compiled with MSVC v" QUOTE(_MSC_VER)
#else
#define _COMPILER_BANNER "Compiled with Clang"
#endif

class MainMenuScreen : public Screen {
	bool is_name_exists = false;
	std::wstring input_buf;
	virtual void Activate(bool y) override {
		if (y) {
			is_name_exists = !game->Settings["Name"].GetString().empty();
			// parent->AddOverlay(MakeVolumeOverlay());
		}
	}
	virtual void Render(GameBuffer& buf) override {
		// TODO: make a proper ui for this ?
		if (!is_name_exists) {
			buf.DrawString("输入您的大名(仅用于录像):", 0, 0, {}, {});
			buf.DrawString(input_buf, 0, 1, {}, {});
			return;
		}
		buf.DrawString("Cmania\n\n[Enter] 选歌\n[O] 设置\n[D] 谱面下载\n[A] 关于", 0, 0, {}, {});
		// TODO: move it to other places
		buf.DrawString(_COMPILER_BANNER "(git-" GIT_COMMIT_HASH "@" GIT_COMMIT_DATE ")\nOriginal game by peppy( https://osu.ppy.sh )\nCopyright 2023-2024 telecomadm1145( https://github.com/telecomadm1145/cmania )", 0, buf.Height - 3, {}, {});
		// TODO: a better main menu with music player.
		// buf.DrawString("Loading beatmap cache...", 0, 0, {}, {});
		// buf.DrawString("正在播放", 0, 0, {}, {});
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
			if (kea.Key == ConsoleKey::Enter) {
				parent->Navigate(MakeSongSelectScreen());
			}
			if (kea.Key == ConsoleKey::O) {
				parent->Navigate(MakeSettingsScreen());
			}
			if (kea.Key == ConsoleKey::A) {
				parent->Navigate(MakeAboutScreen());
			}
		}
	}
};

Screen* MakeMainMenuScreen() {
	return new MainMenuScreen();
}
