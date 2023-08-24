// Makes iters more beautiful.
#pragma once
#include <stdexcept>

template <class Iter>
void ForEach(const Iter& begin, const Iter& end, auto func) {
	auto cur = begin;
	while (cur != end) {
		func(*cur);
		++cur;
	};
}
void ForEach(auto& container, auto func) {
	ForEach(container.begin(), container.end(), func);
}
void ForEach(const auto& container, auto func) {
	ForEach(container.cbegin(), container.cend(), func);
}
auto ForEach(auto func) {
	return [func](auto& cont) -> auto& {
		ForEach(cont, func);
		return cont;
	};
}
void AddRange(auto& dest, const auto& src) {
	ForEach(src, [&dest](const auto& item) {
		dest.push_back(item);
	});
}
auto AddRange(const auto& src) {
	return [&](auto& dest) { AddRange(dest, src); };
}
void AddRangeSet(auto& dest, const auto& src) {
	ForEach(src, [&](const auto& item) {
		dest.emplace(item);
	});
}
auto AddRangeSet(const auto& src) {
	return [&](auto& dest) -> auto& {
		AddRangeSet(dest, src);
		return dest;
	};
}
template <class Iter>
class LinqContainer {
	Iter b;
	Iter e;

public:
	LinqContainer(Iter b, Iter e) : b(b), e(e) {
	}
	Iter begin() {
		return b;
	}
	Iter end() {
		return e;
	}
	Iter cbegin() const {
		return b;
	}
	Iter cend() const {
		return e;
	}
	template <class T>
	std::vector<T> ToList() const {
		std::vector<T> vec;
		ForEach(*this, [&vec](const auto& dat) {
			vec.push_back(dat);
		});
		return vec;
	}
};
template <class Iter, class Function>
class SelectQueryIter {
	Iter it;
	Function func;
	std::remove_cvref_t<decltype(func(**((Iter*)0)))> cache;

public:
	SelectQueryIter(Iter it, Function func) : it(it), func(func) {
	}
	auto& operator*() {
		return cache = func(*it);
	}
	auto& operator++() {
		++it;
		return *this;
	}
	auto& operator--() {
		--it;
		return *this;
	}
	auto operator++(int) {
		auto before = *this;
		++it;
		return before;
	}
	auto operator--(int) {
		auto before = *this;
		--it;
		return before;
	}
	bool operator==(const auto& other) const {
		return other.it == it;
	}
	bool operator!=(const auto& other) const {
		return other.it != it;
	}
};
template <class Iter, class Function>
class WhereQueryIter {
	Iter beg;
	Iter cur;
	Iter end;
	Function func;

public:
	WhereQueryIter(Iter begin, Iter current, Iter end, Function func) : beg(begin), cur(current), end(end), func(func) {
		if (current != end && !func(*current)) {
			++(*this);
		}
	}
	auto& operator*() {
		return *cur;
	}
	auto& operator++() {
		++cur;
		while (cur != end) {
			if (!func(*cur))
				++cur;
			else
				break;
		}
		return *this;
	}
	auto operator++(int) {
		auto before = *this;
		++*this;
		return before;
	}
	auto& operator--() {
		--cur;
		while (cur != end) {
			if (!func(*cur))
				--cur;
			else
				break;
		}
		return *this;
	}
	auto operator--(int) {
		auto before = *this;
		--*this;
		return before;
	}
	bool operator==(const auto& other) const {
		return other.cur == cur;
	}
	bool operator!=(const auto& other) const {
		return other.cur != cur;
	}
};
template <class Function>
auto Where(const auto& src, Function func) {
	auto begiter = WhereQueryIter{ src.cbegin(), src.cbegin(), src.cend(), func };
	auto enditer = WhereQueryIter{ src.cend(), src.cend(), src.cend(), func };
	return LinqContainer(begiter, enditer);
}
template <class Function>
auto Where(Function func) {
	return [func](auto& cont) -> auto{ return Where(cont, func); };
}
template <class Container, class Function>
auto Select(const Container& src, Function func) {
	auto begiter = SelectQueryIter{ src.cbegin(), func };
	auto enditer = SelectQueryIter{ src.cend(), func };
	return LinqContainer(begiter, enditer);
}
template <class Function>
auto Select(Function func) {
	return [func](auto& cont) -> auto{ return Select(cont, func); };
}
template <class Container, class Function>
auto Where(Container& src, Function func) {
	auto begiter = WhereQueryIter{ src.begin(), src.begin(), src.end(), func };
	auto enditer = WhereQueryIter{ src.end(), src.end(), src.end(), func };
	return LinqContainer(begiter, enditer);
}
template <class Container, class Function>
auto Select(Container& src, Function func) {
	auto begiter = SelectQueryIter{ src.begin(), func };
	auto enditer = SelectQueryIter{ src.end(), func };
	return LinqContainer(begiter, enditer);
}
inline void First(const auto& container, auto& target) {
	auto cur = container.cbegin();
	if (cur != container.cend())
		target = *cur;
}
inline auto First() {
	return [](auto& container) -> auto{
		auto cur = container.begin();
		if (cur != container.end()) {
			return *cur;
		}
		else {
			throw std::invalid_argument("No matching element");
		}
	};
}
inline auto FirstOrDefault() {
	return [](auto& container) -> auto{
		using ContainerType = std::remove_reference_t<decltype(*container.begin())>;
		if constexpr (std::is_convertible_v<nullptr_t, ContainerType> && std::is_default_constructible_v<ContainerType>) {
			auto cur = container.begin();
			if (cur != container.end()) {
				return *cur;
			}
			else {
				return ContainerType{};
			}
		}
		else {
			auto cur = container.begin();
			if (cur != container.end()) {
				return (ContainerType*)&*cur;
			}
			else {
				return (ContainerType*)0;
			}
		}
	};
}
template <long long Begin, unsigned long long Count>
class Range {
public:
	class Iterator {
		long long i;

	public:
		Iterator(long long i) : i(i) {}
		auto& operator++() {
			++i;
			return *this;
		}
		auto& operator--() {
			--i;
			return *this;
		}
		auto& operator++(int) {
			auto before = *this;
			++i;
			return before;
		}
		auto& operator--(int) {
			auto before = *this;
			--i;
			return before;
		}
		int operator*() const {
			return i;
		}
		bool operator==(const Iterator& it) const {
			return it.i == i;
		}
		bool operator!=(const Iterator& it) const {
			return it.i != i;
		}
	};
	Iterator begin() {
		return Iterator{ Begin };
	}
	Iterator end() {
		return Iterator{ Begin + Count };
	}
	Iterator cbegin() const {
		return Iterator{ Begin };
	}
	Iterator cend() const {
		return Iterator{ Begin + Count };
	}
	constexpr size_t size() const {
		return Count;
	}
};

template <class T>
concept has = true;

template <class Func, class Type>
concept FluentMethod = requires(Func& f, Type& t) {
						   { f(t) } -> std::convertible_to<Type>;
					   };
template <class Func, class Type>
concept NonFluentMethod = !FluentMethod<Func, Type> and requires(Func& f, Type& t) {
								{ f(t) } -> has;
							};

template <class T>
T& operator>(T& left, FluentMethod<T> auto method) {
	return method(left);
}
template <class T>
auto operator>(T& left, NonFluentMethod<T> auto method) {
	return method(left);
}
template <class T>
T& operator>(T&& left, FluentMethod<T> auto method) {
	return method(left);
}
template <class T>
auto operator>(T&& left, NonFluentMethod<T> auto method) {
	return method(left);
}