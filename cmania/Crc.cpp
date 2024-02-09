#ifdef _WIN32
#include "Crc.h"
#include <intrin.h>
uint32_t GetCrc(const char* data, std::size_t s) {
	uint32_t crc = ~0u;
	// Process any remaining bytes
	while (s--) {
		crc = _mm_crc32_u8(crc, *data++);
	}
	return ~crc;
}
#endif
#ifdef __linux__
#include "Crc.h"
#include <array>
// 生成CRC查询表
constexpr void init_crc_table(uint32_t table[256]) {
	uint32_t c;
	for (uint32_t i = 0; i < 256; i++) {
		c = i;
		for (int j = 0; j < 8; j++) {
			c = c & 1 ? 0xedb88320 ^ (c >> 1) : c >> 1;
		}
		table[i] = c;
	}
}

using crc_tab = std::array<uint32_t, 256>;
constexpr crc_tab crc_table{
	[]() consteval {
		crc_tab table;
		init_crc_table(table.data());
		return table;
}() };

// 计算CRC值
constexpr uint32_t crc32(const char* data, std::size_t length, const uint32_t table[256]) {
	uint32_t crc = 0xffffffff;
	for (std::size_t i = 0; i < length; i++) {
		crc = (crc >> 8) ^ table[(crc & 0xff) ^ data[i]];
	}
	return crc ^ 0xffffffff;
}

// 定义GetCrc函数
uint32_t GetCrc(const char* data, std::size_t s) {
	return crc32(data, s, crc_table.data());
}
#endif