#include "Crc.h"
#include <intrin.h>
uint32_t GetCrc(const char* data, size_t s) {
	uint32_t crc = ~0u;
	// Process any remaining bytes
	while (s--) {
		crc = _mm_crc32_u8(crc, *data++);
	}
	return ~crc;
}