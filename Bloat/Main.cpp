#include <iostream>
#include <filesystem>
#include "ArchiveManipulator.h"
#include "BloatArchive.h"
#include "CmdArgsParser.h"
#include "Xorshift64Star.h"

namespace fs = std::filesystem;

using Operation = CmdArgsParser::Operation;
using ExitCode  = CmdArgsParser::ExitCode;

static inline int GetExitCode(const ExitCode exitCode, const bool pause) noexcept
{
	if (pause)
	{
		std::cout << "\nPress ENTER to continue...";
		std::cin.get();
	}

	return static_cast<int>(exitCode);
}

static inline std::shared_ptr<Scrambler> CreateScrambler(const CmdArgsParser& parser)
{
	auto scrambler = Scrambler::Create(
		parser.GetBloatMultiplier(), ObfuscatorFactory::Create(static_cast<ObfuscatorId>(parser.GetObfuscatorId()))
	);

	if (scrambler->GetObfuscator()->SupportsKey())
	{
		const uint64_t key = parser.GetObfuscatorKey();

		if (key != 0ui64)
			scrambler->GetObfuscator()->SetKey(key);
	}

	return scrambler;
}

int main(int argc, char* argv[])
{
	std::ios::sync_with_stdio(false);

	bool pause = true;
	bool showSuccessMessage = true;

	try
	{
		const CmdArgsParser parser(argc, argv);
		pause = parser.IsPauseActivated();

		ArchiveManipulator am(parser.GetArchivePath(), parser.GetPassword());

		switch (parser.GetOperation())
		{
			case Operation::Info:
				am.DisplayInfo();
				showSuccessMessage = false;
				break;

			case Operation::Create:
				am.Create(parser.GetEntryPaths(), CreateScrambler(parser), parser.DoOverwriteArchive(), parser.DoRecursion());
				break;

			case Operation::Add:
				am.Append(parser.GetEntryPaths(), parser.DoRecursion(), parser.DoOverwriteFiles());
				break;

			case Operation::Remove:
				am.Remove(parser.GetEntryPaths());
				break;

			case Operation::Set:
				am.SetScrambler(CreateScrambler(parser));
				break;

			case Operation::Extract:
				am.Extract(parser.GetEntryPaths(), parser.GetOutputDirectory(), parser.DoOverwriteFiles());
				break;

			case Operation::ExtractAll:
				am.Extract(parser.GetOutputDirectory(), parser.DoOverwriteFiles());
				break;

			case Operation::Version:
				std::cout << CmdArgsParser::GetVersionInfo() << "\n";
				showSuccessMessage = false;
				break;

			case Operation::Help:
			default:
				std::cout << CmdArgsParser::GetVersionInfo() << "\n\n" << CmdArgsParser::GetUsageHelp() << "\n";
				showSuccessMessage = false;
				break;
		}
	}
	catch (const MalformedArgumentException& ex)
	{
		std::cerr << "An error occurred while parsing input arguments: " << ex.what() << "\n"
			      << "Please enter 'bloat help' to view usage help.\n";

		return GetExitCode(ExitCode::MalformedArgument, pause);
	}
	catch (const std::invalid_argument& ex)
	{
		std::cerr << "An error occurred while parsing input arguments: " << ex.what()
			      << " One of the switches or its parameter (value) is invalid.\n"
			      << "Please enter 'bloat help' to view usage help.\n";

		return GetExitCode(ExitCode::MalformedArgument, pause);
	}
	catch (const std::exception& ex)
	{
		std::cerr << "An unexpected error has occurred: " << ex.what() << "\n";
		return GetExitCode(ExitCode::UnexpectedError, pause);
	}
	/*
	catch (...)
	{
		std::cerr << "An unknown error has occurred. Some weirdo threw a cursed exception that doesn't derive from std::exception, so no message is available. Sorry.";
		return GetExitCode(ExitCode::UnexpectedError, pause);
	}
	*/

	if (showSuccessMessage)
		std::cout << "The operation has been completed successfully.\n";

	return GetExitCode(ExitCode::Success, pause);
}
