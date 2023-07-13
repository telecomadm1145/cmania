#pragma once
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