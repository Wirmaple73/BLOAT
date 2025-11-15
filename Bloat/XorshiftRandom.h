#pragma once
#include <random>

class XorshiftRandom
{
private:
	static constexpr const uint32_t DEFAULT_SEED = 0x9e3779B1u;  // Some weird prime cool folks use
	uint32_t seed;

	inline void SetSeed(const uint32_t seed) { this->seed = seed != 0u ? seed : DEFAULT_SEED; }

public:
	inline uint32_t GetSeed() const noexcept { return seed; }

	inline XorshiftRandom() noexcept
	{
		std::random_device rd;
		SetSeed(static_cast<uint32_t>(rd()));
	}

	inline XorshiftRandom(const uint32_t seed) noexcept { SetSeed(seed); }

	inline uint32_t NextUInt32() noexcept
	{
		seed ^= seed << 13;
		seed ^= seed >> 17;
		seed ^= seed << 5;

		return seed;
	}
};
