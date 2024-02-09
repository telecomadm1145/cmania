#pragma once
// if linux
#include <stdexcept>
#ifdef __linux__
using _debugbreak = void (*)(void);
#endif
#ifdef _WIN32
inline void _debugbreak() {
	__debugbreak();
}
#endif