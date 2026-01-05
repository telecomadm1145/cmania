#include "ScreenController.h"
#include <string>
#include "SpeedSettingScreen.h"
#include <vector>
#include "SettingsScreen.h"
#include "OpenFileDialog.h"
#include <filesystem>
#include "UI/UIUtils.h"

class SettingsScreen : public Screen {
	class KeyBindingSetupScreen : public Screen {
	private:
		int i = 0;
		virtual void Render(GameBuffer& buf) {
			auto set = game->Settings["KeyBinds"];
			std::string prompt;
			if (i >= 18) {
				prompt = "已完成,Esc返回";
			}
			else {
				prompt = "请输入你欲设置的键,Esc返回并保存,Tab复原(现在是第";
				prompt += std::to_string(i);
				prompt += "个键)\n";
				auto t = set.GetArray<ConsoleKey>();
				int p = 0;
				int d = 0;
				for (auto j = t; j < t + 18; j++) {
					auto text = std::to_string((int)*j);
					if ((int)*j >= 32 && (int)*j <= 'Z') {
						text = '(';
						text += (char)*j;
						text += ')';
					}
					prompt += text;
					prompt += ' ';
					if (j - t == i)
						d = p;
					p += text.size() + 1;
				}
				prompt += '\n';
				prompt.resize(prompt.size() + d, ' ');
				prompt += '^';
			}
			buf.DrawString(prompt, 0, 0, {}, {});
		}
		virtual void Key(KeyEventArgs kea) {
			auto t = game->Settings["KeyBinds"].GetArray<ConsoleKey>();
			if (kea.Pressed) {
				if (kea.Key == ConsoleKey::Escape) {
					game->Settings.Write();
					parent->Back();
					return;
				}
				if (kea.Key == ConsoleKey::Tab) {
					game->Settings.Read();
					return;
				}
				if (kea.Key == ConsoleKey::Backspace) {
					t[i] = (ConsoleKey)'?';
					return;
				}
				if (kea.Key == ConsoleKey::LeftArrow) {
					if (i > 0)
						i--;
					return;
				}
				if (kea.Key == ConsoleKey::RightArrow) {
					if (i < 17)
						i++;
					return;
				}
				if (i < 17)
					if (kea.UnicodeChar >= 32) {
						auto repeating = false;
						for (auto o = t; o < t + 18; o++) {
							if (*o == kea.Key && o - t != i)
								repeating = true;
						}
						if (!repeating) {
							t[i] = kea.Key;
							i++;
						}
					}
			}
		}
	};
	int selected_index = 0;
	const int SETTINGS_COUNT = 10; // Number of items

	virtual void Render(GameBuffer& buf) {
		UI::DrawHeader(buf, " Settings", 1);

		int y = 5;
		int gap = 2; // Spacing between items

		// 0. Jump Helper
		UI::DrawToggle(buf, "Jump Helper (J)", game->Settings["JumpHelper"].Get<bool>(), 2, y, 40, selected_index == 0);
		y += gap;

		// 1. No Beatmap Hitsounds
		UI::DrawToggle(buf, "Disable Beatmap Hitsounds (H)", game->Settings["NoBmpHs"].Get<bool>(), 2, y, 40, selected_index == 1);
		y += gap;

		// 2. Windows Terminal Mode
		UI::DrawToggle(buf, "Windows Terminal Mode (W)", game->Settings["WtMode"].Get<bool>(), 2, y, 40, selected_index == 2);
		y += gap;

		// 3. No Background
		UI::DrawToggle(buf, "Disable Background (B)", game->Settings["NoBg"].Get<bool>(), 2, y, 40, selected_index == 3);
		y += gap;

		// 4. Tail Hitsounds
		UI::DrawToggle(buf, "Tail Hitsounds (T)", game->Settings["TailHs"].Get<bool>(), 2, y, 40, selected_index == 4);
		y += gap;

		// 5. Low Performance Mode
		UI::DrawToggle(buf, "Low Performance Mode (F)", game->Settings["MyCompSuck"].Get<bool>(), 2, y, 40, selected_index == 5);
		y += gap;

		// 6. Offset
		std::string offStr = "Offset: " + std::to_string(game->Settings["Offset"].Get<double>()) + "ms (+/-)";
		UI::DrawButton(buf, offStr, 2, y, 46, 1, selected_index == 6);
		y += gap;

		// 7. Speed Wizard
		UI::DrawButton(buf, "Speed Setting Wizard (S)", 2, y, 40, 1, selected_index == 7);
		y += gap;

		// 8. Songs Path
		std::string pathStr = "Songs Path (R): " + game->Settings["SongsPath"].GetString();
		if (pathStr.length() > 46)
			pathStr = pathStr.substr(0, 43) + "...";
		UI::DrawButton(buf, pathStr, 2, y, 46, 1, selected_index == 8);
		y += gap;

		// 9. Key Bindings
		UI::DrawButton(buf, "Key Bindings (Y)", 2, y, 40, 1, selected_index == 9);
		y += gap;

		// Tips
		buf.DrawString("Use Up/Down to Navigate, Enter to Toggle/Select", 2, buf.Height - 3, UI::Color_TextDim, {});
		buf.DrawString("Esc to Back", 2, buf.Height - 2, UI::Color_TextDim, {});
	}

	void ToggleBool(const char* key) {
		game->Settings[key].Set(!game->Settings[key].Get<bool>());
		game->Settings.Write();
	}

	virtual void Key(KeyEventArgs kea) {
		if (kea.Pressed) {
			if (kea.Key == ConsoleKey::Escape) {
				parent->Back();
				return;
			}
			if (kea.Key == ConsoleKey::DownArrow) {
				selected_index = (selected_index + 1) % SETTINGS_COUNT;
				return;
			}
			if (kea.Key == ConsoleKey::UpArrow) {
				selected_index = (selected_index - 1 + SETTINGS_COUNT) % SETTINGS_COUNT;
				return;
			}

			// Enter handling based on selection
			if (kea.Key == ConsoleKey::Enter) {
				switch (selected_index) {
				case 0:
					ToggleBool("JumpHelper");
					return;
				case 1:
					ToggleBool("NoBmpHs");
					return;
				case 2:
					ToggleBool("WtMode");
					return;
				case 3:
					ToggleBool("NoBg");
					return;
				case 4:
					ToggleBool("TailHs");
					return;
				case 5:
					ToggleBool("MyCompSuck");
					return;
				case 6: // Offset - special handling, maybe dialog? For now just hint usage
					return;
				case 7:
					parent->Navigate(MakeSpeedSettingScreen());
					return;
				case 8: // Songs Path
					parent->Navigate(PickFile(
						"请选择新的Songs文件夹...", [this](std::filesystem::path pth) {
							std::filesystem::remove("Songs.bin");
							game->Settings["SongsPath"].SetArray(pth.string().c_str(), pth.string().size());
							game->Settings.Write();
						},
						{}, true, game->Settings["SongsPath"].GetString()));
					return;
				case 9:
					parent->Navigate(new KeyBindingSetupScreen());
					return;
				}
			}

			// Direct hotkeys (Legacy support + Offset adjustment)
			if (kea.Key == ConsoleKey::J) {
				selected_index = 0;
				ToggleBool("JumpHelper");
				return;
			}
			if (kea.Key == ConsoleKey::H) {
				selected_index = 1;
				ToggleBool("NoBmpHs");
				return;
			}
			if (kea.Key == ConsoleKey::W) {
				selected_index = 2;
				ToggleBool("WtMode");
				return;
			}
			if (kea.Key == ConsoleKey::B) {
				selected_index = 3;
				ToggleBool("NoBg");
				return;
			}
			if (kea.Key == ConsoleKey::T) {
				selected_index = 4;
				ToggleBool("TailHs");
				return;
			}
			if (kea.Key == ConsoleKey::F) {
				selected_index = 5;
				ToggleBool("MyCompSuck");
				return;
			}
			if (kea.Key == ConsoleKey::S) {
				selected_index = 7;
				parent->Navigate(MakeSpeedSettingScreen());
				return;
			}
			if (kea.Key == ConsoleKey::R) {
				selected_index = 8; /* ... logic duplicated, maybe simplify? */
				parent->Navigate(PickFile(
					"请选择新的Songs文件夹...", [this](std::filesystem::path pth) {
						std::filesystem::remove("Songs.bin");
						game->Settings["SongsPath"].SetArray(pth.string().c_str(), pth.string().size());
						game->Settings.Write();
					},
					{}, true, game->Settings["SongsPath"].GetString()));
				return;
			}
			if (kea.Key == ConsoleKey::Y) {
				selected_index = 9;
				parent->Navigate(new KeyBindingSetupScreen());
				return;
			}

			if (kea.UnicodeChar == '+' || kea.UnicodeChar == '-' || (selected_index == 6 && (kea.Key == ConsoleKey::LeftArrow || kea.Key == ConsoleKey::RightArrow))) {
				auto movement = 1.0;
				if (HasFlag(kea.KeyState, ControlKeyState::Capslock)) {
					movement = 10;
				}
				if (HasFlag(kea.KeyState, ControlKeyState::Shift)) {
					movement = 0.1;
				}

				bool decrease = false;
				if (kea.UnicodeChar == '-')
					decrease = true;
				if (selected_index == 6 && kea.Key == ConsoleKey::LeftArrow)
					decrease = true;

				if (decrease) {
					movement = -movement;
				}

				// Only apply if +, - or (Selected=Offset AND ArrowKeys)
				if (kea.UnicodeChar == '+' || kea.UnicodeChar == '-' || selected_index == 6) {
					game->Settings["Offset"].Set(game->Settings["Offset"].Get<double>() + movement);
					game->Settings.Write();
				}
				return;
			}
		}
	}
};

Screen* MakeSettingsScreen() {
	return new SettingsScreen();
}
