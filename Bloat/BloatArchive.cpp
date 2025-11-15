#include "BloatArchive.h"
#include "Bloater.h"
#include "Checksum.h"
#include "Exceptions.h"
#include "Stream.h"
#include "XorshiftRandom.h"

uint8_t BloatArchive::GetVersion() const noexcept { return version; }

std::shared_ptr<Scrambler> BloatArchive::GetScrambler() const noexcept { return scrambler; }
void BloatArchive::SetScrambler(const std::shared_ptr<Scrambler>& scrambler) noexcept { this->scrambler = scrambler; }

std::vector<ArchiveFile> BloatArchive::GetFiles() const noexcept { return files; }

uint64_t BloatArchive::GetChecksum()
{
	// Silly made-up hash accumulator function or something.
	// I have no idea whether it's decent or utter trash, but we'll call it the glorious BLOATSUM (tm).

	uint64_t checksum = 1610612741ull;
	uint64_t someWeirdPrime = 50331653ull;

	for (ArchiveFile& file : files)
	{
		const uint64_t fileChecksum = Checksum::Calculate(file.GetBytes());

		checksum ^= fileChecksum;
		checksum *= (402653189ull * someWeirdPrime) | 1;

		someWeirdPrime ^= checksum;
	}

	return checksum;
}

BloatArchive::BloatArchive() noexcept
	: scrambler(std::make_shared<Scrambler>(1ui64, ObfuscatorFactory::Create(ObfuscatorId::RandomXorObfuscator))) { }

BloatArchive BloatArchive::Open(const fs::path& archivePath)
{
	auto fs = std::make_shared<FileStream>(FileStream::OpenRead(archivePath));

	if (fs->ReadString(MAGIC_NUMBER.length()) != MAGIC_NUMBER)
		throw InvalidArchiveException("The correct archive magic number could not be detected.");

	BloatArchive archive{};
	archive.version = fs->Read<uint8_t>();

	if (archive.version != CURRENT_ARCHIVE_VERSION)
		throw InvalidArchiveException("The archive version is not supported by this version of BLOAT CLI.");

	const uint64_t bloatMultiplier = fs->Read<uint64_t>();
	const auto obfuscator = ObfuscatorFactory::Create(static_cast<ObfuscatorId>(fs->Read<uint8_t>()));

	const uint32_t key = fs->Read<uint32_t>();

	if (obfuscator->SupportsKey())
		obfuscator->SetKey(key);

	archive.scrambler = std::make_shared<Scrambler>(bloatMultiplier, obfuscator);

	const uint64_t checksum = fs->Read<uint64_t>();
	const uint64_t numFiles = fs->Read<uint64_t>();

	archive.files.reserve(numFiles);
	archive.filePaths.reserve(numFiles);
	
	for (uint64_t i = 0; i < numFiles; i++)
	{
		const uint64_t pathLength = fs->Read<uint64_t>();
		const std::string path = fs->ReadString(pathLength);

		const uint64_t byteLength = fs->Read<uint64_t>();

		archive.files.emplace_back(ArchiveFile(path, fs, archive.scrambler, fs->GetReadPosition(), byteLength));
		archive.filePaths.insert(path);

		fs->SetReadPosition(static_cast<uint64_t>(fs->GetReadPosition()) + byteLength);  // Skip to the next file
	}

	if (checksum != archive.GetChecksum())
		throw InvalidArchiveException("The archive is corrupted as there is a checksum mismatch.");

	return archive;
}

void BloatArchive::AddFile(const fs::path& filePath, const fs::path& basePath)
{
	const fs::path relativePath = fs::relative(filePath, basePath);

	if (filePaths.contains(filePath.string()))
		throw DuplicateFileException(std::format("Another file named \"{}\" already exists in the archive.", relativePath.string()));

	auto fs = std::make_shared<FileStream>(FileStream::OpenRead(filePath));
	files.emplace_back(ArchiveFile(relativePath, fs, scrambler));

	filePaths.insert(filePath.string());
}

void BloatArchive::AddFilesFromDirectory(const fs::path& dirPath, const bool recursive)
{
	if (!fs::exists(dirPath) || fs::is_directory(dirPath))
		throw std::invalid_argument("The specified path does not exist.");

	if (recursive)
	{
		for (const auto& file : fs::recursive_directory_iterator(dirPath, fs::directory_options::skip_permission_denied))
		{
			if (file.is_regular_file())
				AddFile(file.path(), dirPath);
		}

		return;
	}

	for (const auto& file : fs::directory_iterator(dirPath, fs::directory_options::skip_permission_denied))
	{
		// Exclude subdirectories (even top-level ones)

		if (file.is_regular_file())
			AddFile(file.path(), dirPath);
	}
}

void BloatArchive::Extract(const fs::path& destDir, const bool overwriteExistingFiles)
{
	for (ArchiveFile& file : files)
	{
		const fs::path outputFilePath = destDir / file.GetPath();
		const fs::path outputFileParentPath = outputFilePath.parent_path();

		if (!overwriteExistingFiles && fs::exists(outputFilePath) && fs::is_regular_file(outputFilePath))
			continue;

		if (!fs::exists(outputFileParentPath))
			fs::create_directories(outputFileParentPath);

		FileStream fs = FileStream::OpenWrite(outputFilePath, true);
		fs.Write(file.GetBytes());
	}
}

void BloatArchive::Save(const fs::path& destPath)
{
	// A regular FileStream (or std::ofstream) would require truncating the file,
	// making writing files impossible and risking archive corruption if any exceptions are thrown.
	MemoryStream ms{};
	
	// Write the header
	ms.Write(std::string{ MAGIC_NUMBER });                                         // Magic number       (offset 0)
	ms.Write<uint8_t>(CURRENT_ARCHIVE_VERSION);                                    // Archive version: 1 (offset 6)
	ms.Write<uint64_t>(scrambler->GetBloatMultiplier());                           // Bloat multiplier   (offset 7)
	ms.Write<uint8_t>(static_cast<uint8_t>(scrambler->GetObfuscator()->GetId()));  // Obfuscator ID		 (offset F)

	if (scrambler->GetObfuscator()->SupportsKey())
	{
		scrambler->GetObfuscator()->SetKey(XorshiftRandom().GetSeed());
		ms.Write<uint32_t>(scrambler->GetObfuscator()->GetKey());                  // Obfuscator key     (offset 10)
	}
	else
	{
		ms.Write<uint32_t>(0u);
	}

	ms.Write<uint64_t>(GetChecksum());                                             // Archive checksum   (offset 14)
	ms.Write<uint64_t>(uint64_t{ files.size() });                                  // Archive file count (offset 1C)

	// Write all files to the archive
	for (ArchiveFile& file : files)
	{
		std::vector<unsigned char> bytes = file.GetBytes();
		scrambler->Scramble(bytes);

		const std::string path = file.GetPath().string();

		ms.Write(uint64_t{ path.length() });
		ms.Write(path);

		ms.Write(uint64_t{ bytes.size() });
		ms.Write(bytes);
	}

	ms.WriteToFile(destPath);
}
