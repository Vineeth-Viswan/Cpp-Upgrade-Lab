
#include "Logger.h"
#include "Search.h"
#include "Filter.h"
#include "Format.h"
#include "PrintRange.h"

#include <vector>
#include <string>
#include <array>

// Type with no comparison operators
class NoComparison
{
    int value;

public:    
    NoComparison (int v = 0) : value (v) { }

    // Explicitly delete comparison operators
    bool operator< (const NoComparison &) const = delete;
    bool operator> (const NoComparison &) const = delete;
    bool operator<= (const NoComparison &) const = delete;
    bool operator>= (const NoComparison &) const = delete;
    bool operator== (const NoComparison &) const = delete;
};

int main ()
{
    // Initialize logger from environment variables
    Logger::InitializeFromEnvironment ();
    Logger::Log (Logger::Level::INFO, "Constrained Algorithms Library!");

    // Example usage of Search    
    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto result = Search (vec, [](int x) { return x == 3; });
    Logger::Log (Logger::Level::INFO, "Searching for 3 in vector: {}", result.has_value () ? "Found" : "Not Found");

    // Example usage of Filter
    auto filtered = Filter (vec, [] (int x) { return x % 2 == 0; });
    Logger::Log (Logger::Level::INFO, "Filtered even numbers: {}", Format (filtered.value ()));

    // Example usage of Format
    std::string formatted = Format (vec, " | ");
    Logger::Log (Logger::Level::INFO, "Formatted vector: {}", formatted);

    // Case that breaks - NoComparison has deleted operator==
    // std::vector<NoComparison> vec2 = {NoComparison (1), NoComparison (2), NoComparison (3)};
    // auto result2 = Search (vec2, NoComparison (2));  // This will fail to compile!

    std::vector<int> numbers {1, 2, 3};
    std::array<int, 3> values {4, 5, 6};

    PrintRange (numbers);
    PrintRange (values);
    PrintRange (std::string {"hello"});

    return 0;
}
