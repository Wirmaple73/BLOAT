#include <iostream>
#include <filesystem>
#include "ArchiveManipulator.h"
#include "BloatArchive.h"
#include "CmdArgsParser.h"
#include "Xorshift64Star.h"

namespace fs = std::filesystem;

using Operation = CmdArgsParser::Operation;
using ExitCode  = CmdArgsParser::ExitCode;

#if _WIN32
extern "C" __declspec(dllimport) int __stdcall SetConsoleTitleW(const wchar_t* lpConsoleTitle);
#endif

static inline int GetExitCode(const ExitCode exitCode, const bool pause) noexcept
{
	if (pause)
	{
		std::cout << "\nPress ENTER to continue...";
		std::cin.get();
	}

	return static_cast<int>(exitCode);
}

static inline ArchiveManipulator CreateArchiveManipulator(const CmdArgsParser& parser)
{
	switch (parser.GetOperation())
	{
		case Operation::Version:
		case Operation::Help:
			return ArchiveManipulator{};

		default:
			return ArchiveManipulator{ parser.GetArchivePath(), parser.GetPassword(), parser.DoChecksumVerification() };
	}
}

static inline std::shared_ptr<Scrambler> CreateScrambler(const CmdArgsParser& parser)
{
	auto scrambler = Scrambler::Create(
		parser.GetBloatMultiplier(), ObfuscatorFactory::Create(static_cast<ObfuscatorId>(parser.GetObfuscatorId()))
	);

	const uint64_t key = parser.GetObfuscatorKey();

	if (key != 0ui64)
		scrambler->GetObfuscator()->SetKey(key);

	return scrambler;
}

int main(int argc, char* argv[])
{
	std::ios::sync_with_stdio(false);

#if _WIN32
	SetConsoleTitleW(L"BLOAT");
#endif

	bool pause = true;
	bool showSuccessMessage = true;

	const auto& start = std::chrono::steady_clock::now();

	try
	{
		const CmdArgsParser& parser{ argc, argv };
		pause = parser.IsPauseActivated();

		const Operation operation = parser.GetOperation();
		const ArchiveManipulator& am = CreateArchiveManipulator(parser);

		switch (operation)
		{
			case Operation::Info:
				am.DisplayInfo();
				showSuccessMessage = false;
				break;

			case Operation::Verify:
				am.VerifyIntegrity();
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
	catch (const ChecksumMismatchException& ex)
	{
		std::cerr << "An error occurred while trying to open the specified archive: " << ex.what() << "\n"
			      << std::format("(archive checksum: {}, calculated checksum: {})\n",
					  ex.GetArchiveChecksum(), ex.GetCalculatedChecksum()
				  );

		return GetExitCode(ExitCode::ChecksumMismatch, pause);
	}
	catch (const std::invalid_argument& ex)
	{
		std::cerr << "An error occurred while parsing input arguments: " << ex.what() << "\n"
			      // << " One of the switches or its value is invalid.\n"
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

	const auto& end = std::chrono::steady_clock::now();

	if (showSuccessMessage)
		std::cout << std::format("The operation has been completed in {:.2f} seconds.\n", std::chrono::duration<double>(end - start).count());

	return GetExitCode(ExitCode::Success, pause);
}
