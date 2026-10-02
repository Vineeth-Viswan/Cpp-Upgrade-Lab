#pragma once

#ifndef FILTER_H
#define FILTER_H

#include <vector>
#include <optional>
#include <concepts>

template<typename T, std::predicate< const T&> F>
std::optional<std::vector<T>> Filter (const std::vector<T>& vec, F pred)
{
    std::vector<T> result;
    for (const auto &item: vec)
    {
        if (pred (item))
        {
            result.push_back (item);
        }
    }
    return std::make_optional (result);
}

#endif // !FILTER_H


