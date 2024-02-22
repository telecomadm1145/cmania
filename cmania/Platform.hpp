#pragma once
// if linux
#include <stdexcept>
#ifdef __linux__
inline void _debugbreak() {
	(*(int*)0) = 0;
}
#endif
#ifdef _WIN32
inline void _debugbreak() {
	__debugbreak();
}
#endif