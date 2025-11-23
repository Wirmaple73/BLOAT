#pragma once
#include <filesystem>
#include <fstream>
#include <vector>
#include <ranges>
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

	const fs::path actualPath;  // For external files only
	const fs::path relativePath;

	const std::shared_ptr<Scrambler> scrambler;
	bool isRemoved = false;

	// For internal files only
	mutable std::shared_ptr<FileStream> archiveStream;

	const uint64_t dataStartOffset{};
	const uint64_t dataLength{};

public:
	// For external files
	inline ArchiveFile(const fs::path& actualPath, const fs::path& relativePath, const std::shared_ptr<Scrambler>& scrambler)
		: fileType(ArchiveFileType::ExternalFile), actualPath(actualPath), relativePath(relativePath), scrambler(scrambler) { }

	// For internal files
	inline ArchiveFile(
		const fs::path& relativePath, const std::shared_ptr<Scrambler>& scrambler,
		const std::shared_ptr<FileStream>& archiveStream, const uint64_t dataStartOffset, const uint64_t dataLength
	)
		: fileType(ArchiveFileType::InternalFile), relativePath(relativePath), scrambler(scrambler),
		archiveStream(archiveStream), dataStartOffset(dataStartOffset), dataLength(dataLength) { }

	inline const fs::path& GetPath() const noexcept { return relativePath; }

	// In bytes
	inline uint64_t GetUnscrambledSize() const noexcept
	{
		return fileType == ArchiveFileType::InternalFile ?
			dataLength / scrambler->GetBloatMultiplier() : fs::file_size(actualPath);
	}

	// In bytes
	inline uint64_t GetSize() const noexcept
	{
		return fileType == ArchiveFileType::InternalFile ?
			dataLength : fs::file_size(actualPath) * scrambler->GetBloatMultiplier();
	}

	// Note: Advances the internal archive file pointer.
	inline std::vector<unsigned char> GetBytes() const
	{
		if (fileType == ArchiveFileType::InternalFile)
		{
			archiveStream->SetReadPosition(dataStartOffset);

			auto bytes = archiveStream->ReadBytes(dataLength);
			scrambler->Unscramble(bytes);

			return bytes;
		}

		return FileStream::OpenRead(actualPath).ReadAllBytes();  // External file
	}

	inline bool IsRemoved() const noexcept { return isRemoved; }
	inline void MarkAsRemoved() noexcept { isRemoved = true; }
};
