#pragma once
#include "EnumFlag.h"

// Specifies the standard keys on a console.
enum class ConsoleKey {
	// The BACKSPACE key.
	Backspace = 8,
	// The TAB key.
	Tab = 9,
	// The CLEAR key.
	Clear = 12,
	// The ENTER key.
	Enter = 13,
	// The PAUSE key.
	Pause = 19,
	// The ESC (ESCAPE) key.
	Escape = 27,
	// The SPACEBAR key.
	Spacebar = 32,
	// The PAGE UP key.
	PageUp = 33,
	// The PAGE DOWN key.
	PageDown = 34,
	// The END key.
	End = 35,
	// The HOME key.
	Home = 36,
	// The LEFT ARROW key.
	LeftArrow = 37,
	// The UP ARROW key.
	UpArrow = 38,
	// The RIGHT ARROW key.
	RightArrow = 39,
	// The DOWN ARROW key.
	DownArrow = 40,
	// The SELECT key.
	Select = 41,
	// The PRINT key.
	Print = 42,
	// The EXECUTE key.
	Execute = 43,
	// The PRINT SCREEN key.
	PrintScreen = 44,
	// The INS (INSERT) key.
	Insert = 45,
	// The DEL (DELETE) key.
	Delete = 46,
	// The HELP key.
	Help = 47,
	// The 0 key.
	D0 = 48,
	// The 1 key.
	D1 = 49,
	// The 2 key.
	D2 = 50,
	// The 3 key.
	D3 = 51,
	// The 4 key.
	D4 = 52,
	// The 5 key.
	D5 = 53,
	// The 6 key.
	D6 = 54,
	// The 7 key.
	D7 = 55,
	// The 8 key.
	D8 = 56,
	// The 9 key.
	D9 = 57,
	// The A key.
	A = 65,
	// The B key.
	B = 66,
	// The C key.
	C = 67,
	// The D key.
	D = 68,
	// The E key.
	E = 69,
	// The F key.
	F = 70,
	// The G key.
	G = 71,
	// The H key.
	H = 72,
	// The I key.
	I = 73,
	// The J key.
	J = 74,
	// The K key.
	K = 75,
	// The L key.
	L = 76,
	// The M key.
	M = 77,
	// The N key.
	N = 78,
	// The O key.
	O = 79,
	// The P key.
	P = 80,
	// The Q key.
	Q = 81,
	// The R key.
	R = 82,
	// The S key.
	S = 83,
	// The T key.
	T = 84,
	// The U key.
	U = 85,
	// The V key.
	V = 86,
	// The W key.
	W = 87,
	// The X key.
	X = 88,
	// The Y key.
	Y = 89,
	// The Z key.
	Z = 90,
	// The left Windows logo key (Microsoft Natural Keyboard).
	LeftWindows = 91,
	// The right Windows logo key (Microsoft Natural Keyboard).
	RightWindows = 92,
	// The Application key (Microsoft Natural Keyboard).
	Applications = 93,
	// The Computer Sleep key.
	Sleep = 95,
	// The 0 key on the numeric keypad.
	NumPad0 = 96,
	// The 1 key on the numeric keypad.
	NumPad1 = 97,
	// The 2 key on the numeric keypad.
	NumPad2 = 98,
	// The 3 key on the numeric keypad.
	NumPad3 = 99,
	// The 4 key on the numeric keypad.
	NumPad4 = 100,
	// The 5 key on the numeric keypad.
	NumPad5 = 101,
	// The 6 key on the numeric keypad.
	NumPad6 = 102,
	// The 7 key on the numeric keypad.
	NumPad7 = 103,
	// The 8 key on the numeric keypad.
	NumPad8 = 104,
	// The 9 key on the numeric keypad.
	NumPad9 = 105,
	// The Multiply key (the multiplication key on the numeric keypad).
	Multiply = 106,
	// The Add key (the addition key on the numeric keypad).
	Add = 107,
	// The Separator key.
	Separator = 108,
	// The Subtract key (the subtraction key on the numeric keypad).
	Subtract = 109,
	// The Decimal key (the decimal key on the numeric keypad).
	Decimal = 110,
	// The Divide key (the division key on the numeric keypad).
	Divide = 111,
	// The F1 key.
	F1 = 112,
	// The F2 key.
	F2 = 113,
	// The F3 key.
	F3 = 114,
	// The F4 key.
	F4 = 115,
	// The F5 key.
	F5 = 116,
	// The F6 key.
	F6 = 117,
	// The F7 key.
	F7 = 118,
	// The F8 key.
	F8 = 119,
	// The F9 key.
	F9 = 120,
	// The F10 key.
	F10 = 121,
	// The F11 key.
	F11 = 122,
	// The F12 key.
	F12 = 123,
	// The F13 key.
	F13 = 124,
	// The F14 key.
	F14 = 125,
	// The F15 key.
	F15 = 126,
	// The F16 key.
	F16 = 127,
	// The F17 key.
	F17 = 128,
	// The F18 key.
	F18 = 129,
	// The F19 key.
	F19 = 130,
	// The F20 key.
	F20 = 131,
	// The F21 key.
	F21 = 132,
	// The F22 key.
	F22 = 133,
	// The F23 key.
	F23 = 134,
	// The F24 key.
	F24 = 135,
	// The Browser Back key.
	BrowserBack = 166,
	// The Browser Forward key.
	BrowserForward = 167,
	// The Browser Refresh key.
	BrowserRefresh = 168,
	// The Browser Stop key.
	BrowserStop = 169,
	// The Browser Search key.
	BrowserSearch = 170,
	// The Browser Favorites key.
	BrowserFavorites = 171,
	// The Browser Home key.
	BrowserHome = 172,
	// The Volume Mute key (Microsoft Natural Keyboard).
	VolumeMute = 173,
	// The Volume Down key (Microsoft Natural Keyboard).
	VolumeDown = 174,
	// The Volume Up key (Microsoft Natural Keyboard).
	VolumeUp = 175,
	// The Media Next Track key.
	MediaNext = 176,
	// The Media Previous Track key.
	MediaPrevious = 177,
	// The Media Stop key.
	MediaStop = 178,
	// The Media Play/Pause key.
	MediaPlay = 179,
	// The Start Mail key (Microsoft Natural Keyboard).
	LaunchMail = 180,
	// The Select Media key (Microsoft Natural Keyboard).
	LaunchMediaSelect = 181,
	// The Start Application 1 key (Microsoft Natural Keyboard).
	LaunchApp1 = 182,
	// The Start Application 2 key (Microsoft Natural Keyboard).
	LaunchApp2 = 183,
	// The OEM 1 key (OEM specific).
	Oem1 = 186,
	// The OEM Plus key on any country/region keyboard.
	OemPlus = 187,
	// The OEM Comma key on any country/region keyboard.
	OemComma = 188,
	// The OEM Minus key on any country/region keyboard.
	OemMinus = 189,
	// The OEM Period key on any country/region keyboard.
	OemPeriod = 190,
	// The OEM 2 key (OEM specific).
	Oem2 = 191,
	// The OEM 3 key (OEM specific).
	Oem3 = 192,
	// The OEM 4 key (OEM specific).
	Oem4 = 219,
	// The OEM 5 (OEM specific).
	Oem5 = 220,
	// The OEM 6 key (OEM specific).
	Oem6 = 221,
	// The OEM 7 key (OEM specific).
	Oem7 = 222,
	// The OEM 8 key (OEM specific).
	Oem8 = 223,
	// The OEM 102 key (OEM specific).
	Oem102 = 226,
	// The IME PROCESS key.
	Process = 229,
	// The PACKET key (used to pass Unicode characters with keystrokes).
	Packet = 231,
	// The ATTN key.
	Attention = 246,
	// The CRSEL (CURSOR SELECT) key.
	CrSel = 247,
	// The EXSEL (EXTEND SELECTION) key.
	ExSel = 248,
	// The ERASE EOF key.
	EraseEndOfFile = 249,
	// The PLAY key.
	Play = 250,
	// The ZOOM key.
	Zoom = 251,
	// A constant reserved for future use.
	NoName = 252,
	// The PA1 key.
	Pa1 = 253,
	// The CLEAR key (OEM specific).
	OemClear = 254
};
enum class ControlKeyState {
	RightAlt = 0x1,
	LeftAlt = 0x2,
	RightCtrl = 0x4,
	LeftCtrl = 0x8,
	Shift = 0x10,
	Numlock = 0x20,
	Scrolllock = 0x40,
	Capslock = 0x80
};
struct KeyEventArgs {
	KeyEventArgs(int cks, bool down, int key, wchar_t chr, int rc) {
		KeyState = (ControlKeyState)cks;
		Pressed = down;
		Key = (ConsoleKey)key;
		UnicodeChar = chr;
		RepeatCount = rc;
	}
	ControlKeyState KeyState;
	wchar_t UnicodeChar;
	ConsoleKey Key;
	bool Pressed;
	int RepeatCount;
#define M1(T1, T2) \
	bool T1() { return HasFlag(KeyState, ControlKeyState::T2); }
	M1(LeftAltDown, LeftAlt);
	M1(RightAltDown, RightAlt);
	M1(LeftCtrlDown, LeftCtrl);
	M1(RightCtrlDown, RightCtrl);
	M1(Numlock, Numlock);
	M1(Scrolllock, Scrolllock);
	M1(Capslock, Capslock);
#undef M1
	bool AltDown() {
		return LeftAltDown() || RightAltDown();
	}
	bool CtrlDown() {
		return RightCtrlDown() || LeftCtrlDown();
	}
};
struct MoveEventArgs {
	int X;
	int Y;
};
struct ResizeEventArgs {
	int X;
	int Y;
};
struct WheelEventArgs {
	double Delta;
	enum class Direction {
		Vertical,
		Horizontal,
	} WheelDirection;
};
struct MouseKeyEventArgs {
	int X;
	int Y;
	enum class Button {
		Left,
		Middle,
		Right,
		X1,
		X2,
	} MouseButton;
	bool Pressed;
	MouseKeyEventArgs(int x, int y, Button btn, bool pressed) {
		X = x;
		Y = y;
		MouseButton = btn;
		Pressed = pressed;
	}
};