#pragma once
#include <vector>

class Utils
{
public:
    class Vector
    {
    public:
        template <typename T>
        static bool Contains(const std::vector<T>& vector, const T& value)
        {
            for (const T& element : vector)
            {
                if (element == value)
                    return true;
            }

            return false;
        }
    };
};
