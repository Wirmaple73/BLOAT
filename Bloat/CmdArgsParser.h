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
  bloat verify      <archive_path>
  bloat info        <archive_path> [switches...]
  bloat create      <archive_path> [switches...] <file1> [file2...]
  bloat add         <archive_path> [switches...] <file1> [file2...]
  bloat remove      <archive_path> [switches...] <file1> [file2...]
  bloat set         <archive_path> [switches...]
  bloat extract     <archive_path> <output_path> [switches...] <file1> [file2...]
  bloat extract-all <archive_path> <output_path> [switches...]


OPERATIONS:
  help                    Display this wall of text.
  version                 Display the current BLOAT version.
  info                    Display the file table and information about the specified archive.
  verify                  Verify archive integrity by recalculating the checksum and ensuring it's valid.
  create                  Create a new archive and add the specified files/directories to it.
  add                     Add the specified files/directories to an existing archive.
  remove                  Remove the specified files/directories from an archive.
  set                     Change the bloat multiplier and/or the obfuscator of an existing archive, then rebuild it.
  extract                 Extract the specified archive files to the specified path.
  extract-all             Extract all files to the specified path.


SWITCHES:
  -bm                     Specify the bloat multiplier the Bloater uses to enlarge every file in the archive.
                          Applicable to: create, set
                          Allowed values: Positive non-zero integers
                          Default value: 1 (no bloating is performed)

  -obid                   Specify the obfuscator ID used to eliminate byte patterns and confuse compressors.
                          Obfuscation is reversible and does not affect archive size.
                          Applicable to: create, set
                          Allowed values: See the NOTES section below.
                          Default value: 1 (Random XOR obfuscator)

  -obkey                  Specify the key if the obfuscator supports it. See the NOTES section below.
                          Applicable to: create, set
                          Allowed values: Positive non-zero integers
                          Default value: A randomly-generated integer

  -password               create: Encrypt the archive with the specified password. NOT MEANT FOR ACTUAL PROTECTION.
                          Other operations: Use the specified password to open the archive if it's encrypted.
                          Applicable to: info, create, add, remove, set, extract, extract-all
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

  --no-verify             Skip integrity (checksum) verification to significantly speed up the archive opening phase,
                          especially for larger archives.

                          Note: If the operation is 'extract' or 'extract-all', extracted files may become corrupted
                                if the archive itself is corrupted. Nonetheless, archive integrity is still verified
                                when saving the archive.

                          Applicable to: info, add, remove, set, extract, extract-all
                          Disabled by default.

  --pause                 Wait for key press instead of immediately exiting when done.
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
  2: The archive is corrupted as there is a checksum mismatch.
  3: The specified password is incorrect or no password was specified.
  4: An unexpected error occurred. The error message was written to the standard error stream.)";

    // Example: BLOAT.exe extract-all MyArchive.blt D:\OutputFolder
    static constexpr inline const size_t OPERATION_INDEX    = 1;
    static constexpr inline const size_t ARCHIVE_PATH_INDEX = 2;
    static constexpr inline const size_t OUTPUT_DIR_INDEX   = 3;
    
    static constexpr inline const size_t SWITCH_START_INDEX = 3;

    static constexpr inline const size_t DEFAULT_FILE_PATH_START_INDEX = 3;
    static constexpr inline const size_t DEFAULT_EXTRACTION_FILE_PATH_START_INDEX = 4;

    // Example: BLOAT.exe create MyArchive.blt [+ 8 arguments]
    //static constexpr inline const size_t MAX_ARG_SEARCH_INDEX = 10;

    const std::span<char*> args;

    inline bool DoesSwitchExist(const char* switchName) const noexcept
    {
        for (size_t i = SWITCH_START_INDEX; i < args.size(); i++)
        {
            if (StringUtils::AreEqualCaseInsensitive<char>(args[i], switchName))
                return true;
        }

        return false;
    }

    inline std::optional<const char*> GetSwitchParameter(const char* switchName) const noexcept
    {
        for (size_t i = SWITCH_START_INDEX; i + 1 < args.size(); i++)
        {
            if (StringUtils::AreEqualCaseInsensitive<char>(args[i], switchName))
                return args[i + 1];
        }

        return std::nullopt;
    }

    inline size_t GetLastSwitchIndex() const
    {
        size_t lastArgIndex = static_cast<size_t>(-1);

        for (size_t i = SWITCH_START_INDEX; i < args.size(); i++)
        {
            if (args[i][0] != '-')
                continue;

            // Switches starting with "--" don't take any parameter, while those starting with a single "-" do.
            if (args[i][1] == '-')
            {
                lastArgIndex = i;
            }
            else
            {
                if (i + 1 >= args.size())
                    throw MalformedArgumentException(std::format("The switch '{}' expected a parameter (value) to be provided.", args[i]));

                lastArgIndex = i + 1;
            }
        }

        return lastArgIndex;
    }

public:
    enum class Operation
    {
        Help, Version, Info, Verify, Create, Add, Remove, Set, Extract, ExtractAll
    };

    enum class ExitCode
    {
        Success = 0, MalformedArgument = 1, ChecksumMismatch = 2, InvalidPassword = 3, UnexpectedError = 4
    };

    static inline const char* GetVersionInfo() noexcept { return VERSION_INFO; }
    static inline const char* GetUsageHelp() noexcept { return USAGE_HELP; }

    inline CmdArgsParser(const int argc, char* argv[]) : args(argv, argc)
    {
        if (argc < OPERATION_INDEX + 1)
            throw MalformedArgumentException("No operation has been specified.");
    }

    inline Operation GetOperation() const
    {
        const std::string& arg(args[OPERATION_INDEX]);

        // Quick 'n' dirty
        if (StringUtils::AreEqualCaseInsensitive<char>(arg, "help"))        return Operation::Help;
        if (StringUtils::AreEqualCaseInsensitive<char>(arg, "version"))     return Operation::Version;
        if (StringUtils::AreEqualCaseInsensitive<char>(arg, "info"))        return Operation::Info;
        if (StringUtils::AreEqualCaseInsensitive<char>(arg, "create"))      return Operation::Create;
        if (StringUtils::AreEqualCaseInsensitive<char>(arg, "add"))         return Operation::Add;
        if (StringUtils::AreEqualCaseInsensitive<char>(arg, "remove"))      return Operation::Remove;
        if (StringUtils::AreEqualCaseInsensitive<char>(arg, "set"))         return Operation::Set;
        if (StringUtils::AreEqualCaseInsensitive<char>(arg, "extract"))     return Operation::Extract;
        if (StringUtils::AreEqualCaseInsensitive<char>(arg, "extract-all")) return Operation::ExtractAll;

        throw MalformedArgumentException(std::format("The specified operation ('{}') could not be resolved.", arg));
    }

    inline fs::path GetArchivePath() const
    {
        switch (GetOperation())
        {
            case Operation::Help:
            case Operation::Version:
                throw InvalidOperationException("The specified operation does not support an archive path.");
        }

        if (args.size() < ARCHIVE_PATH_INDEX + 1)
            throw MalformedArgumentException("No archive path has been specified.");
        
        return args[ARCHIVE_PATH_INDEX];
    }

    inline bool IsPauseActivated() const noexcept { return DoesSwitchExist("--pause"); }

    inline bool DoChecksumVerification() const noexcept { return !DoesSwitchExist("--no-verify"); }
    inline bool DoOverwriteArchive() const noexcept { return DoesSwitchExist("--overwrite-archive"); }

    inline bool DoOverwriteFiles() const noexcept { return DoesSwitchExist("--overwrite-files"); }
    inline bool DoRecursion() const noexcept { return !DoesSwitchExist("--no-subdirs"); }

    inline uint64_t GetBloatMultiplier() const { return std::stoull(GetSwitchParameter("-bm").value_or("1")); }

    inline uint8_t GetObfuscatorId() const { return static_cast<uint8_t>(std::stoi(GetSwitchParameter("-obid").value_or("1"))); }
    inline uint64_t GetObfuscatorKey() const { return std::stoull(GetSwitchParameter("-obkey").value_or("0")); }

    inline std::string GetPassword() const noexcept { return GetSwitchParameter("-password").value_or(""); }

    inline fs::path GetOutputDirectory() const
    {
        switch (GetOperation())
        {
            case Operation::Extract:
            case Operation::ExtractAll:
                if (args.size() < OUTPUT_DIR_INDEX + 1)
                    throw MalformedArgumentException("No archive path has been specified.");

                return args[OUTPUT_DIR_INDEX];

            default:
                throw MalformedArgumentException("The specified operation does not take an output directory.");
        }
    }

    inline std::span<char*> GetEntryPaths() const
    {
        size_t filePathStartIndex = DEFAULT_FILE_PATH_START_INDEX;
        size_t extractionFilePathStartIndex = DEFAULT_EXTRACTION_FILE_PATH_START_INDEX;

        const size_t lsi = GetLastSwitchIndex();

        if (lsi != static_cast<size_t>(-1))  // Was any switch passed?
            filePathStartIndex = extractionFilePathStartIndex = lsi + 1;

        const Operation operation = GetOperation();

        const std::span<char*>& paths = (operation == Operation::Extract || operation == Operation::ExtractAll) ?
            args.subspan(extractionFilePathStartIndex) : args.subspan(filePathStartIndex);

        if (paths.size() == 0)
            throw MalformedArgumentException("No paths have been specified.");

        return paths;
    }
};
