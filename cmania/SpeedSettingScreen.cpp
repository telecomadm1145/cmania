#include "ScreenController.h"
#include "BassAudioManager.h"
#include "File.h"
#include "SpeedSettingScreen.h"

class SpeedSettingScreen :public Screen
{
	double speed = 750;
	double elapsedTime = 0;
	static constexpr double c1 = 12000;
	std::unique_ptr<IAudioManager::ISample> tick;
	std::unique_ptr<IAudioManager::ISample> hit;
	double lastact = 0;

	virtual void Activate(bool y)
	{
		if (y)
		{
			speed = game->Settings["ScrollSpeed"].Get<double>();
			auto bam = GetBassAudioManager();
			{
				auto dat = ReadAllBytes("Samples\\Triangles\\normal-slidertick.wav");
				tick = std::unique_ptr<IAudioManager::ISample>(bam->loadSample(dat.data(), dat.size()));
			}
			{
				auto dat = ReadAllBytes("Samples\\Triangles\\normal-hitnormal.wav");
				hit = std::unique_ptr<IAudioManager::ISample>(bam->loadSample(dat.data(), dat.size()));
			}
		}
	}
	virtual void Render(GameBuffer& buf)
	{
		if (tick == 0 || hit == 0)
			return;
		buf.DrawString("按F3/F4加减速到合适的区间(按R重置)(一个区间4个音符)\n", 0, 0, {}, {});
		// Render speed range information
		std::string speedInfo = "Mania速度: " + std::to_string(c1 / speed) + "(" + std::to_string(speed) + "ms)";
		buf.DrawString(speedInfo.c_str(), 0, 2, {}, {});
		if (speed > 0)
		{
			buf.FillRect(0, buf.Height - 4, buf.Width, buf.Height - 4, { {},{255,128,128,128},' ' });
			auto x = 10;
			auto mid = buf.Width + x;
			mid /= 2;
			auto clock = elapsedTime;
			auto clock2 = int(elapsedTime / speed / 32) * speed * 32;
			int j = 0;
			for (double i = clock2 - speed * 32; i < clock2 + speed * 32; i += speed / 2)
			{
				auto clr = GameBuffer::Color{ 255,255,255,255 };
				if ((j + 2) % 4 == 0)
				{
					clr = { 255,255,0,0 };
				}
				auto y = (clock - i) / speed * (buf.Height - 4); // note的起始时间
				buf.FillRect(mid - x / 2, y, mid + x / 2, y, { {},clr,' ' });
				j++;
			}
			if (abs(clock - int(elapsedTime / speed / 0.5) * speed * 0.5) < 30 && abs(lastact - clock) > 60)
			{
				if (abs(clock - int(elapsedTime / speed / 2) * speed * 2) < 30)
				{
					auto stream = hit->generateStream();
					stream->play();
					delete stream;
				}
				auto stream = tick->generateStream();
				stream->play();
				delete stream;
				lastact = clock;
			}
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
			if (kea.Key == ConsoleKey::R)
			{
				speed = 500;
				game->Settings["ScrollSpeed"].Set(speed);
				game->Settings.Write();
			}
			if (kea.Key == ConsoleKey::F3) // 减速
			{
				if (speed > 0)
				{
					speed = c1 / (c1 / speed - 1.0);
					if (!(speed >= 0))
						speed = 500;
				}
				game->Settings["ScrollSpeed"].Set(speed);
				game->Settings.Write();
			}
			if (kea.Key == ConsoleKey::F4) // 加速
			{
				if (speed > 151)
				{
					speed = c1 / (c1 / speed + 1.0);
				}
				else
				{
					speed = 500;
				}
				game->Settings["ScrollSpeed"].Set(speed);
				game->Settings.Write();
			}
		}
	}
};

Screen* MakeSpeedSettingScreen()
{
	return new SpeedSettingScreen();
}
