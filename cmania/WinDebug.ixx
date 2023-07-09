module;
#include <windows.h>
export module WinDebug;

export void Break()
{
	::DebugBreak();
}