#include <thread>
#include <Windows.h>
#include "Game.h"
#include "ConsoleInput.h"
#include "Win32ConsoleComponent.h"
#include "LogOverlay.h"
#pragma warning(disable : 4267)

class Win32ConsoleComponent : public GameComponent {
	static short HighShort(DWORD dw) {
		DWORD tmp = (dw & 0xffff0000) >> 16;
		return *(int*)&tmp;
	}
	static void WriteConsoleAsync(const char* text, int size) {
		auto hstdout = GetStdHandle(STD_OUTPUT_HANDLE);
		auto _ = 0UL;
		WriteFile(hstdout, text, size, &_, 0);
	}

public:
	std::thread* input_thread = 0;
	void ProcessEvent(const char* evt, const void* evtargs) {
		if (strcmp(evt, "start") == 0) {
			auto hstdin = GetStdHandle(STD_INPUT_HANDLE);
			unsigned long mode = 0;
			GetConsoleMode(hstdin, &mode);
			mode = (mode | ENABLE_MOUSE_INPUT) & (~ENABLE_QUICK_EDIT_MODE);
			SetConsoleMode(hstdin, mode);
			auto hstdout = GetStdHandle(STD_OUTPUT_HANDLE);
			GetConsoleMode(hstdout, &mode);
			mode = mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
			SetConsoleMode(hstdout, mode);
			SetConsoleOutputCP(65001);
			input_thread = new std::thread(&InputWorker, parent);
			input_thread->detach();
		}
		if (strcmp(evt, "push") == 0) {
			struct PushEventArgs {
				const char* buf;
				size_t len;
			};
			auto pea = *(PushEventArgs*)evtargs;
			SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { 0, 0 });
			WriteConsoleAsync(pea.buf, pea.len);
		}
		if (strcmp(evt, "fresize") == 0) {
			SendResize(parent);
		}
	}
	static void SendResize(Game* parent) {
		CONSOLE_SCREEN_BUFFER_INFO bufferInfo{};
		auto sout = GetStdHandle(STD_OUTPUT_HANDLE);
		GetConsoleScreenBufferInfo(sout, &bufferInfo);
		bufferInfo.dwSize = COORD{ bufferInfo.srWindow.Right - bufferInfo.srWindow.Left, bufferInfo.srWindow.Bottom - bufferInfo.srWindow.Top };
		ResizeEventArgs rea{ bufferInfo.dwSize.X + 1, bufferInfo.dwSize.Y + 1 };
		parent->Raise("resize", rea);
	}
	static void InputWorker(Game* parent) {
		auto hstdin = GetStdHandle(-10);
		unsigned long read = 0;
		INPUT_RECORD record{};
		while (ReadConsoleInputW(hstdin, &record, 1, &read)) {
			try {
				switch (record.EventType) {
				case KEY_EVENT: {
					auto ke = record.Event.KeyEvent;
					KeyEventArgs kea(ke.dwControlKeyState, ke.bKeyDown, ke.wVirtualKeyCode, ke.uChar.UnicodeChar, ke.wRepeatCount);
					parent->Raise("key", kea);
					break;
				}
				case MOUSE_EVENT: {
					auto me = record.Event.MouseEvent;
					if (me.dwEventFlags == MOUSE_MOVED) {
						MoveEventArgs mea(me.dwMousePosition.X, me.dwMousePosition.Y);
						parent->Raise("move", mea);
					}
					if (me.dwEventFlags == MOUSE_WHEELED) {
						WheelEventArgs wea(double(HighShort(me.dwButtonState)) / 120, WheelEventArgs::Direction::Vertical);
						parent->Raise("wheel", wea);
					}
					if (me.dwEventFlags == MOUSE_HWHEELED) {
						WheelEventArgs wea(double(HighShort(me.dwButtonState)) / 120, WheelEventArgs::Direction::Horizontal);
						parent->Raise("wheel", wea);
					}
					bool leftnow = HasFlag(me.dwButtonState, FROM_LEFT_1ST_BUTTON_PRESSED);
					static bool leftclicked = false;
					if (leftnow ^ leftclicked) {
						MouseKeyEventArgs mkea(me.dwMousePosition.X, me.dwMousePosition.Y, MouseKeyEventArgs::Button::Left, leftnow);
						parent->Raise("mousekey", mkea);
						leftclicked = leftnow;
					}
					bool rightnow = HasFlag(me.dwButtonState, RIGHTMOST_BUTTON_PRESSED);
					static bool rightclicked = false;
					if (rightnow ^ rightclicked) {
						MouseKeyEventArgs mkea(me.dwMousePosition.X, me.dwMousePosition.Y, MouseKeyEventArgs::Button::Right, rightnow);
						parent->Raise("mousekey", mkea);
						rightclicked = rightnow;
					}
					bool middlenow = HasFlag(me.dwButtonState, FROM_LEFT_2ND_BUTTON_PRESSED);
					static bool middleclicked = false;
					if (middlenow ^ middleclicked) {
						MouseKeyEventArgs mkea(me.dwMousePosition.X, me.dwMousePosition.Y, MouseKeyEventArgs::Button::Middle, middlenow);
						parent->Raise("mousekey", mkea);
						middleclicked = middlenow;
					}
					break;
				}
				case WINDOW_BUFFER_SIZE_EVENT: {
					SendResize(parent);
					break;
				}
				case FOCUS_EVENT: {
					parent->Raise("focus", record.Event.FocusEvent.bSetFocus);
					break;
				}
				default:
					break;
				}
			}
			catch (const std::exception& ex) {
				parent->GetFeature<ILogger>().LogError(ex.what());
			}
			catch (...) {
				parent->GetFeature<ILogger>().LogError("Error during key.");
			}
		}
	}
};

GameComponent* MakeWin32ConsoleComponent() {
	return new Win32ConsoleComponent();
}
