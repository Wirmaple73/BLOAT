#include <iostream>
#include <filesystem>
#include "BloatArchive.h"
#include "CmdArgsParser.h"
#include "OperationExecutor.h"
#include "Xorshift64Star.h"

namespace fs = std::filesystem;

using Operation = CmdArgsParser::Operation;
using ExitCode  = CmdArgsParser::ExitCode;

static inline int GetExitCode(const ExitCode exitCode, const bool pause) noexcept
{
	if (pause)
	{
		std::cout << "Press ENTER to continue...";
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
		scrambler->GetObfuscator()->SetKey(parser.GetObfuscatorKey());

	return scrambler;
}

int main(int argc, char* argv[])
{
	std::ios::sync_with_stdio(false);
	bool pause;

	try
	{
		const CmdArgsParser parser(argc, argv);
		pause = parser.IsPauseActivated();

		switch (parser.GetOperation())
		{
			case Operation::Version:
				OperationExecutor::DisplayProgramVersion();
				break;

			case Operation::Info:
				OperationExecutor::DisplayArchiveInfo(parser.GetArchivePath());
				break;

			case Operation::Create:
				OperationExecutor::CreateArchive(
					parser.GetArchivePath(), CreateScrambler(parser), parser.GetPassword(),
					parser.DoOverrideArchive(), parser.DoRecursion(), parser.GetEntryPaths()
				);
				break;

			case Operation::Help:
			default:
				OperationExecutor::DisplayProgramUsageHelp();
				break;
		}
	}
	catch (const MalformedArgumentException& ex)
	{
		std::cerr << "An error occurred while parsing input arguments: " << ex.what() << "\n";
		std::cerr << "Please enter 'bloat help' to view the usage help.\n";

		return GetExitCode(ExitCode::MalformedArgument, pause);
	}
	catch (const std::exception& ex)
	{
		std::cerr << "An unexpected error has occurred: " << ex.what() << "\n";
		return GetExitCode(ExitCode::UnexpectedError, pause);
	}
	catch (...)
	{
		std::cerr << "An unknown error has occurred. Some weirdo threw a cursed exception that doesn't derive from std::exception, so no message is available. Sorry.";
		return GetExitCode(ExitCode::UnexpectedError, pause);
	}

	return GetExitCode(ExitCode::Success, pause);
}
