#pragma once
#include <vector>
#include "Bloater.h"
#include "Obfuscator.h"

struct Scrambler
{
private:
	uint64_t bloatMultiplier;
	std::unique_ptr<Obfuscator> obfuscator;

public:
	inline uint64_t GetBloatMultiplier() const noexcept { return bloatMultiplier; }
	inline const std::unique_ptr<Obfuscator>& GetObfuscator() const noexcept { return obfuscator; }

	static inline std::shared_ptr<Scrambler> Create(uint64_t bloatMultiplier, const std::unique_ptr<Obfuscator>& obfuscator) noexcept
	{
		return std::make_shared<Scrambler>(bloatMultiplier, obfuscator);
	}

	static inline std::shared_ptr<Scrambler> CreateEmpty() noexcept
	{
		return std::make_shared<Scrambler>(1ui64, ObfuscatorFactory::Create(ObfuscatorId::EmptyObfuscator));
	}

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
