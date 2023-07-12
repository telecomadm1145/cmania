export module ScreenController;
import Game;
import GameBuffer;
import "ConsoleInput.h";

export class Screen
{
protected:
	class ScreenController* parent;
	Game* game;
public:
	virtual ~Screen() = default;
	void Init(class ScreenController* ptr, Game* ptr2)
	{
		parent = ptr;
		game = ptr2;
	}
	virtual void Render(GameBuffer& buf) {};
	virtual void Tick(double fromRun) {};
	virtual void Initalized() {};
	virtual void Key(KeyEventArgs kea) {};
	virtual void Wheel(WheelEventArgs wea) {};
	virtual void Move(MoveEventArgs mea) {};
	virtual void Activate(bool) {};
	virtual void Resize() {};
	virtual void MouseKey(MouseKeyEventArgs mkea) {};
	virtual void ProcessEvent(const char* evt, const void* evtargs) {};
};

export class ScreenController : public GameComponent
{
	std::vector<Screen*> history;
	int history_index = 0;
	Screen* current;

	// Í¨¹ý Component ¼Ì³Ð
	virtual void ProcessEvent(const char* evt, const void* evtargs)
	{
		if (strcmp(evt, "navigate") == 0)
		{
			Navigate((Screen*)evtargs);
			return;
		}
		if (current != 0)
		{
			if (strcmp(evt, "draw") == 0)
			{
				current->Render(*(GameBuffer*)evtargs);
			}
			if (strcmp(evt, "tick") == 0)
			{
				current->Tick(*(double*)evtargs);
			}
			if (strcmp(evt, "key") == 0)
			{
				current->Key(*(KeyEventArgs*)evtargs);
			}
			if (strcmp(evt, "wheel") == 0)
			{
				current->Wheel(*(WheelEventArgs*)evtargs);
			}
			if (strcmp(evt, "mousekey") == 0)
			{
				current->MouseKey(*(MouseKeyEventArgs*)evtargs);
			}
			if (strcmp(evt, "move") == 0)
			{
				current->Move(*(MoveEventArgs*)evtargs);
			}
			if (strcmp(evt, "resize") == 0)
			{
				current->Resize();
			}
			current->ProcessEvent(evt, evtargs);
		}
	}

public:
	template<typename T>
	void Navigate()
	{
		T* scr = new T();
		Navigate(scr);
	}
	void Navigate(Screen* scr)
	{
		if (scr)
		{
			scr->Init(this, parent);
			scr->Initalized();
		}
		if (current)
			current->Activate(false);
		current = scr;
		if (history_index < history.size())
		{
			delete history[history_index];
			history[history_index] = scr;
		}
		else
		{
			history.insert(history.begin() + history_index, scr);
		}
		history_index++;
		if (current)
			current->Activate(true);
	}
	void GoForward()
	{
		history_index++;
		int ind = history_index - 1;
		if (ind < 0 || ind >= history.size())
			return;
		if (current)
			current->Activate(false);
		current = history[ind];
		if (current)
			current->Activate(true);
	}
	void Back()
	{
		history_index--;
		int ind = history_index - 1;
		if (ind < 0 || ind >= history.size())
			return;
		if (current)
			current->Activate(false);
		current = history[ind];
		if (current)
			current->Activate(true);
	}
};