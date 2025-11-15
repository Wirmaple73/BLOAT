#pragma once
#include <fstream>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

// std::fstream if it were actually good:
template <typename StreamType>
class Stream
{
protected:
	StreamType stream;

public:
	inline std::streampos GetReadPosition() noexcept { return stream.tellg(); }
	inline void SetReadPosition(const std::streampos position) noexcept { stream.seekg(position); }

	explicit Stream(StreamType&& stream) : stream(std::move(stream)) { }

	template<typename T>
	T Read()
	{
		T value{};
		stream.read(reinterpret_cast<char*>(&value), sizeof(T));

		return value;
	}

	std::vector<unsigned char> ReadBytes(std::streamsize numBytes)
	{
		std::vector<unsigned char> bytes(numBytes);

		stream.read(reinterpret_cast<char*>(bytes.data()), numBytes);
		return bytes;
	}

	std::vector<unsigned char> ReadAllBytes()
	{
		stream.seekg(0, std::ios::end);
		const std::streamsize size = stream.tellg();

		stream.seekg(0, std::ios::beg);
		return ReadBytes(size);
	}

	std::string ReadString(std::streamsize numBytes)
	{
		auto bytes = ReadBytes(numBytes);
		return std::string(bytes.begin(), bytes.end());
	}

	template<typename T>
	inline void Write(const T& value) noexcept
	{
		stream.write(reinterpret_cast<const char*>(&value), sizeof(T));
	}

	inline void Write(const std::string& value) noexcept
	{
		stream.write(value.c_str(), value.length());
	}

	inline void Write(const std::vector<unsigned char>& bytes) noexcept
	{
		stream.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
	}
};

class FileStream : public Stream<std::fstream>
{
public:
	inline static FileStream OpenRead(const fs::path& path)
	{
		std::fstream stream(path, std::ios::in | std::ios::binary);
		stream.exceptions(std::ios::badbit | std::ios::failbit);

		return FileStream(std::move(stream));
	}

	inline static FileStream OpenWrite(const fs::path& path, const bool overwrite)
	{
		std::fstream stream(path, std::ios::out | std::ios::binary | (overwrite ? std::ios::trunc : 0));
		stream.exceptions(std::ios::badbit | std::ios::failbit);

		return FileStream(std::move(stream));
	}

	FileStream(std::fstream&& stream) : Stream<std::fstream>(std::move(stream)) { }

	inline void Close() noexcept { stream.close(); }
};

class MemoryStream : public Stream<std::ostringstream>
{
public:
	inline MemoryStream() : Stream<std::ostringstream>(std::ostringstream(std::ios::out | std::ios::binary))
	{
		stream.exceptions(std::ios::badbit | std::ios::failbit);
	}

	void WriteToFile(const fs::path& path)
	{
		FileStream fs = FileStream::OpenWrite(path, true);

		fs.Write(stream.str());
		fs.Close();
	}
};
