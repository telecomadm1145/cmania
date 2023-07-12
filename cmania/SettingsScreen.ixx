export module SettingsScreen;
import ScreenController;
import <string>;
import SpeedSettingScreen;
import <vector>;

export class SettingsScreen :public Screen
{

	// 这里
	virtual void Render(GameBuffer& buf)
	{
		std::string str = "选项:\n";
		str.append("J - 双押辅助\nS - 速度设置界面");
		//str.append(game->Settings[JumpHelper].Get<bool>() ? "[ON]" : "[OFF]");
		str.append("\n");
		buf.DrawString(str, 0, 0, {}, {});
	}
	virtual void Key(KeyEventArgs kea)
	{
		if (kea.Pressed)
		{
			if (kea.Key == ConsoleKey::Escape)
			{
				parent->Back();
			}
			if (kea.Key == ConsoleKey::S)
			{
				parent->Navigate<SpeedSettingScreen>();
				return;
			}
		}
	}
};