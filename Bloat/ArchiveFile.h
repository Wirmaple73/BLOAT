#pragma once
#include <filesystem>
#include <fstream>
#include <vector>
#include "Stream.h"
#include "Bloater.h"
#include "Scrambler.h"

namespace fs = std::filesystem;

enum class ArchiveFileType
{
	InternalFile, ExternalFile
};

class ArchiveFile
{
private:
	const ArchiveFileType fileType;

	const fs::path path;
	const std::shared_ptr<FileStream> fs;

	const std::shared_ptr<Scrambler> scrambler;

	// For internal files only
	const uint64_t dataStartOffset{};
	const uint64_t dataLength{};

public:
	//inline ArchiveFileType GetArchiveFileType() const noexcept { return fileType; }

	inline ArchiveFile(const fs::path& path, const std::shared_ptr<FileStream>& stream, const std::shared_ptr<Scrambler>& scrambler)
		: fileType(ArchiveFileType::ExternalFile), path(path), fs(stream), scrambler(scrambler) { }

	inline ArchiveFile(
		const fs::path& path, const std::shared_ptr<FileStream>& stream, const std::shared_ptr<Scrambler>& scrambler,
		const uint64_t dataStartOffset, const uint64_t dataLength
	)
		: fileType(ArchiveFileType::InternalFile), path(path), fs(stream), scrambler(scrambler), dataStartOffset(dataStartOffset), dataLength(dataLength) { }

	inline fs::path GetPath() const noexcept { return path; }

	inline std::vector<unsigned char> GetBytes()
	{
		if (fileType == ArchiveFileType::InternalFile)
		{
			fs->SetReadPosition(dataStartOffset);

			auto bytes = fs->ReadBytes(dataLength);
			scrambler->Unscramble(bytes);

			return bytes;
		}

		return fs->ReadAllBytes();  // External file (not scrambled)
	}
};
