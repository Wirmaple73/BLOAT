#pragma once
#include <stdexcept>
#include <string>

class AggregateException : public std::runtime_error
{
private:
    std::vector<std::exception_ptr> exceptions{};

public:
    inline explicit AggregateException() noexcept : std::runtime_error("One or more exceptions have been thrown.") {}

    inline const std::vector<std::exception_ptr>& GetInnerExceptions() const noexcept { return exceptions; }

    inline void Add(std::exception_ptr ex) noexcept
    {
        exceptions.emplace_back(std::move(ex));
    }

    inline void ThrowIfNonempty() const
    {
        if (!exceptions.empty())
            throw *this;
    }
};

class MalformedArgumentException : public std::runtime_error
{
public:
    inline explicit MalformedArgumentException(const std::string& message) noexcept : std::runtime_error(message) {}
};

class InvalidOperationException : public std::runtime_error
{
public:
    inline explicit InvalidOperationException(const std::string& message) noexcept : std::runtime_error(message) {}
};

class InvalidObfuscatorKeyException : public std::runtime_error
{
public:
    inline explicit InvalidObfuscatorKeyException(const std::string& message) noexcept : std::runtime_error(message) {}
};

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
