#include "ScreenController.h"
#include <string>
#include "SpeedSettingScreen.h"
#include <vector>
#include "SettingsScreen.h"
class SettingsScreen : public Screen {
	int scroll = 0;
	virtual void Render(GameBuffer& buf) {
		std::string line = "选项(用上下箭头翻页):\n";
		line.push_back('[');
		line.push_back(game->Settings["JumpHelper"].Get<bool>() ? 'x' : ' ');
		line.push_back(']');
		line.append("双押辅助(J)\n");
		line.push_back('[');
		line.push_back(game->Settings["NoBmpHs"].Get<bool>() ? 'x' : ' ');
		line.push_back(']');
		line.append("关闭谱面击打声(H)\n");
		line.push_back('[');
		line.push_back(game->Settings["WtMode"].Get<bool>() ? 'x' : ' ');
		line.push_back(']');
		line.append("Windows Terminal模式(关闭长按计分)(W)\n");
		line.push_back('[');
		line.push_back(game->Settings["NoBg"].Get<bool>() ? 'x' : ' ');
		line.push_back(']');
		line.append("关闭背景(B)\n");
		line.push_back('[');
		line.push_back(game->Settings["TailHs"].Get<bool>() ? 'x' : ' ');
		line.push_back(']');
		line.append("长按尾击打声(T)\n");
		line.push_back('[');
		line.push_back(game->Settings["MyCompSuck"].Get<bool>() ? 'x' : ' ');
		line.push_back(']');
		line.append("低性能模式(F)\n");
		line.append("偏移:");
		line.append(std::to_string(game->Settings["Offset"].Get<double>()));
		line.append("ms(+/-调整,Shift==0.1,大写锁定==10,判定偏移量(不是歌曲偏移量))\n");
		line.append("速度设置向导(S)\n");
		line.append("重置Songs路径(R)\n");
		for (size_t i = 0; i < buf.Height; i++) {
			line.push_back('\n');
		}
		line.append("Author: Telecomadm1145 ( https://github.com/telecomadm1145 )\n");
		buf.DrawString(line, 0, scroll, {}, {});
	}
	virtual void Key(KeyEventArgs kea) {
		if (kea.Pressed) {
			if (kea.Key == ConsoleKey::Escape) {
				parent->Back();
				return;
			}
			if (kea.Key == ConsoleKey::DownArrow) {
				scroll++;
				return;
			}
			if (kea.Key == ConsoleKey::UpArrow) {
				scroll--;
				return;
			}
			if (kea.Key == ConsoleKey::J) {
				game->Settings["JumpHelper"].Set(!game->Settings["JumpHelper"].Get<bool>());
				game->Settings.Write();
				return;
			}
			if (kea.Key == ConsoleKey::H) {
				game->Settings["NoBmpHs"].Set(!game->Settings["NoBmpHs"].Get<bool>());
				game->Settings.Write();
				return;
			}
			if (kea.Key == ConsoleKey::T) {
				game->Settings["TailHs"].Set(!game->Settings["TailHs"].Get<bool>());
				game->Settings.Write();
				return;
			}
			if (kea.Key == ConsoleKey::W) {
				game->Settings["WtMode"].Set(!game->Settings["WtMode"].Get<bool>());
				game->Settings.Write();
				return;
			}
			if (kea.Key == ConsoleKey::B) {
				game->Settings["NoBg"].Set(!game->Settings["NoBg"].Get<bool>());
				game->Settings.Write();
				return;
			}
			if (kea.Key == ConsoleKey::F) {
				game->Settings["MyCompSuck"].Set(!game->Settings["MyCompSuck"].Get<bool>());
				game->Settings.Write();
				return;
			}
			if (kea.Key == ConsoleKey::S) {
				parent->Navigate(MakeSpeedSettingScreen());
				return;
			}
			if (kea.Key == ConsoleKey::R) {
				game->Settings["SongsPath"].SetArray("", 0);
				game->Settings.Write();
				return;
			}
			if (kea.UnicodeChar == '+' || kea.UnicodeChar == '-')
			{
				auto movement = 1.0;
				if (HasFlag(kea.KeyState, ControlKeyState::Capslock))
				{
					movement = 10;
				}
				if (HasFlag(kea.KeyState,ControlKeyState::Shift))
				{
					movement = 0.1;
				}
				if (kea.UnicodeChar == '-')
				{
					movement = -movement;
				}
				game->Settings["Offset"].Set(game->Settings["Offset"].Get<double>() + movement);
				game->Settings.Write();
				return;
			}
		}
	}
};

Screen* MakeSettingsScreen() {
	return new SettingsScreen();
}
