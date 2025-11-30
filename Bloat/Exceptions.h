#pragma once
#include <stdexcept>
#include <string>

class InvalidArchiveException : public std::runtime_error
{
public:
    inline explicit InvalidArchiveException(const std::string& message) noexcept : std::runtime_error(message) {}
};

class DuplicateFileException : public std::runtime_error
{
private:
    fs::path filePath;

public:
    inline explicit DuplicateFileException(const fs::path& filePath) noexcept
        : std::runtime_error("Another file with the same name already exists in the specified path."), filePath(filePath) {}

    inline explicit DuplicateFileException(const std::string& message, const fs::path& filePath) noexcept
        : std::runtime_error(message), filePath(filePath) {}

    const fs::path& GetFilePath() const noexcept { return filePath; }
};

class FileNotFoundException : public std::runtime_error
{
private:
    fs::path filePath;

public:
    inline explicit FileNotFoundException(const std::string& message, const fs::path& filePath) noexcept
        : std::runtime_error(message), filePath(filePath) {}

    const fs::path& GetFilePath() const noexcept { return filePath; }
};
