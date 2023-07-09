module;
#include <windows.h>
export module AsyncConsole;

export void WriteConsoleAsync(const char* text, int size)
{
	auto hstdout = GetStdHandle(STD_OUTPUT_HANDLE);
	OVERLAPPED ol{};
	WriteFileEx(hstdout, text, size, &ol, [](DWORD, DWORD, LPOVERLAPPED) {});
}

export void WriteConsoleAsync(const char* text)
{
	return WriteConsoleAsync(text, strlen(text));
}
