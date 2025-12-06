#pragma once
#include <filesystem>
#include "BloatArchive.h"
#include "CmdArgsParser.h"
#include "Utils.h"

namespace fs = std::filesystem;

class ArchiveManipulator
{
private:
	fs::path archivePath;
	std::string password;

	bool verifyChecksum = true;

	static inline void InternalAddEntriesToArchive(BloatArchive& archive, const std::span<char*>& paths, const bool recursive,
		const bool overwrite)
	{
		for (const fs::path& path : paths)
		{
			try
			{
				if (fs::is_regular_file(path))
					archive.AddFile(path, overwrite);
				else if (fs::is_directory(path))
					archive.AddDirectory(path, recursive, overwrite);
				else
					DisplayPathError(path, "The specified path does not represent a valid file or directory.");
			}
			catch (const AggregateException& ex)
			{
				HandleAggregatePathException(ex);
			}
			catch (...)
			{
				HandlePathException(std::current_exception());
			}
		}
	}

	static inline void HandlePathException(const std::exception_ptr originalException)
	{
		try
		{
			std::rethrow_exception(originalException);
		}
		catch (const DuplicateFileException& ex)
		{
			DisplayPathError(ex.GetFilePath(), std::string(ex.what()) + " Specify \"--overwrite-files\" to force overwrite.");
		}
		catch (const FileNotFoundException& ex)
		{
			DisplayPathError(ex.GetFilePath(), ex.what());
		}

		// Probably something fatal. Abort and let main() handle it.
	}

	static inline void HandleAggregatePathException(const AggregateException& ex)
	{
		for (const std::exception_ptr& inner : ex.GetInnerExceptions())
			HandlePathException(inner);
	}

	static inline void DisplayPathError(const fs::path& path, const std::string& message) noexcept
	{
		std::cout << "Skipping \"" << path.generic_string() << "\": " << message << "\n";
	}

public:
	inline explicit ArchiveManipulator() noexcept { }

	inline explicit ArchiveManipulator(const fs::path& archivePath, const std::string& password, const bool verifyChecksum) noexcept
		: archivePath(archivePath), password(password), verifyChecksum(verifyChecksum) {}

	void DisplayInfo() const
	{
		const BloatArchive& archive = BloatArchive::Open(archivePath, verifyChecksum);
		const auto& obfuscator = archive.GetScrambler()->GetObfuscator();

		const uint64_t bloatMultiplier = archive.GetScrambler()->GetBloatMultiplier();
		const uint64_t archiveSize = fs::file_size(archivePath);

		// MSVC's std::format has gotta be the dumbest formatting function on Earth. MSVC can't even give proper error
		// messages (as expected from Microsoft). Why does C++ keep caring about someone's legacy MFC application from
		// 1998 and doesn't add QoL features? Heck, even the committee doesn't even bother to add an ASCII-only ToLower
		// function for strings. Ditch C and move on to modernize C++ like other sane languages lol

		const auto& files = archive.GetAllFiles();
		const size_t fileCount = files.size();

		MemoryStream stream{};

		stream << "Archive info for " << archivePath.filename().string() << ":\n\n"

			<< "--- General Information ---\n"
			<< "* Archive version:  " << static_cast<int>(archive.GetVersion()) << "\n"
			<< "* Archive size:     " << StringUtils::AddThousandsSeparators(archiveSize / 1024) << " KiB\n"
			<< "* Unscrambled size: " << StringUtils::AddThousandsSeparators(archiveSize / bloatMultiplier / 1024) << " KiB\n"
			<< "* Checksum:         " << archive.GetChecksum() << (!verifyChecksum ? " (unverified)" : "") << "\n\n"

			<< "--- Scrambler Information ---\n"
			<< "* Bloat multiplier: " << bloatMultiplier << "\n"
			<< "* Obfuscator ID:    " << static_cast<int>(obfuscator->GetId()) << " (" << obfuscator->GetName() << ")\n";

		if (obfuscator->SupportsKey())
			stream << "* Obfuscator key:   " << obfuscator->GetKey() << "\n\n";
		else
			stream << "* Obfuscator key:   " << "Not supported" << "\n\n";

		stream << "--- File List ---\n"
			<< "* File count: " << fileCount << "\n\n";

		stream.Write(std::format("{:>4} | {:<50} | {:>20} | {:>22}\n", "#", "Path", "Scrambled size (KiB)", "Unscrambled size (KiB)"));
		stream.Write(std::string(105, '-') + "\n");

		for (size_t i = 0; i < fileCount; i++)
		{
			const uint64_t scrambledSize   = files[i].GetScrambledSize() / 1024;  // Convert bytes to KiB
			const uint64_t unscrambledSize = files[i].GetUnscrambledSize() / 1024;

			stream.Write(
				std::format("{:>4} | {:<50} | {:>20} | {:>22}\n",
					i + 1,
					StringUtils::Truncate(files[i].GetPath().generic_string(), 50),
					StringUtils::AddThousandsSeparators(scrambledSize != 0ui64 ? scrambledSize : 1ui64),
					StringUtils::AddThousandsSeparators(unscrambledSize != 0ui64 ? unscrambledSize : 1ui64)
				)
			);
		}

		std::cout << stream.GetData() << "\n";
	}

	inline void VerifyIntegrity() const
	{
		BloatArchive::Open(archivePath, true);
		std::cout << "No errors have been found.\n";
	}

	//template<std::convertible_to<fs::path>... Paths>
	void Create(const std::span<char*>& paths, const std::shared_ptr<Scrambler>& scrambler, const bool overwriteArchive,
		const bool recursive) const
	{
		if (fs::is_regular_file(archivePath))
		{
			if (!overwriteArchive)
				throw DuplicateFileException("Another archive with the same name already exists. Please specify \"--overwrite-archive\" to overwrite it.");
		}

		BloatArchive archive{};
		InternalAddEntriesToArchive(archive, paths, recursive, true);

		archive.SetScrambler(scrambler);
		archive.Save(archivePath, true);
	}

	inline void Append(const std::span<char*>& paths, const bool recursive, const bool overwriteExisting) const
	{
		BloatArchive archive = BloatArchive::Open(archivePath, verifyChecksum);
		InternalAddEntriesToArchive(archive, paths, recursive, overwriteExisting);

		archive.Save(archivePath, true);
	}

	inline void Remove(const std::span<char*>& paths) const
	{
		BloatArchive archive = BloatArchive::Open(archivePath, verifyChecksum);

		for (const fs::path& path : paths)
		{
			try
			{
				if (archive.DoesFileExist(path))
					archive.RemoveFile(path);
				else if (archive.DoesDirectoryExist(path))
					archive.RemoveDirectory(path);
				else
					DisplayPathError(path, "The specified path does not represent a valid file or directory.");
			}
			catch (const AggregateException& ex)
			{
				HandleAggregatePathException(ex);
			}
			catch (...)
			{
				HandlePathException(std::current_exception());
			}
		}

		archive.Save(archivePath, true);
	}

	inline void SetScrambler(const std::shared_ptr<Scrambler>& scrambler) const
	{
		BloatArchive archive = BloatArchive::Open(archivePath, verifyChecksum);

		archive.SetScrambler(scrambler);
		archive.Save(archivePath, true);
	}

	inline void Extract(const std::span<char*>& paths, const fs::path& outputDir, const bool overwriteExisting) const
	{
		const BloatArchive& archive = BloatArchive::Open(archivePath, verifyChecksum);

		for (const fs::path& path : paths)
		{
			try
			{
				archive.Extract(path, outputDir, overwriteExisting, true);
			}
			catch (...)
			{
				HandlePathException(std::current_exception());
			}
		}
	}

	inline void Extract(const fs::path& outputDir, const bool overwriteExisting) const
	{
		const BloatArchive& archive = BloatArchive::Open(archivePath, verifyChecksum);

		try
		{
			archive.Extract(outputDir, overwriteExisting);
		}
		catch (const AggregateException& ex)
		{
			HandleAggregatePathException(ex);
		}
	}
};
