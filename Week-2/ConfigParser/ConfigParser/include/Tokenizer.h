#pragma once

#ifndef TOKENIZER
#define TOKENIZER

// Tokenizer is a stateless, value-oriented service: Tokenize() returns
// a vector of tokens. Tokens use one-based line and column numbers for
// diagnostics (user-facing). Token objects are immutable after creation.

#include "ConfigTypes.h"

#include <variant>
#include <vector>
#include <string>

class Tokenizer
{
public:
    Tokenizer () = default;
    ~Tokenizer () = default;

    // Tokenize the input string and return either a vector of Token or an Error.
    // Line and column numbers in Token are one-based.
    std::variant<std::vector<Token>, Error> Tokenize (const std::string &input);

private:
    // No per-instance state. tokenList removed to make the class stateless
};

#endif // !TOKENIZER
