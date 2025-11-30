#pragma once
#include <filesystem>
#include "BloatArchive.h"
#include "CmdArgsParser.h"

namespace fs = std::filesystem;

class OperationExecutor
{
private:
	static void AddEntriesToArchive(BloatArchive& archive, const bool recursive, const bool overwrite, const std::span<char*>& paths)
	{
		for (const char* pathString : paths)
		{
			const fs::path& path(pathString);

			try
			{
				if (fs::is_regular_file(path))
					archive.AddFile(path, overwrite);
				else if (fs::is_directory(path))
					archive.AddFilesFromDirectory(path, recursive, overwrite);
				else
					std::cout << "Skipping " << path << " as it does not represent a valid file or directory.\n";
			}
			catch (const DuplicateFileException& ex)
			{
				std::cout << "Skipping " << path << ": " << ex.what() << "\n";
			}
		}
	}

public:
	static inline void DisplayProgramVersion() noexcept
	{
		std::cout << CmdArgsParser::GetVersionInfo() << "\n\n";
	}

	static inline void DisplayProgramUsageHelp() noexcept
	{
		std::cout << CmdArgsParser::GetVersionInfo() << "\n\n" << CmdArgsParser::GetUsageHelp() << "\n\n";
	}

	static void DisplayArchiveInfo(const fs::path& path)
	{
		const BloatArchive& archive = BloatArchive::Open(path);
		const auto& obfuscator = archive.GetScrambler()->GetObfuscator();

		const uint64_t bloatMultiplier = archive.GetScrambler()->GetBloatMultiplier();
		const uint64_t archiveSize = fs::file_size(path);

		// MSVC's std::format has gotta be the dumbest formatting function on Earth. MSVC can't even give proper error
		// messages (as expected from Microsoft). Why does C++ keep caring about someone's legacy MFC application from
		// 1998 and doesn't add QoL features? Heck, even the committee doesn't even bother to add an ASCII-only ToLower
		// function for strings. Ditch C and move on to modernize C++ like other sane languages lol

		const auto& files = archive.GetAllFiles();
		const size_t fileCount = files.size();

		MemoryStream stream{};

		stream << "Archive name: " << path.filename().string() << "\n\n"

			<< "--- General Information ---\n"
			<< "* Archive version: " << static_cast<int>(archive.GetVersion()) << "\n"
			<< "* Archive size: " << archiveSize / 1024.0 << " KB\n"
			<< "* Unscrambled size: " << archiveSize / static_cast<double>(bloatMultiplier) / 1024 << " KB\n"
			<< "* Checksum: " << archive.GetChecksum() << "\n\n"

			<< "--- Scrambler Information ---\n"
			<< "* Bloat multiplier: " << bloatMultiplier << "\n"
			<< "* Obfuscator ID: " << static_cast<int>(obfuscator->GetId()) << " (" << obfuscator->GetName() << ")\n"
			<< "* Obfuscator key: " << obfuscator->GetKey() << "\n\n"

			<< "--- File List ---\n"
			<< "* File count: " << files.size() << "\n\n";

		stream.Write(std::format("{:>4} | {:<50} | {:>20} | {:>22}\n", "#", "Name", "Scrambled size (KiB)", "Unscrambled size (KiB)"));
		stream.Write(std::string(105, '-') + "\n");

		for (size_t i = 0; i < fileCount; i++)
		{
			stream.Write(
				std::format("{:>4} | {:<50} | {:>20} | {:>22}\n",
					i + 1,
					Utils::String::Truncate(files[i].GetPath().generic_string(), 50),
					Utils::String::AddThousandsSeparators(files[i].GetSize()),
					Utils::String::AddThousandsSeparators(files[i].GetUnscrambledSize())
				)
			);
		}

		std::cout << stream.GetData() << "\n";
	}

	//template<std::convertible_to<fs::path>... Paths>
	static void CreateArchive(
		const fs::path& path, const std::shared_ptr<Scrambler>& scrambler, const std::string& password,
		const bool overwriteArchive, const bool recursive, const std::span<char*>& paths)
	{
		if (fs::is_regular_file(path))
		{
			if (!overwriteArchive)
				throw DuplicateFileException("Another archive with the same name already exists. Please specify --overwrite-archive to overwrite it.");

			fs::remove(path);
		}

		BloatArchive archive{};
		AddEntriesToArchive(archive, recursive, true, paths);

		archive.SetScrambler(scrambler);
		// TODO: Handle custom passwords

		archive.Save(path, true);
	}
};
