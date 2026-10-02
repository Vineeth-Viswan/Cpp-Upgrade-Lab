#pragma once

#ifndef PRINT_RANGE
#define PRINT_RANGE

#include <iostream>

template<std::ranges::range R>
void PrintRange (const R &values)
{
    for (const auto &value: values)
    {
        std::cout << value << ' ';
    }
    std::cout << '\n';
}

#endif // !PRINT_RANGE
