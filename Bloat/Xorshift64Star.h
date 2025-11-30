#pragma once
#include <random>

class Xorshift64Star  // Kinda overkill but quick and deadly
{
private:
	static constexpr const uint64_t DEFAULT_STATE = 0x9e3779b97f4a7c15;  // Some weird number cool folks use
	uint64_t state;

	inline void SetState(const uint64_t state) { this->state = state != 0u ? state : DEFAULT_STATE; }

public:
	inline uint64_t GetState() const noexcept { return state; }

	inline Xorshift64Star() noexcept
	{
		std::random_device rd{};
		SetState(static_cast<uint64_t>(rd()));
	}

	inline Xorshift64Star(const uint64_t state) noexcept { SetState(state); }

	inline uint64_t NextUInt64() noexcept
	{
		state ^= state >> 12;
		state ^= state << 25;
		state ^= state >> 27;

		return state * 0x2545F4914F6CDD1Dui64;
	}
};
