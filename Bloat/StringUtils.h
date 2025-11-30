#pragma once
#include <algorithm>
#include <concepts>
#include <string>
#include <vector>

class StringUtils
{
public:
    template<typename T>
    static inline std::basic_string<T> ToLower(std::basic_string<T> s) noexcept
    {
        std::transform(s.begin(), s.end(), s.begin(), [](const T c)
        {
            return static_cast<T>(std::tolower(c));
        });

        return s;
    }

    template<typename T>
    static inline bool AreEqualCaseInsensitive(const std::basic_string<T>& s1, const std::basic_string<T>& s2) noexcept
    {
        return ToLower(s1) == ToLower(s2);
    }

    static inline std::string Truncate(const std::string& s, const size_t maxLength) noexcept
    {
        // My file name.txt
        // Length: 10

        if (s.length() < maxLength)
            return s;

        return s.substr(0, maxLength - 3) + "...";
    }

    template<std::integral T>
    static inline std::string AddThousandsSeparators(const T value)
    {
        // Index:  0   1   2   3   4   5   6   7
        // -------------------------------------
        // Number: 7   5   7   8   2   1   2   4

        // Length: 8

        std::string s = std::to_string(value);
        int i = static_cast<int>(s.length()) - 3;

        while (i > 0)
        {
            s.insert(i, ",");
            i -= 3;
        }

        return s;
    }
};
