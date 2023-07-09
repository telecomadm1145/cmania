module;
#include <Windows.h>
#pragma warning(disable:4005)
#pragma warning(disable:5260)
export module Win32ConsoleComponent;
import "ConsoleInput.h";
import Game;
import <thread>;

export class Win32ConsoleComponent : public GameComponent
{
	static short HighShort(DWORD dw)
	{
		DWORD tmp = (dw & 0xffff0000) >> 16;
		return *(int*)&tmp;
	}
public:
	std::thread* input_thread = 0;
	void ProcessEvent(const char* evt, const void* evtargs)
	{
		if (strcmp(evt, "start") == 0)
		{
			auto hstdin = GetStdHandle(STD_INPUT_HANDLE);
			unsigned long mode = 0;
			GetConsoleMode(hstdin, &mode);
			mode = (mode | ENABLE_MOUSE_INPUT) & (~ENABLE_QUICK_EDIT_MODE);
			SetConsoleMode(hstdin, mode);
			auto hstdout = GetStdHandle(STD_OUTPUT_HANDLE);
			GetConsoleMode(hstdout, &mode);
			mode = mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
			SetConsoleMode(hstdout, mode);
			input_thread = new std::thread(&InputWorker, parent);
			input_thread->detach();
		}
	}
	static void InputWorker(Game* parent)
	{
		auto hstdin = GetStdHandle(-10);
		unsigned long read = 0;
		INPUT_RECORD record{};
		while (ReadConsoleInputW(hstdin, &record, 1, &read))
		{
			switch (record.EventType)
			{
			case KEY_EVENT:
			{
				auto ke = record.Event.KeyEvent;
				KeyEventArgs kea(ke.dwControlKeyState, ke.bKeyDown, ke.wVirtualKeyCode, ke.uChar.UnicodeChar, ke.wRepeatCount);
				parent->Raise("key", kea);
				break;
			}
			case MOUSE_EVENT:
			{
				auto me = record.Event.MouseEvent;
				if (me.dwEventFlags == MOUSE_MOVED)
				{
					MoveEventArgs mea(me.dwMousePosition.X, me.dwMousePosition.Y);
					parent->Raise("move", mea);
				}
				if (me.dwEventFlags == MOUSE_WHEELED)
				{
					WheelEventArgs wea(double(HighShort(me.dwButtonState)) / 120, WheelEventArgs::Direction::Vertical);
					parent->Raise("wheel", wea);
				}
				if (me.dwEventFlags == MOUSE_HWHEELED)
				{
					WheelEventArgs wea(double(HighShort(me.dwButtonState)) / 120, WheelEventArgs::Direction::Horizontal);
					parent->Raise("wheel", wea);
				}
				bool leftnow = HasFlag(me.dwButtonState, FROM_LEFT_1ST_BUTTON_PRESSED);
				static bool leftclicked = false;
				if (leftnow ^ leftclicked)
				{
					MouseKeyEventArgs mkea(me.dwMousePosition.X, me.dwMousePosition.Y, MouseKeyEventArgs::Button::Left, leftnow);
					parent->Raise("mousekey", mkea);
					leftclicked = leftnow;
				}
				bool rightnow = HasFlag(me.dwButtonState, RIGHTMOST_BUTTON_PRESSED);
				static bool rightclicked = false;
				if (rightnow ^ rightclicked)
				{
					MouseKeyEventArgs mkea(me.dwMousePosition.X, me.dwMousePosition.Y, MouseKeyEventArgs::Button::Right, rightnow);
					parent->Raise("mousekey", mkea);
					rightclicked = rightnow;
				}
				bool middlenow = HasFlag(me.dwButtonState, FROM_LEFT_2ND_BUTTON_PRESSED);
				static bool middleclicked = false;
				if (middlenow ^ middleclicked)
				{
					MouseKeyEventArgs mkea(me.dwMousePosition.X, me.dwMousePosition.Y, MouseKeyEventArgs::Button::Middle, middlenow);
					parent->Raise("mousekey", mkea);
					middleclicked = middlenow;
				}
				break;
			}
			case WINDOW_BUFFER_SIZE_EVENT:
			{
				parent->Raise("resize");
			}
			case FOCUS_EVENT:
			{
				parent->Raise("focus", record.Event.FocusEvent.bSetFocus);
			}
			default:
				break;
			}
		}
	}
};