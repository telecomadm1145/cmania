#pragma once
#include <type_traits>

template<typename Num, typename Num2>
inline bool HasFlag(Num flags, Num2 flag)
{
	return ((unsigned long long)flags & (unsigned long long)flag) == (unsigned long long)flag;
}
template<typename Num, typename Num2>
inline Num ModifyFlag(Num flags, Num2 flag)
{
	return (Num)((unsigned long long)flags | (unsigned long long)flag);
}
template<typename Num, typename Num2>
inline Num RemoveFlag(Num flags, Num2 flag)
{
	return (Num)((unsigned long long)flags & ~(unsigned long long)flag);
}
template<typename Num, typename Num2>
inline Num ToggleFlag(Num flags, Num2 flag)
{
	return (Num)((unsigned long long)flags ^ (unsigned long long)flag);
}

template<class T>
concept Enum = std::is_enum<T>::value;

constexpr bool operator>(Enum auto a, Enum auto b)
{
	return ((uint64_t)a) > ((uint64_t)b);
}
constexpr bool operator<(Enum auto a, Enum auto b)
{
	return ((uint64_t)a) < ((uint64_t)b);
}
constexpr bool operator>=(Enum auto a, Enum auto b)
{
	return ((uint64_t)a) >= ((uint64_t)b);
}
constexpr bool operator<=(Enum auto a, Enum auto b)
{
	return ((uint64_t)a) <= ((uint64_t)b);
}
template<Enum T>
constexpr T operator^(T a, Enum auto b)
{
	return (T)(((uint64_t)a) ^ ((uint64_t)b));
}
template<Enum T>
constexpr T operator|(T a, Enum auto b)
{
	return (T)(((uint64_t)a) | ((uint64_t)b));
}
template<Enum T>
constexpr T operator&(T a, Enum auto b)
{
	return (T)(((uint64_t)a) & ((uint64_t)b));
}
template<Enum T>
constexpr T operator~(T val)
{
	return (T)(~(uint64_t)val);
}