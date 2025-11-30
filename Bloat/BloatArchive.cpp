#pragma once
#include <execution>
#include "BloatArchive.h"
#include "Exceptions.h"
#include "Obfuscator.h"
#include "Stream.h"
#include "StringUtils.h"
#include "SplitMix64.h"
#include "Xorshift64Star.h"

// Private methods

size_t BloatArchive::GetActiveFileCount() const noexcept
{
	auto view = files | std::views::filter([](const ArchiveFile& file) { return !file.IsRemoved(); });
	return std::ranges::distance(view);
}

void BloatArchive::ThrowIfFileDoesNotExist(const fs::path& filePath) const
{
	if (!DoesFileExist(filePath))
		throw FileNotFoundException("The specified file does not exist in the archive.", filePath);
}

ArchiveFile& BloatArchive::GetFileOrThrow(const fs::path& filePath)
{
	ThrowIfFileDoesNotExist(filePath);
	return files[fileIndices.at(filePath)];
}

void BloatArchive::ExtractFile(const ArchiveFile& file, const fs::path& destDir,
	const bool overwriteExisting, const bool throwIfRemoved, const bool throwIfDuplicated) const
{
	if (file.IsRemoved())
	{
		if (throwIfRemoved)
			throw FileNotFoundException("The specified file is marked for removal.", file.GetPath());

		return;
	}

	const fs::path& outputFilePath = destDir / file.GetPath();
	const fs::path& outputFileParentPath = outputFilePath.parent_path();

	if (!overwriteExisting && fs::is_regular_file(outputFilePath))
	{
		if (throwIfDuplicated)
			throw DuplicateFileException(outputFilePath);

		return;
	}

	if (!fs::exists(outputFileParentPath))
		fs::create_directories(outputFileParentPath);

	FileStream::OpenWrite(outputFilePath, true).Write(file.GetBytes());
}

// Public methods

BloatArchive::BloatArchive() noexcept
	: scrambler(Scrambler::Create(1ui64, ObfuscatorFactory::Create(ObfuscatorId::RandomXorObfuscator))) { }

uint8_t BloatArchive::GetVersion() const noexcept { return version; }

uint64_t BloatArchive::GetScrambledSize() const
{
	uint64_t size = 0ui64;

	for (const ArchiveFile& file : files)
		size += file.GetScrambledSize();

	return size;
}

uint64_t BloatArchive::GetUnscrambledSize() const
{
	uint64_t size = 0ui64;

	for (const ArchiveFile& file : files)
		size += file.GetUnscrambledSize();

	return size;
}

const std::shared_ptr<Scrambler>& BloatArchive::GetScrambler() const noexcept { return scrambler; }
void BloatArchive::SetScrambler(const std::shared_ptr<Scrambler>& scrambler) noexcept { this->scrambler = scrambler; }

uint64_t BloatArchive::GetChecksum() const
{
	if (isChecksumUpToDate)
		return checksum;

	// TODO: Bypass the CRT's dumb 512 (or 2048) file limit and parallelize

	//std::vector<uint64_t> hashes(GetActiveFileCount());
	//std::vector<uint64_t> indices(numFiles);
	//std::iota(indices.begin(), indices.end(), 0);  // Fill indices with 0, 1, 2, ..., numFiles - 1
	
	//std::for_each(std::execution::par_unseq, indices.begin(), indices.end(), [this, &hashes](const uint64_t i)
	//{
	//	hashes[i] = SplitMix64::ComputeHash(files[i].GetBytes());
	//});

	uint64_t acc = 0xcbf29ce484222325ui64;  // FNV offset basis number - should be a good starting value

	for (const ArchiveFile& file : files)
	{
		if (!file.IsRemoved())
			acc ^= SplitMix64::Mix(acc ^ SplitMix64::ComputeHash(file.GetBytes()));
	}

	checksum = acc;
	isChecksumUpToDate = true;

	return checksum;
}

BloatArchive BloatArchive::Open(const fs::path& archivePath)
{
	if (!fs::is_regular_file(archivePath))
		throw std::invalid_argument("The specified path does not represent a BLOAT archive.");

	auto fs = std::make_shared<FileStream>(FileStream::OpenRead(archivePath));

	if (fs->ReadString(MAGIC_NUMBER.length()) != MAGIC_NUMBER)
		throw InvalidArchiveException("The correct archive magic number could not be detected.");

	BloatArchive archive{};
	archive.version = fs->Read<uint8_t>();

	if (archive.version != CURRENT_ARCHIVE_VERSION)
		throw InvalidArchiveException("The archive version is unsupported.");

	const uint64_t bloatMultiplier = fs->Read<uint64_t>();
	const auto obfuscator = ObfuscatorFactory::Create(static_cast<ObfuscatorId>(fs->Read<uint8_t>()));

	const uint64_t key = fs->Read<uint64_t>();

	if (obfuscator->SupportsKey())
		obfuscator->SetKey(key);

	archive.scrambler = std::make_shared<Scrambler>(bloatMultiplier, obfuscator);

	const uint64_t checksum = fs->Read<uint64_t>();
	const uint64_t numFiles = fs->Read<uint64_t>();

	archive.files.reserve(numFiles);
	archive.fileIndices.reserve(numFiles);
	
	for (uint64_t i = 0; i < numFiles; i++)
	{
		const uint64_t pathLength = fs->Read<uint64_t>();
		const fs::path path(fs->ReadString(pathLength));

		const uint64_t byteLength = fs->Read<uint64_t>();

		// TODO: Trim redundant bytes
		archive.files.emplace_back(ArchiveFile(path, archive.scrambler, archivePath, fs->GetReadPosition(), byteLength));
		archive.fileIndices[path] = archive.files.size() - 1;

		fs->SetReadPosition(static_cast<uint64_t>(fs->GetReadPosition()) + byteLength);  // Skip to the next file
	}

	if (checksum != archive.GetChecksum())
		throw InvalidArchiveException("The archive is corrupted as there is a checksum mismatch.");

	return archive;
}

void BloatArchive::AddFile(const fs::path& filePath, const fs::path& relativePath, const bool overwriteExisting)
{
	if (DoesFileExist(relativePath))
	{
		if (overwriteExisting)
			RemoveFile(relativePath);
		else
			throw DuplicateFileException("Another file with the same name already exists in the archive.", filePath);
	}

	files.emplace_back(ArchiveFile(filePath, relativePath, scrambler));
	fileIndices[relativePath] = files.size() - 1;

	isChecksumUpToDate = false;
}

void BloatArchive::AddFile(const fs::path& filePath, const bool overwriteExisting)
{
	AddFile(filePath, filePath.filename(), overwriteExisting);
}

void BloatArchive::AddDirectory(const fs::path& dirPath, const bool recursive, const bool overwriteExisting)
{
	if (!fs::is_directory(dirPath))
		throw std::invalid_argument("The specified path does not exist or is not a valid directory.");

	AggregateException exceptions{};

	const auto& addFile = [this, &exceptions, &dirPath, overwriteExisting](const fs::directory_entry& entry) -> void
	{
		try
		{
			if (entry.is_regular_file())
				AddFile(entry.path(), fs::relative(entry.path(), dirPath), overwriteExisting);
		}
		catch (...)
		{
			exceptions.Add(std::current_exception());
		}
	};
	
	if (recursive)
	{
		for (const auto& file : fs::recursive_directory_iterator(dirPath, fs::directory_options::skip_permission_denied))
			addFile(file);
	}
	else
	{
		for (const auto& file : fs::directory_iterator(dirPath, fs::directory_options::skip_permission_denied))
			addFile(file);
	}

	exceptions.ThrowIfNonempty();
}

void BloatArchive::RemoveFile(const fs::path& filePath)
{
	GetFileOrThrow(filePath).MarkAsRemoved();
	isChecksumUpToDate = false;
}

void BloatArchive::RemoveDirectory(const fs::path& dirPath)
{
	auto normalizedDir = StringUtils::ToLower(dirPath.generic_u8string());

	if (!normalizedDir.ends_with('/'))
		normalizedDir += '/';

	for (ArchiveFile& file : files)
	{
		if (StringUtils::ToLower(file.GetPath().generic_u8string()).starts_with(normalizedDir))
		{
			file.MarkAsRemoved();
			isChecksumUpToDate = false;
		}
	}
}

const std::vector<ArchiveFile>& BloatArchive::GetAllFiles() const noexcept { return files; }

const ArchiveFile& BloatArchive::GetFile(const fs::path& filePath) const
{
	ThrowIfFileDoesNotExist(filePath);
	return files[fileIndices.at(filePath)];
}

bool BloatArchive::DoesFileExist(const fs::path& filePath) const noexcept
{
	return fileIndices.contains(filePath) && !files[fileIndices.at(filePath)].IsRemoved();
}

void BloatArchive::Extract(const fs::path& destDir, const bool overwriteExistingFiles) const
{
	AggregateException exceptions{};

	for (const ArchiveFile& file : files)
	{
		try
		{
			ExtractFile(file, destDir, overwriteExistingFiles, false, true);
		}
		catch (...)
		{
			exceptions.Add(std::current_exception());
		}
	}

	exceptions.ThrowIfNonempty();
}

void BloatArchive::Extract(const fs::path& filePath, const fs::path& destDir, const bool overwriteExisting, const bool throwIfDuplicated) const
{
	ExtractFile(GetFile(filePath), destDir, overwriteExisting, true, throwIfDuplicated);
}

void BloatArchive::Save(const fs::path& destPath, const bool overwrite) const
{
	if (files.empty())
		throw InvalidArchiveException("The archive must contain at least one file.");

	fs::path tempPath = destPath;
	tempPath += ".tmp";

	if (!overwrite && (fs::is_regular_file(destPath) || fs::is_regular_file(tempPath)))
		throw DuplicateFileException(destPath);

	FileStream ts = FileStream::OpenWrite(tempPath, true);

	try
	{
		// Write the header
		ts.Write(std::string{ MAGIC_NUMBER });                                         // Magic number       (offset 0x0)
		ts.Write<uint8_t>(CURRENT_ARCHIVE_VERSION);                                    // Archive version: 1 (offset 0x7)
		ts.Write<uint64_t>(scrambler->GetBloatMultiplier());                           // Bloat multiplier   (offset 0x8)
		ts.Write<uint8_t>(static_cast<uint8_t>(scrambler->GetObfuscator()->GetId()));  // Obfuscator ID		 (offset 0x10)

		if (scrambler->GetObfuscator()->SupportsKey())
			ts.Write<uint64_t>(scrambler->GetObfuscator()->GetKey());                  // Obfuscator key     (offset 0x11)
		else
			ts.Write<uint64_t>(0ui64);

		ts.Write<uint64_t>(GetChecksum());                                             // Archive checksum   (offset 0x19)
		ts.Write<uint64_t>(uint64_t{ GetActiveFileCount() });                          // Archive file count (offset 0x21)

		// Write all files to the archive
		for (const ArchiveFile& file : files)
		{
			if (file.IsRemoved())
				continue;

			/* File structure:
			* Path length (uint64)
			* Path
			* Data length (uint64)
			* Scrambled bytes
			*/

			std::vector<unsigned char> bytes = file.GetBytes();
			scrambler->Scramble(bytes);

			const std::u8string path = file.GetPath().generic_u8string();

			ts.Write(static_cast<uint64_t>(path.length()));
			ts.Write(path);

			ts.Write(static_cast<uint64_t>(bytes.size()));
			ts.Write(bytes);
		}
		
		ts.Close();

		if (fs::is_regular_file(destPath))
			fs::remove(destPath);

		fs::rename(tempPath, destPath);
	}
	catch (...)
	{
		try
		{
			ts.Close();
			fs::remove(tempPath);
		}
		catch (...) { /* Swallow to preserve the original exception */ }

		throw;
	}
}
