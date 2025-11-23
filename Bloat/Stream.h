#pragma once
#include <fstream>
#include <filesystem>
#include <vector>
#include <iostream>
#include <Windows.h>

namespace fs = std::filesystem;

template <typename StreamType>
class Stream
{
protected:
	StreamType stream;

public:
	inline explicit Stream(StreamType&& stream) : stream(std::move(stream)) { }

	inline /* const */ StreamType& GetStream() noexcept { return stream; }

	template<typename T>
	inline Stream& operator <<(const T& value)
	{
		stream << value;
		return *this;
	}

	inline std::streampos GetReadPosition() noexcept { return stream.tellg(); }
	inline void SetReadPosition(const std::streampos position) noexcept { stream.seekg(position); }

	template<typename T>
	inline T Read()
	{
		T value{};
		stream.read(reinterpret_cast<char*>(&value), sizeof(T));

		return value;
	}

	inline std::vector<unsigned char> ReadBytes(std::streamsize numBytes)
	{
		std::vector<unsigned char> bytes(numBytes);

		stream.read(reinterpret_cast<char*>(bytes.data()), numBytes);
		return bytes;
	}

	inline std::vector<unsigned char> ReadAllBytes()
	{
		stream.seekg(0, std::ios::end);
		const std::streamsize size = stream.tellg();

		stream.seekg(0, std::ios::beg);
		return ReadBytes(size);
	}

	inline std::string ReadString(std::streamsize numBytes)
	{
		auto bytes = ReadBytes(numBytes);
		return std::string(bytes.begin(), bytes.end());
	}

	inline std::u8string ReadUtf8String(std::streamsize numBytes)
	{
		auto bytes = ReadBytes(numBytes);
		return std::u8string(bytes.begin(), bytes.end());
	}

	template<typename T>
	inline void Write(const T& value)
	{
		stream.write(reinterpret_cast<const char*>(&value), sizeof(T));
	}

	inline void Write(const std::string& value)
	{
		stream.write(value.data(), value.length());
	}

	inline void Write(const std::u8string& value)
	{
		stream.write(reinterpret_cast<const char*>(value.data()), value.length());
	}

	inline void Write(const std::vector<unsigned char>& bytes)
	{
		stream.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
	}
};

// std::fstream if it were actually good:
class FileStream : public Stream<std::fstream>
{
public:
	inline static FileStream OpenRead(const fs::path& path)
	{
		std::fstream stream;

		stream.exceptions(std::ios::badbit | std::ios::failbit);
		stream.open(path, std::ios::in | std::ios::binary);

		return FileStream(std::move(stream));
	}

	inline static FileStream OpenWrite(const fs::path& path, const bool overwrite)
	{
		std::fstream stream;

		stream.exceptions(std::ios::badbit | std::ios::failbit);
		stream.open(path, std::ios::out | std::ios::binary | (overwrite ? std::ios::trunc : 0));

		static char buffer[65536];
		stream.rdbuf()->pubsetbuf(buffer, sizeof(buffer));

		return FileStream(std::move(stream));
	}

	FileStream(std::fstream&& stream) : Stream<std::fstream>(std::move(stream)) { }

	inline void Close() noexcept
	{
		if (stream.is_open())
			stream.close();
	}
};

class MemoryStream : public Stream<std::ostringstream>
{
public:
	inline MemoryStream() : Stream<std::ostringstream>(std::ostringstream(std::ios::out | std::ios::binary))
	{
		stream.exceptions(std::ios::badbit | std::ios::failbit);
	}

	inline std::string GetData() const noexcept { return stream.str(); }

	inline void WriteToFile(const fs::path& path) const
	{
		FileStream fs = FileStream::OpenWrite(path, true);

		fs.Write(stream.str());
		fs.Close();
	}
};
