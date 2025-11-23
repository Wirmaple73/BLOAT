#pragma once
#include <stdexcept>
#include <string>

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
public:
    inline explicit DuplicateFileException(const std::string& message) noexcept : std::runtime_error(message) {}
};

class FileNotFoundException : public std::runtime_error
{
public:
    inline explicit FileNotFoundException(const std::string& message) noexcept : std::runtime_error(message) {}
};
