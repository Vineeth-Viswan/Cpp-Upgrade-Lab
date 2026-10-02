#pragma once

#ifndef FORMAT_H
#define FORMAT_H

#include <vector>
#include <string>
#include <concepts>

template<typename T>
concept ConvertibleToString = requires (T t) {
    { std::to_string (t) } -> std::convertible_to<std::string>;
};

template<ConvertibleToString T>
std::string Format (const std::vector<T> &vec, const std::string &delimiter = ", ")
{
    std::string result;
    for (size_t i = 0; i < vec.size (); ++i)
    {
        result += std::to_string (vec[i]);
        if (i < vec.size () - 1)
        {
            result += delimiter;
        }
    }
    return result;
}

#endif // !FORMAT_H
