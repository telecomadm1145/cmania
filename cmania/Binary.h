#pragma once
#include <concepts>
#include <istream>
#include <ostream>
#include <any>
#include <unordered_map>

template <class T>
concept trivial = std::is_trivial_v<T>;
template <class T>
concept trivial_pair = trivial<typename T::first_type> && trivial<typename T::second_type>;
template <class T>
concept BinaryData = trivial<T> || trivial_pair<T>;
template <class T>
concept BinaryClass =
	requires(T& t, std::ostream& os, std::istream& is) {
		{ t.Write(os) } -> std::same_as<void>;
		{ t.Read(is) } -> std::same_as<void>;
	};
template <class T>
using ContainerChild = std::remove_cvref_t<decltype(*std::declval<T>().cbegin())>;
template <class T>
concept BinaryVector =
	requires(T v, typename T::value_type& val, size_t sz) {
		// 需要大小操作
		{ v.size() } -> std::convertible_to<std::size_t>;
		{ v.reserve(sz) } -> std::same_as<void>;

		// 需要只读迭代器
		{ v.cbegin() } -> std::convertible_to<typename T::const_iterator>;
		{ v.cend() } -> std::convertible_to<typename T::const_iterator>;

		{ v.push_back(val) } -> std::same_as<void>;
	};
template <class T>
concept BinaryMap =
	requires(T m) {
		// 需要键值对类型
		typename T::key_type;
		typename T::mapped_type;
		typename T::value_type;

		// 需要只读迭代器
		{ m.cbegin() } -> std::convertible_to<typename T::const_iterator>;
		{ m.cend() } -> std::convertible_to<typename T::const_iterator>;
	};

class DataObject {
public:
	virtual ~DataObject() {
	}
	virtual void Write(std::ostream& stm) = 0;
	virtual void Read(std::istream& stm) = 0;
};

template <class T>
concept IsDataObject = std::convertible_to<std::remove_cvref_t<T>*, DataObject*>;

using NewDataObject = DataObject* (*)();

extern std::unordered_map<int, NewDataObject> InstanceLoaders;

/// <summary>
/// 提供结构化的二进制与stl的转换
/// </summary>
class Binary {
public:
	template <typename T>
	constexpr static inline uint64_t CalculateClassHash() {
		constexpr uint64_t Prime = 0x100000001B3ull;
		constexpr uint64_t OffsetBasis = 0xCBF29CE484222325ull;

		const std::string className = typeid(T).name();
		uint64_t hash = OffsetBasis;

		for (char c : className) {
			hash ^= static_cast<uint64_t>(c);
			hash *= Prime;
		}

		return hash;
	}
	template <typename T>
	constexpr static inline uint64_t ClassHash = CalculateClassHash<T>();

	template <IsDataObject T>
	static inline void Read(std::istream& stm, std::unique_ptr<T>& obj) {
		uint64_t classHash;
		Read(stm, classHash);
		if (InstanceLoaders.find(classHash) == InstanceLoaders.end()) {
			throw std::runtime_error("Missing instance loader for class hash: " + std::to_string(classHash));
		}
		obj.reset(InstanceLoaders[classHash]());
		obj->Read(stm);
	}

	template <IsDataObject T>
	static inline void Write(std::ostream& stm, const std::unique_ptr<T>& obj) {
		uint64_t classHash = ClassHash<T>; 
		Write(stm, classHash);
		obj->Write(stm);
	}
	template <typename T>
	static inline void Register() {
		InstanceLoaders[ClassHash<T>] = []() -> DataObject* {
			return new T();
		};
	}
	template <IsDataObject T>
	static inline void Read(std::istream& stm, T*& obj) {
		uint64_t classHash;
		Read(stm, classHash);
		if (InstanceLoaders.find(classHash) == InstanceLoaders.end()) {
			throw std::runtime_error("Missing instance loader for class hash: " + std::to_string(classHash));
		}
		obj = dynamic_cast<T*>(InstanceLoaders[classHash]());
		obj->Read(stm);
	}

	template <IsDataObject T>
	static inline void Write(std::ostream& stm, const T* obj) {
		uint64_t classHash = ClassHash<T>;
		Write(stm, classHash);
		obj->Write(stm);
	}

	static inline void Write(std::ostream& stm, const BinaryData auto& dat) {
		stm.write((char*)&dat, sizeof(dat));
	}
	static inline void Read(std::istream& stm, BinaryData auto& dat) {
		stm.read((char*)&dat, sizeof(dat));
	}
	static inline void Write(std::ostream& stm, const BinaryClass auto& cls) {
		cls.Write(stm);
	}
	static inline void Read(std::istream& stm, BinaryClass auto& cls) {
		cls.Read(stm);
	}
	static inline void Read(std::istream& stm, BinaryVector auto& vec) {
		using ContainerChild = ::ContainerChild<decltype(vec)>;
		unsigned long long size = 0;
		Read(stm, size);
		if (size > 1ULL << 48) {
			__debugbreak();
		}
		vec.reserve(size);
		for (size_t i = 0; i < size; i++) {
			if (stm.eof())
				return;
			ContainerChild data{};
			Read(stm, data);
			vec.push_back(data);
		}
	}
	static inline void Write(std::ostream& stm, const BinaryVector auto& vec) {
		unsigned long long sz = vec.size();
		Write(stm, sz);
		for (const auto& data : vec) {
			Write(stm, data);
			sz--;
		}
		if (sz != 0)
			__debugbreak();
	}
	static inline void Read(std::istream& stm, BinaryMap auto& map) {
		using ContainerChild = ::ContainerChild<decltype(map)>;
		unsigned long long size = 0;
		Read(stm, size);
		if (size > 1ULL << 48) {
			__debugbreak();
		}
		for (size_t i = 0; i < size; i++) {
			if (stm.eof())
				return;
			std::remove_cvref_t<typename ContainerChild::first_type> key{};
			Read(stm, key);
			std::remove_cvref_t<typename ContainerChild::second_type> val{};
			Read(stm, val);
			map[key] = val;
		}
	}
	static inline void Write(std::ostream& stm, const BinaryMap auto& map) {
		unsigned long long sz = map.size();
		Write(stm, sz);
		for (const auto& kv : map) {
			Write(stm, kv.first);
			Write(stm, kv.second);
			sz--;
		}
		if (sz != 0)
			__debugbreak();
	}
};