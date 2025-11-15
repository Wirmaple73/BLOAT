#pragma once
#include <stdexcept>
#include <string>

class InvalidArchiveException : public std::runtime_error
{
public:
    inline explicit InvalidArchiveException(const std::string& message) : std::runtime_error(message) {}
};

class DuplicateFileException : public std::runtime_error
{
public:
    inline explicit DuplicateFileException(const std::string& message) : std::runtime_error(message) {}
};
