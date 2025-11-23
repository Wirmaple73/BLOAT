#pragma once
#include <vector>
#include "Exceptions.h"
#include "Xorshift64Star.h"

enum class ObfuscatorId : uint8_t
{
	EmptyObfuscator = 0ui8, RandomXorObfuscator = 1ui8
};

class Obfuscator
{
protected:
	uint64_t key = 0ui64;

public:
	inline virtual ObfuscatorId GetId() const noexcept = 0;
	inline virtual const char* GetName() const noexcept = 0;

	inline virtual bool SupportsKey() const noexcept = 0;

	inline virtual uint64_t GetKey() const = 0;
	inline virtual void SetKey(const uint64_t key) = 0;

	explicit Obfuscator() noexcept { }

	virtual void Obfuscate(std::vector<unsigned char>& bytes) const = 0;
	virtual void Deobfuscate(std::vector<unsigned char>& bytes) const = 0;

	inline virtual std::unique_ptr<Obfuscator> Clone() const = 0;
	inline virtual ~Obfuscator() = default;
};

class EmptyObfuscator : public Obfuscator
{
public:
	inline ObfuscatorId GetId() const noexcept override { return ObfuscatorId::EmptyObfuscator; }
	inline const char* GetName() const noexcept override { return "Empty obfuscator"; }

	inline bool SupportsKey() const noexcept override { return false; }

	inline uint64_t GetKey() const override
	{
		throw std::logic_error("This obfuscator does not support random number generation.");
	}

	inline void SetKey(const uint64_t key) override
	{
		throw std::logic_error("This obfuscator does not support random number generation.");
	}

	inline void Obfuscate(std::vector<unsigned char>&) const override
	{
		// Preserve the original bytes
	}

	inline void Deobfuscate(std::vector<unsigned char>&) const override
	{
		// Preserve the original bytes
	}

	inline virtual std::unique_ptr<Obfuscator> Clone() const override
	{
		return std::make_unique<EmptyObfuscator>();
	}
};

class RandomXorObfuscator : public Obfuscator
{
public:
	inline ObfuscatorId GetId() const noexcept override { return ObfuscatorId::RandomXorObfuscator; }
	inline const char* GetName() const noexcept override { return "Random XOR obfuscator"; }

	inline bool SupportsKey() const noexcept override { return true; }

	inline uint64_t GetKey() const override { return key; }

	inline void SetKey(const uint64_t key) override
	{
		if (key == 0ui64)
			throw InvalidObfuscatorKeyException("This obfuscator cannot use zero as its key.");

		this->key = key;
	}

	void Obfuscate(std::vector<unsigned char>& bytes) const override
	{
		if (key == 0ui64)
			throw InvalidObfuscatorKeyException("This obfuscator cannot use zero as its key.");

		// Xorshift is much faster than mt19937 and eliminates patterns as efficiently
		Xorshift64Star random(key);

		const size_t byteSize = bytes.size();
		size_t i;

		for (i = 0; i + 7 < byteSize; i += 8)
		{
			const uint64_t r = random.NextUInt64();

			for (int j = 0; j < 8; j++)
				bytes[i + j] ^= (r >> ((7 - j) * 8)) & 0xFF;

			/* Equivalent to:
				bytes[i]	 ^= r >> (7 * 8);
				bytes[i + 1] ^= r >> (6 * 8);
				bytes[i + 2] ^= r >> (5 * 8);
				bytes[i + 3] ^= r >> (4 * 8);
				bytes[i + 4] ^= r >> (3 * 8);
				bytes[i + 5] ^= r >> (2 * 8);
				bytes[i + 6] ^= r >> (1 * 8);
				bytes[i + 7] ^= r >> (0 * 8);
			*/
		}

		if (i < byteSize)  // Handle leftover bytes
		{
			const uint64_t r = random.NextUInt64();

			for (int j = 0; i + j < byteSize; j++)
				bytes[i + j] ^= (r >> ((7 - j) * 8)) & 0xFF;
		}
	}

	inline void Deobfuscate(std::vector<unsigned char>& bytes) const override
	{
		// XOR'ing previously-XOR'ed bytes with the same key will yield the original bytes
		Obfuscate(bytes);
	}

	inline virtual std::unique_ptr<Obfuscator> Clone() const override
	{
		auto obfuscator = std::make_unique<RandomXorObfuscator>();
		obfuscator->SetKey(this->key);

		return obfuscator;
	}
};

class ObfuscatorFactory
{
public:
	static std::unique_ptr<Obfuscator> Create(const ObfuscatorId id)
	{
		switch (id)
		{
			case ObfuscatorId::EmptyObfuscator:
				return std::make_unique<EmptyObfuscator>();

			case ObfuscatorId::RandomXorObfuscator:
			{
				auto obfuscator = std::make_unique<RandomXorObfuscator>();
				obfuscator->SetKey(Xorshift64Star().GetState());  // Initialize with a random state

				return obfuscator;
			}

			default:
				throw std::invalid_argument("The specified obfuscator ID could not be resolved.");
		}
	}
};
