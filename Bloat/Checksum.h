#pragma once
#include <vector>

class Checksum
{
public:
	static inline uint64_t Calculate(const std::vector<unsigned char>& bytes) noexcept
	{
		// FNV-1a hashing algorithm - probably overkill for this toy project
		uint64_t hash = 0xcbf29ce484222325ull;

		for (const unsigned char byte : bytes)
		{
			hash ^= byte;
			hash *= 0x100000001b3;
		}

		return hash;
	}
};
