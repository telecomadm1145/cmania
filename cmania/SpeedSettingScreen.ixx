export module SpeedSettingScreen;
import ScreenController;
import Settings;
import WinDebug;

export class SpeedSettingScreen :public Screen
{
	double speed = 750;
	double elapsedTime = 0;
	static constexpr double c1 = 12000;

	virtual void Activate(bool y)
	{
		if (y)
		{
			game->Raise("require", "speed");
		}
	}
	virtual void ProcessEvent(const char* evt, const void* evtargs)
	{
		if (strcmp(evt, "set") == 0)
		{
			auto& sea = *(SetEventArgs*)evtargs;
			if (strcmp(evt, "speed") == 0)
			{
				if (sea.Value.has_value() && sea.Value.type() == typeid(double))
				{
					speed = std::any_cast<double>(sea.Value);
				}
			}
		}
	}
	virtual void Render(GameBuffer& buf)
	{
		buf.DrawString("按F3/F4加减速到合适的区间\n(4NPS)", 0, 0, {}, {});
		// Render speed range information
		std::string speedInfo = "Mania速度: " + std::to_string(c1 / speed) + "(" + std::to_string(speed) + "ms)";
		buf.DrawString(speedInfo.c_str(), 0, 2, {}, {});
		if (speed > 0)
		{
			auto x = 6;
			auto mid = buf.Width + x;
			mid /= 2;
			auto clock = elapsedTime;
			auto clock2 = int(elapsedTime / speed) * speed;
			for (double i = clock2 - speed; i < clock2 + speed; i += 250)
			{
				auto y = (clock - i) / speed * (buf.Height - 4); // note的起始时间
				buf.FillRect(mid - x / 2, y, mid + x / 2, y, { {},{255,255,255,255},' ' });
			}
			buf.FillRect(0, buf.Height - 4, buf.Width, buf.Height - 4, { {},{255,128,128,128},' ' });
		}
	}
	virtual void Tick(double time)
	{
		elapsedTime = time;
	}
	virtual void Key(KeyEventArgs kea)
	{
		if (kea.Pressed)
		{
			if (kea.Key == ConsoleKey::Escape)
			{
				parent->Back();
				return;
			}
			if (kea.Key == ConsoleKey::F3) // 减速
			{
				if (speed > 0)
				{
					speed = c1 / (c1 / speed - 1.0);
				}
			}
			if (kea.Key == ConsoleKey::F4) // 加速
			{
				if (speed > 0)
				{
					speed = c1 / (c1 / speed + 1.0);
				}
				else
				{
					speed = 1;
				}
			}
		}
	}
};