export module BufferController;
import Game;
import GameBuffer;

export class BufferController : public GameComponent
{
	GameBuffer buf;
	// Í¨¹ý Component ¼Ì³Ð
	virtual void ProcessEvent(const char* evt, const void* evtargs)
	{
		if (strcmp(evt, "start") == 0)
		{
			buf.InitConsole();
		}
		if (strcmp(evt, "tick") == 0)
		{
			buf.Clear();
			parent->Raise("draw", buf);
			buf.Output();
		}
		if (strcmp(evt, "resize") == 0)
		{
			buf.ResizeBuffer();
		}
	}
};