#pragma once
#include <fstream>
#include <vector>
#include <map>
#include <type_traits>
#include <concepts>
#pragma warning(push)
#pragma warning(disable : 4267)
struct BinaryStorageNode {
	unsigned int Size;
	char Name[16];
};
struct BinaryStorageItem {
	unsigned int Size = 0;
	void* Data = 0;
	BinaryStorageItem() = default;
	BinaryStorageItem(unsigned int sz, void* dat) : Size(sz), Data(dat) {}
	template <typename T>
	BinaryStorageItem(const T& ref) {
		Set(ref);
	}
	char* GetString() {
		if (Size == 0)
			return (char*)0;
		return (char*)Data;
	}
	template <typename T>
	T* GetArray() {
		return (T*)Data;
	}
	template <typename T>
	T& Get() {
		if (Size == 0)
			Set(T());
		if (sizeof(T) != Size)
			throw std::exception("Size mismatched.");
		return *(T*)Data;
	}
	template <typename T>
	void Set(const T& ref) {
		if (Data != 0) {
			delete Data;
		}
		Size = sizeof(T);
		Data = new T(ref);
	}
	template <typename T>
	void SetArray(const T* ptr, unsigned int count) {
		if (Data != 0) {
			delete[] Data;
		}
		Size = count * sizeof(T);
		Data = new T[count];
		memcpy((void*)Data, (const void*)ptr, Size);
	}
};
/// <summary>
/// 一个简单的二进制存储类，支持无指针的有复制构造函数的类型
/// </summary>
class BinaryStorage {
private:
	std::map<std::string, BinaryStorageItem> items;

public:
	void Read() {
		std::ifstream ifs("Setting.bin", std::ios_base::binary | std::ios_base::in);
		if (!ifs.good())
			return;
		unsigned int sig = 0;
		ifs.read((char*)&sig, 4);
		if (sig != 0x57524d43)
			return;
		unsigned int count = 0;
		ifs.read((char*)&count, 4);
		if (count != 0) {
			for (size_t i = 0; i < count; i++) {
				BinaryStorageNode bsn{};
				ifs.read((char*)&bsn, sizeof(bsn));
				char* data = new char[bsn.Size];
				ifs.read((char*)data, bsn.Size);
				items[bsn.Name] = BinaryStorageItem{ bsn.Size, data };
			}
		}
		ifs.close();
	}
	void Write() {
		std::ofstream ofs("Setting.bin", std::ios_base::binary | std::ios_base::out);
		if (!ofs.good())
			throw std::exception("cannot save file");
		unsigned int sig = 0x57524d43;
		ofs.write((char*)&sig, 4);
		unsigned int count = items.size();
		ofs.write((char*)&count, 4);
		for (auto& kv : items) {
			BinaryStorageNode bsn{};
			std::copy(kv.first.begin(), kv.first.end(), bsn.Name);
			bsn.Size = kv.second.Size;
			ofs.write((char*)&bsn, sizeof(bsn));
			ofs.write((char*)kv.second.Data, kv.second.Size);
		}
		ofs << "\n\n\nNotice:This is a BINARY file, NOT a text file.DO NOT edit this by text editors like notepad.";
		ofs.close();
	}
	BinaryStorageItem& operator[](const std::string& key) {
		return items[key];
	}
};
// std::pair<int, int>;
template <class T>
concept trivial = std::is_trivial<T>::value;
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

// #define _BIN_DBG

/// <summary>
/// 提供结构化的二进制与stl的转换
/// </summary>
class Binary {
public:
	static void Write(std::ostream& stm, const BinaryData auto& dat) {
		stm.write((char*)&dat, sizeof(dat));
#ifdef _BIN_DBG
		stm.flush();
#endif //  _BIN_DBG
	}
	static void Read(std::istream& stm, BinaryData auto& dat) {
		stm.read((char*)&dat, sizeof(dat));
	}
	static void Write(std::ostream& stm, const BinaryClass auto& cls) {
		cls.Write(stm);
	}
	static void Read(std::istream& stm, BinaryClass auto& cls) {
		cls.Read(stm);
	}
	static void Read(std::istream& stm, BinaryVector auto& vec) {
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
	static void Write(std::ostream& stm, const BinaryVector auto& vec) {
		unsigned long long sz = vec.size();
		Write(stm, sz);
		for (const auto& data : vec) {
			Write(stm, data);
			sz--;
		}
		if (sz != 0)
			__debugbreak();
	}
	static void Read(std::istream& stm, BinaryMap auto& map) {
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
	static void Write(std::ostream& stm, const BinaryMap auto& map) {
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
#pragma warning(pop)