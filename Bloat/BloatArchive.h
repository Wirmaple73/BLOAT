#pragma once
#include <filesystem>
#include <unordered_map>
#include "ArchiveFile.h"
#include "Scrambler.h"

namespace fs = std::filesystem;

class BloatArchive
{
private:
	uint8_t version = CURRENT_ARCHIVE_VERSION;

	mutable bool isChecksumUpToDate = false;
	mutable uint64_t checksum = 0ui64;

	std::shared_ptr<Scrambler> scrambler;

	std::vector<ArchiveFile> files{};
	std::unordered_map<fs::path, size_t> fileIndices{};  // For blazing fast file duplication checks and index lookups

	static inline const std::string MAGIC_NUMBER = "\xE9" "BLTBCS";  // "BLOAT Because Compression Sucks"
	static constexpr inline const uint8_t CURRENT_ARCHIVE_VERSION = 1ui8;

	size_t GetActiveFileCount() const noexcept;

	void ThrowIfFileDoesNotExist(const fs::path& filePath) const;
	ArchiveFile& GetFileOrThrow(const fs::path& filePath);

	void ExtractFile(const ArchiveFile& file, const fs::path& destDir,
		const bool overwriteExisting, const bool throwIfRemoved, const bool throwIfDuplicated) const;

public:
	explicit BloatArchive() noexcept;
	uint8_t GetVersion() const noexcept;

	uint64_t GetScrambledSize() const;
	uint64_t GetUnscrambledSize() const;

	const std::shared_ptr<Scrambler>& GetScrambler() const noexcept;
	void SetScrambler(const std::shared_ptr<Scrambler>& scrambler) noexcept;

	// Some homebrewed hash accumulator function or something. We'll call it the glorious BLOATSUM (tm).
	uint64_t GetChecksum() const;

	static BloatArchive Open(const fs::path& archivePath);

	// Adds a file to the archive.
	void AddFile(const fs::path& filePath, const fs::path& relativePath, const bool overwriteExisting);

	// Adds a file to the root of the archive.
	void AddFile(const fs::path& filePath, const bool overwriteExisting);

	void AddDirectory(const fs::path& dirPath, const bool recursive, const bool overwriteExisting);
	void RemoveFile(const fs::path& filePath);

#undef RemoveDirectory  // TODO

	void RemoveDirectory(const fs::path& filePath);

	const std::vector<ArchiveFile>& GetAllFiles() const noexcept;
	const ArchiveFile& GetFile(const fs::path& filePath) const;

	bool DoesFileExist(const fs::path& filePath) const noexcept;

	// Extracts all files to the destination directory.
	void Extract(const fs::path& destDir, const bool overwriteExistingFiles) const;

	// Extracts the specified file to the destination directory.
	void Extract(const fs::path& filePath, const fs::path& destDir, const bool overwriteExisting, const bool throwIfDuplicated) const;

	void Save(const fs::path& destPath, const bool overwrite) const;
};
