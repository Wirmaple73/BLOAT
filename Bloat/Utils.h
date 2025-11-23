#pragma once
#include <algorithm>
#include <concepts>
#include <string>
#include <vector>

class Utils
{
public:
    class String
    {
    public:
        static inline std::string ToLower(std::string s) noexcept
        {
            std::transform(s.begin(), s.end(), s.begin(), [](const unsigned char c)
            {
                return static_cast<unsigned char>(std::tolower(c));
            });

            return s;
        }

        static inline bool AreEqualCaseInsensitive(const std::string& s1, const std::string& s2) noexcept
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
            int i = static_cast<int>(s.length()) - 3;  // size_t underflows to something huge

            while (i > 0)
            {
                s.insert(i, ",");
                i -= 3;
            }

            return s;
        }
    };
};
