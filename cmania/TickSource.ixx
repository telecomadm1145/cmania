export module TickSource;
import Game;
import <thread>;
import Hpet;

export class TickSource : public GameComponent
{
	std::thread* thread;
	// Í¨¹ý Component ¼Ì³Ð
	virtual void ProcessEvent(const char* evt, const void* evtargs)
	{
		if (strcmp(evt, "start") == 0)
		{
			HighPerformanceTimer::BeginHighResolutionSystemClock();
			thread = new std::thread(TickingWorker, parent);
			thread->detach();
		}
	}
	static void TickingWorker(Game* parent)
	{
		double last_tick = 0;
		while (true)
		{
			int fps = 1000;
			parent->Raise("tick", last_tick = HighPerformanceTimer::GetMilliseconds());
			if (fps < 480 && fps > 1)
			{
				auto now = HighPerformanceTimer::GetMilliseconds();
				auto offset = 1000.0 / fps - (now - last_tick);
				if (offset > 1)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds((int)offset));
				}
			}
		}
	}
};