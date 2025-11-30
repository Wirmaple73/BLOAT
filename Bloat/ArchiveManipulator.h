#pragma once
#include <filesystem>
#include "BloatArchive.h"
#include "CmdArgsParser.h"
#include "StringUtils.h"

namespace fs = std::filesystem;

class ArchiveManipulator
{
private:
	const fs::path archivePath;
	const std::string password;

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
				HandleAggregatePathException(ex, path);
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
			DisplayPathError(ex.GetFilePath(), std::string(ex.what()) + " (Specify \"--overwrite-files\" to force overwrite.)");
		}
		catch (const FileNotFoundException& ex)
		{
			DisplayPathError(ex.GetFilePath(), ex.what());
		}
		catch (...)  // Probably something fatal. Abort and let main() handle it.
		{
			throw;
		}
	}

	static inline void HandleAggregatePathException(const AggregateException& ex, const fs::path& originalPath)
	{
		for (const std::exception_ptr& inner : ex.GetInnerExceptions())
			HandlePathException(inner);
	}

	static inline void DisplayPathError(const fs::path& path, const std::string& message) noexcept
	{
		std::cout << "Skipping \"" << path.generic_string() << "\": " << message << "\n";
	}

public:
	inline explicit ArchiveManipulator(const fs::path& archivePath, const std::string& password) noexcept
		: archivePath(archivePath), password(password) { }

	void DisplayInfo() const
	{
		const BloatArchive& archive = BloatArchive::Open(archivePath);
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

		stream << "Archive name: " << archivePath.filename().string() << "\n\n"

			<< "--- General Information ---\n"
			<< "* Archive version: "  << static_cast<int>(archive.GetVersion()) << "\n"
			<< "* Archive size: "     << archiveSize / 1024.0 << " KB\n"
			<< "* Unscrambled size: " << archiveSize / static_cast<double>(bloatMultiplier) / 1024 << " KB\n"
			<< "* Checksum: "         << archive.GetChecksum() << "\n\n"

			<< "--- Scrambler Information ---\n"
			<< "* Bloat multiplier: " << bloatMultiplier << "\n"
			<< "* Obfuscator ID: "    << static_cast<int>(obfuscator->GetId()) << " (" << obfuscator->GetName() << ")\n"
			<< "* Obfuscator key: "   << obfuscator->GetKey() << "\n\n"

			<< "--- File List ---\n"
			<< "* File count: " << fileCount << "\n\n";

		stream.Write(std::format("{:>4} | {:<50} | {:>20} | {:>22}\n", "#", "Path", "Scrambled size (KiB)", "Unscrambled size (KiB)"));
		stream.Write(std::string(105, '-') + "\n");

		for (size_t i = 0; i < fileCount; i++)
		{
			stream.Write(
				std::format("{:>4} | {:<50} | {:>20} | {:>22}\n",
					i + 1,
					StringUtils::Truncate(files[i].GetPath().generic_string(), 50),
					StringUtils::AddThousandsSeparators(files[i].GetScrambledSize()),
					StringUtils::AddThousandsSeparators(files[i].GetUnscrambledSize())
				)
			);
		}

		std::cout << stream.GetData() << "\n";
	}

	//template<std::convertible_to<fs::path>... Paths>
	void Create(const std::span<char*>& paths, const std::shared_ptr<Scrambler>& scrambler, const bool overwriteArchive,
		const bool recursive)
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

	inline void Append(const std::span<char*>& paths, const bool recursive, const bool overwriteExisting)
	{
		BloatArchive archive = BloatArchive::Open(archivePath);
		InternalAddEntriesToArchive(archive, paths, recursive, overwriteExisting);

		archive.Save(archivePath, true);
	}

	inline void Remove(const std::span<char*>& paths)
	{
		BloatArchive archive = BloatArchive::Open(archivePath);

		for (const fs::path& path : paths)
		{
			try
			{
				if (fs::is_regular_file(path))
					archive.RemoveFile(path);
				else if (fs::is_directory(path))
					archive.RemoveDirectory(path);
				else
					DisplayPathError(path, "The specified path does not represent a valid file or directory.");
			}
			catch (const AggregateException& ex)
			{
				HandleAggregatePathException(ex, path);
			}
			catch (...)
			{
				HandlePathException(std::current_exception());
			}
		}

		archive.Save(archivePath, true);
	}

	inline void SetScrambler(const std::shared_ptr<Scrambler>& scrambler)
	{
		BloatArchive archive = BloatArchive::Open(archivePath);

		archive.SetScrambler(scrambler);
		archive.Save(archivePath, true);
	}

	inline void Extract(const std::span<char*>& paths, const fs::path& outputDir, const bool overwriteExisting)
	{
		const BloatArchive& archive = BloatArchive::Open(archivePath);

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

	inline void Extract(const fs::path& outputDir, const bool overwriteExisting)
	{
		const BloatArchive& archive = BloatArchive::Open(archivePath);

		try
		{
			archive.Extract(outputDir, overwriteExisting);
		}
		catch (const AggregateException& ex)
		{
			HandleAggregatePathException(ex, outputDir);
		}
	}
};
