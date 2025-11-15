#pragma once
#include <filesystem>
#include <unordered_set>
#include "ArchiveFile.h"
#include "Obfuscator.h"
#include "Scrambler.h"

namespace fs = std::filesystem;

class BloatArchive
{
private:
	uint8_t version = CURRENT_ARCHIVE_VERSION;

	std::shared_ptr<Scrambler> scrambler;

	std::vector<ArchiveFile> files{};
	std::unordered_set<std::string> filePaths{};  // For very fast file duplication checks

	static inline const std::string MAGIC_NUMBER = "BLTBCS";  // "Bloat Because Compression Sucks"
	static constexpr inline const uint8_t CURRENT_ARCHIVE_VERSION = 1ui8;

public:
	uint8_t GetVersion() const noexcept;

	std::shared_ptr<Scrambler> GetScrambler() const noexcept;
	void SetScrambler(const std::shared_ptr<Scrambler>& scrambler) noexcept;

	std::vector<ArchiveFile> GetFiles() const noexcept;
	uint64_t GetChecksum();

	BloatArchive() noexcept;

	static BloatArchive Open(const fs::path& archivePath);

	void AddFile(const fs::path& filePath, const fs::path& basePath);
	void AddFilesFromDirectory(const fs::path& dirPath, const bool recursive);

	void Extract(const fs::path& destDir, const bool overwriteExistingFiles);
	void Save(const fs::path& destPath);
};
