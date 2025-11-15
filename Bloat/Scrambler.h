#pragma once
#include <vector>
#include "Bloater.h"
#include "Obfuscator.h"

class Scrambler
{
private:
	uint64_t bloatMultiplier;
	std::unique_ptr<Obfuscator> obfuscator;

public:
	inline uint64_t GetBloatMultiplier() const noexcept { return bloatMultiplier; }
	inline const std::unique_ptr<Obfuscator>& GetObfuscator() const noexcept { return obfuscator; }

	inline Scrambler(uint64_t bloatMultiplier, const std::unique_ptr<Obfuscator>& obfuscator) noexcept
		: bloatMultiplier(bloatMultiplier), obfuscator(obfuscator->Clone()) { }

	inline void Scramble(std::vector<unsigned char>& bytes) const
	{
		Bloater::Bloat(bytes, bloatMultiplier);
		obfuscator->Obfuscate(bytes);
	}

	inline void Unscramble(std::vector<unsigned char>& bytes) const
	{
		obfuscator->Deobfuscate(bytes);
		Bloater::Debloat(bytes, bloatMultiplier);
	}
};
