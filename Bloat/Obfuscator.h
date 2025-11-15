#pragma once
#include <vector>
#include "XorshiftRandom.h"

enum class ObfuscatorId : uint8_t
{
	EmptyObfuscator = 0, RandomXorObfuscator = 1
};

class Obfuscator
{
protected:
	uint32_t key{};

public:
	inline virtual ObfuscatorId GetId() const noexcept = 0;
	inline virtual bool SupportsKey() const noexcept = 0;

	virtual inline uint32_t GetKey() const = 0;
	virtual inline void SetKey(const uint32_t key) = 0;

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

	inline bool SupportsKey() const noexcept override { return false; }

	inline uint32_t GetKey() const override
	{
		throw std::logic_error("This obfuscator does not support random number generation.");
	}

	inline void SetKey(const uint32_t key) override
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

	inline bool SupportsKey() const noexcept override { return true; }

	inline uint32_t GetKey() const override { return key; }
	inline void SetKey(const uint32_t key) override { this->key = key; }

	inline void Obfuscate(std::vector<unsigned char>& bytes) const override
	{
		const size_t byteSize = bytes.size();

		// We're just hiding patterns from compressors, not aiming for military-grade cryptography. Xorshift is significantly faster than mt19937.
		XorshiftRandom random(key);

		for (size_t i = 0; i < byteSize; i++)
			bytes[i] ^= random.NextUInt32();
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
				return std::make_unique<RandomXorObfuscator>();

			default:
				throw std::invalid_argument("The specified obfuscator ID could not be resolved.");
		}
	}
};
