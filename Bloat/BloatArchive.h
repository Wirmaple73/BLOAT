#pragma once
#include <filesystem>
#include <unordered_map>
#include "ArchiveFile.h"
#include "Scrambler.h"
#include "Exceptions.h"
#include "Stream.h"
#include "SplitMix64.h"
#include "Xorshift64Star.h"

namespace fs = std::filesystem;

class BloatArchive
{
private:
	uint8_t version = CURRENT_ARCHIVE_VERSION;

	mutable bool isChecksumUpToDate = false;
	mutable uint64_t checksum{};

	bool isChecksumVerified = true;

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

	uint64_t CalculateChecksum(const bool forceRecalculate) const noexcept;

public:
	// Creates a new empty BLOAT archive.
	explicit BloatArchive() noexcept;

	// Gets the version of this BLOAT archive.
	uint8_t GetVersion() const noexcept;

	// Gets the approximate scrambled (packed) size of this BLOAT archive.
	uint64_t GetScrambledSize() const;

	// Gets the approximate unscrambled (unpacked) size of this BLOAT archive.
	uint64_t GetUnscrambledSize() const;

	const std::shared_ptr<Scrambler>& GetScrambler() const noexcept;
	void SetScrambler(const std::shared_ptr<Scrambler>& scrambler) noexcept;

	// Gets the checksum of this BLOAT archive.
	uint64_t GetChecksum() const noexcept;

	// Loads an existing BLOAT archive from disk.
	static BloatArchive Open(const fs::path& archivePath, const bool verifyChecksum = true);

	// Gets all files inside the archive.
	const std::vector<ArchiveFile>& GetAllFiles() const noexcept;

	// Gets the specified file inside the archive.
	const ArchiveFile& GetFile(const fs::path& filePath) const;

	// Determines whether the specified file exists in the archive.
	bool DoesFileExist(const fs::path& filePath) const noexcept;

	// Determines whether the specified directory exists in the archive.
	bool DoesDirectoryExist(const fs::path& dirPath) const noexcept;

	// Adds the specified file to the archive.
	void AddFile(const fs::path& filePath, const fs::path& relativePath, const bool overwriteExisting);

	// Adds the specified file to the root of the archive.
	void AddFile(const fs::path& filePath, const bool overwriteExisting);

	// Adds the specified directory to the archive.
	void AddDirectory(const fs::path& dirPath, const bool recursive, const bool overwriteExisting);

	// Removes the specified file from the archive.
	void RemoveFile(const fs::path& filePath);

	// Removes the specified directory and its contents from the archive.
	void RemoveDirectory(const fs::path& filePath);

	// Extracts the specified file to the destination directory.
	void ExtractFile(const fs::path& filePath, const fs::path& destDir, const bool overwriteExisting, const bool throwIfDuplicated) const;

	// Extracts the specified directory to the destination directory.
	void ExtractDirectory(const fs::path& dirPath, const fs::path& destDir, const bool overwriteExisting, const bool throwIfDuplicated) const;

	// Extracts all files to the destination directory.
	void Extract(const fs::path& destDir, const bool overwriteExistingFiles) const;

	// Exports the current archive to the destination path.
	void Save(const fs::path& destPath, const bool overwrite) const;
};
