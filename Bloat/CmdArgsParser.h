#pragma once
#include <span>
#include "Exceptions.h"
#include "Utils.h"

class CmdArgsParser
{
private:
    static constexpr inline const char* VERSION_INFO =
        "BLOAT 1.0.0 by Wirmaple73 (https://github.com/Wirmaple73/BLOAT) - 2025/11/21";

    static constexpr inline const char* USAGE_HELP =
R"(BLOAT is a general-purpose archive format that enlarges files and pisses compressors off because compression sucks.
BLOAT is the first revolt against the heinous CIA (Compression Intelligence Agency). VIVA LA REVOBLOATION!

USAGE:
  bloat help
  bloat version
  bloat info        <archive_path>
  bloat create      <archive_path> [switches...] <file1> [file2...]
  bloat add         <archive_path> [switches...] <file1> [file2...]
  bloat remove      <archive_path> [switches...] <file1> [file2...]
  bloat set         <archive_path> [switches...]
  bloat extract     <archive_path> <output_path> [switches...] <file1> [file2...]
  bloat extract-all <archive_path> <output_path> [switches...]


OPERATIONS:
  help                    Display this wall of text.
  version                 Display the current BLOAT version.
  info                    Display the file list and other information about the specified archive.
  create                  Create a new archive and add the specified files/directories to it.
  add                     Add the specified files/directories to an existing archive.
  remove                  Remove the specified files/directories from an archive.
  set                     Change the bloat multiplier and/or the obfuscator of an existing archive and rebuild.
  extract                 Extract the specified archive files to the specified path.
  extract-all             Extract all files to the specified path.


SWITCHES:
  -bm                     Specify the bloat multiplier the Bloater uses to enlarge every file in the archive.
                          Applicable to: create, set
                          Allowed values: Positive non-zero integers
                          Default value: 1 (no bloating is performed)

  -obid                   Specify the obfuscator ID used to eliminate byte patterns and confuse compressors.
                          Obfuscation is reversible and does not affect the archive size.
                          Applicable to: create, set
                          Allowed values: See the NOTES section below.
                          Default value: 1 (Random XOR obfuscator)

  -obkey                  Specify the key if the obfuscator supports it. See the NOTES section below.
                          Applicable to: create, set
                          Allowed values: Positive non-zero integers
                          Default value: A randomly-generated integer

  -password               create: Encrypt the archive with the specified password. NOT MEANT FOR ACTUAL PROTECTION.
                          Other operations: Use the specified password to open the archive if it's encrypted.
                          Applicable to: create, add, remove, set, extract, extract-all
                          Allowed values: Any (enclose the password in quotes if it contains space)
                          Default value: No password

  --no-subdirs            Do not include files from subdirectories when adding directories.
                          Applicable to: create, add
                          Disabled by default.

  --overwrite-archive     Overwrite the output archive if it already exists.
                          Applicable to: create
                          Disabled by default.

  --overwrite-files       add: Overwrite all files in the archive if they already exist.
                          extract & extract-all: Overwrite any existing files in the output directory.
                          Applicable to: add, extract, extract-all
                          Disabled by default.

  --no-pause              Exit immediately when done instead of waiting for key press.
                          Disabled by default.


EXAMPLES:
  * Create a new archive from D:\Folder with a random XOR obfuscator using 1234 as its key (state), specifying
    a bloat multiplier of 5, using "my password" as the password to encrypt the archive:
    bloat create "D:\My archive.blt" -obid 1 -obkey 1234 -bm 5 -password "my password" D:\Folder

  * Add D:\My file.txt and all files in D:\Folder to an existing archive, overwriting any files that already
    exist in the archive:
    bloat add "D:\My archive.blt" --overwrite-files D:\Folder "D:\My file.txt"

  * Add all files inside D:\Folder excluding subfolders to D:\archive.blt, overwriting archive files
    with the same name:
    bloat add D:\archive.blt --no-subdirs --overwrite-files D:\Folder

  * Remove the directory "My games/Half-Life" and everything inside it from games.blt, using "pass" as the
    password for opening the archive:
    bloat remove games.blt -password pass "My games/Half-Life"

  * Change the bloat multiplier of an existing archive to 100 and disable obfuscation:
    bloat set MyArchive.blt -bm 100 -obid 0

  * Extract the file "Folder/File.txt" and the directory "CIA classified files" to D:\Extracted, but
    don't overwrite any files with the same name in D:\Extracted:
    bloat extract "Secret archive.blt" D:\Extracted Folder/File.txt "CIA classified files"

  * Extract everything inside C:\Archive.blt to D:\Extracted, overwriting files that already exist:
    bloat extract-all C:\Archive.blt D:\Extracted --overwrite-files


NOTES:
  * For your convenience, files or directories (denoted by "<file1> [file2...]") can be dragged and dropped
    onto the executable.

  * Passwords, files, and directories containing spaces must be enclosed in quotes.

  * Supported obfuscators:
    - Empty obfuscator:
        ID: 0
        Supports custom key: No
        Description: Performs no obfuscation.

    - Random XOR obfuscator:
        ID: 1
        Supports custom key: Yes
        Description: Eliminates byte patterns by XOR'ing all bytes with numbers supplied by an RNG, which is
                     seeded with the custom key.


EXIT CODES:
  0: The operation completed successfully.
  1: One or more arguments are malformed.
  2: The archive is corrupted or is not a valid archive.
  3: The specified password is incorrect or no password was specified.
  4: An unexpected error occurred. The error message was written to the standard error stream.)";

    // Example: BLOAT.exe extract-all MyArchive.blt D:\OutputFolder
    static constexpr inline const int OPERATION_INDEX    = 1;
    static constexpr inline const int ARCHIVE_PATH_INDEX = 2;
    static constexpr inline const int OUTPUT_PATH_INDEX  = 3;

    static constexpr inline const int SWITCH_START_INDEX = 3;

    static constexpr inline const int DEFAULT_FILE_PATH_START_INDEX = 3;
    static constexpr inline const int DEFAULT_EXTRACTION_FILE_PATH_START_INDEX = 4;

    // Example: BLOAT.exe create MyArchive.blt [+ 8 arguments]
    //static constexpr inline const int MAX_ARG_SEARCH_INDEX = 10;

    const std::span<char*> args;

    inline bool DoesSwitchExist(const char* switchName) const noexcept
    {
        for (size_t i = SWITCH_START_INDEX; i < args.size(); i++)
        {
            if (Utils::String::AreEqualCaseInsensitive(args[i], switchName))
                return true;
        }

        return false;
    }

    inline std::optional<const char*> GetSwitchParameter(const char* switchName) const noexcept
    {
        for (size_t i = SWITCH_START_INDEX; i + 1 < args.size(); i++)
        {
            if (Utils::String::AreEqualCaseInsensitive(args[i], switchName))
                return args[i + 1];
        }

        return std::nullopt;
    }

    inline size_t GetLastSwitchIndex() const noexcept
    {
        size_t lastArgIndex = static_cast<size_t>(-1);

        for (size_t i = SWITCH_START_INDEX; i < args.size(); i++)
        {
            if (args[i][0] == '-')
                lastArgIndex = i;
        }

        return lastArgIndex;
    }

public:
    enum class Operation
    {
        Help, Version, Info, Create, Add, Remove, Extract, ExtractAll
    };

    enum class ExitCode
    {
        Success = 0, MalformedArgument = 1, InvalidArchive = 2, InvalidPassword = 3, UnexpectedError = 4
    };

    static inline const char* GetVersionInfo() noexcept { return VERSION_INFO; }
    static inline const char* GetUsageHelp() noexcept { return USAGE_HELP; }

    inline explicit CmdArgsParser(const int argc, char* argv[]) : args(argv, argc)
    {
        if (argc < OPERATION_INDEX + 1)
            throw MalformedArgumentException("No operation has been specified.");
    }

    inline bool IsPauseActivated() const noexcept
    {
        return !DoesSwitchExist("--no-pause");
    }

    inline Operation GetOperation() const
    {
        // Quick 'n' dirty
        if (Utils::String::AreEqualCaseInsensitive(args[OPERATION_INDEX], "help"))        return Operation::Help;
        if (Utils::String::AreEqualCaseInsensitive(args[OPERATION_INDEX], "version"))     return Operation::Version;
        if (Utils::String::AreEqualCaseInsensitive(args[OPERATION_INDEX], "info"))        return Operation::Info;
        if (Utils::String::AreEqualCaseInsensitive(args[OPERATION_INDEX], "create"))      return Operation::Create;
        if (Utils::String::AreEqualCaseInsensitive(args[OPERATION_INDEX], "add"))         return Operation::Add;
        if (Utils::String::AreEqualCaseInsensitive(args[OPERATION_INDEX], "remove"))      return Operation::Remove;
        if (Utils::String::AreEqualCaseInsensitive(args[OPERATION_INDEX], "extract"))     return Operation::Extract;
        if (Utils::String::AreEqualCaseInsensitive(args[OPERATION_INDEX], "extract-all")) return Operation::ExtractAll;

        throw MalformedArgumentException(std::format("The specified operation ('{}') could not be resolved.", args[OPERATION_INDEX]));
    }

    inline fs::path GetArchivePath() const
    {
        switch (GetOperation())
        {
            case Operation::Help:
            case Operation::Version:
                throw InvalidOperationException("The specified operation does not support an archive path.");
        }

        if (args.size() < static_cast<size_t>(ARCHIVE_PATH_INDEX + 1))
            throw MalformedArgumentException("No archive path has been specified.");
        
        return args[ARCHIVE_PATH_INDEX];
    }

    inline bool DoOverrideArchive() const noexcept { return DoesSwitchExist("--overwrite-archive"); }
    inline bool DoRecursion() const noexcept { return !DoesSwitchExist("--no-subdirs"); }

    inline uint64_t GetBloatMultiplier() const noexcept { return std::stoull(GetSwitchParameter("-bm").value_or("1")); }

    inline uint8_t GetObfuscatorId() const noexcept { return static_cast<uint8_t>(std::stoi(GetSwitchParameter("-obid").value_or("1"))); }
    inline uint64_t GetObfuscatorKey() const noexcept { return std::stoull(GetSwitchParameter("-obkey").value_or("0")); }

    inline std::string GetPassword() const noexcept { return GetSwitchParameter("-password").value_or(""); }

    inline std::span<char*> GetEntryPaths() const
    {
        size_t filePathStartIndex = DEFAULT_FILE_PATH_START_INDEX;
        size_t extractionFilePathStartIndex = DEFAULT_EXTRACTION_FILE_PATH_START_INDEX;

        const size_t lsi = GetLastSwitchIndex();

        if (lsi != static_cast<size_t>(-1))  // Was any switch passed?
            filePathStartIndex = extractionFilePathStartIndex = lsi + 1;

        const Operation operation = GetOperation();

        if (operation == Operation::Extract || operation == Operation::ExtractAll)
            return args.subspan(extractionFilePathStartIndex);

        return args.subspan(filePathStartIndex);
    }
};
