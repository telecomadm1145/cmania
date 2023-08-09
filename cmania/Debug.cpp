#include "Debug.h"
#include <Windows.h>

void DbgOutput(const char* str) {
	OutputDebugStringA(str);
}
