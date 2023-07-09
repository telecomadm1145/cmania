export module MainMenuScreen;
import ScreenController;
import SongSelectScreen;
import SettingsScreen;

export class MainMenuScreen : public Screen
{
	virtual void Render(GameBuffer& buf)
	{
		buf.DrawString("按下 Enter 进入选歌界面\n按下 O 键进行设置", 0, 0, {}, {});
	}
	virtual void Key(KeyEventArgs kea)
	{
		if (kea.Pressed)
		{
			if (kea.Key == ConsoleKey::Enter)
			{
				parent->Navigate<SongSelectScreen>();
			}
			if (kea.Key == ConsoleKey::O)
			{
				parent->Navigate<SettingsScreen>();
			}
		}
	}
};