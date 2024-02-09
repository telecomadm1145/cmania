#pragma once
// if linux
#include <stdexcept>
#ifdef __linux__
using _debugbreak = void (*)(void);
#endif
#ifdef _WIN32
using debugbreak = __debugbreak;
#endif