#pragma once
#include <vector>

class Bloater
{
public:
	inline static void Bloat(std::vector<unsigned char>& bytes, const uint64_t bloatMultiplier)
	{
		/* Example:
		* bytes = AB CD
		* bloatMultiplier = 3
		* bytes * bloatMultiplier = AB CD   AB CD   AB CD
		*/

		if (bloatMultiplier < 1)
			throw std::invalid_argument("The specified bloat multiplier must be greater than or equal to 1.");

		if (bloatMultiplier == 1)
			return;

		const uint64_t originalByteSize = bytes.size();
		bytes.resize(originalByteSize * bloatMultiplier);

		for (uint64_t run = 1; run < bloatMultiplier; run++)
			std::memcpy(bytes.data() + run * originalByteSize, bytes.data(), originalByteSize);
	}

	inline static void Debloat(std::vector<unsigned char>& bytes, const uint64_t bloatMultiplier)
	{
		/* Example:
		* bytes = AB CD   AB CD   AB CD
		* bloatMultiplier = 3
		* bytes / bloatMultiplier = AB CD
		*/

		if (bloatMultiplier < 1)
			throw std::invalid_argument("The specified bloat multiplier is invalid.");

		if (bloatMultiplier == 1)
			return;

		if (bytes.size() % bloatMultiplier != 0)
			throw std::invalid_argument("The passed bytes cannot be debloated. The data is either corrupted or was bloated using a different bloat multiplier.");

		bytes.resize(bytes.size() / bloatMultiplier);
	}
};
