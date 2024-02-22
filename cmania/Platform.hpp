#pragma once
// if linux
#include <stdexcept>
#ifdef __linux__
inline void _debugbreak() {
	__builtin_trap();
}
#endif
#ifdef _WIN32
inline void _debugbreak() {
	__debugbreak();
}
#endif