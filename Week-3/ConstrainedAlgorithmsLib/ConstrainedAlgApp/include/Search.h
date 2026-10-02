#pragma once

#ifndef SEARCH_H
#define SEARCH_H

#include <vector>
#include <optional>
#include <algorithm>
#include <concepts>

template <typename T, std::predicate <const T&> F>
std::optional<T> Search (const std::vector<T> &vec, F pred)
{
    auto it = std::find_if (vec.begin (), vec.end (), pred);
    return  (it != vec.end ()) ? std::make_optional (*it) : std::nullopt;
}

#endif // !SEARCH_H
