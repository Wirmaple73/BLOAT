#pragma once

class SplitMix64
{
public:
    static inline uint64_t Mix(uint64_t x) noexcept
    {
        x ^= x >> 30;
        x *= 0xbf58476d1ce4e5b9ui64;
        x ^= x >> 27;
        x *= 0x94d049bb133111ebui64;
        x ^= x >> 31;

        return x;
    }

    static inline uint64_t ComputeHash(const std::vector<unsigned char>& bytes) noexcept
    {
        // Sample:
        // 
        // Index:       0        1       2       3       4       5       6       7       8       9
        // Address:     1000     1001    1002    1003    1004    1005    1006    1007    1008    1009
        // Byte value:  AA       BB      CC      DD      EE      FF      00      11      22      33

        const uint64_t byteSize = bytes.size();

        uint64_t hash = 0x9e3779b97f4a7c15ui64;  // Cool golden ratio constant
        uint64_t i;

        for (i = 0; i + 8 <= byteSize; i += 8)
        {
            uint64_t buffer;
            std::memcpy(&buffer, &bytes[i], 8);

            hash += Mix(buffer);
        }

        if (i < byteSize)  // Handle the tail
        {
            uint64_t buffer = 0ui64;
            std::memcpy(&buffer, &bytes[i], byteSize - i);

            hash += Mix(buffer);
        }

        return hash;
    }
};
