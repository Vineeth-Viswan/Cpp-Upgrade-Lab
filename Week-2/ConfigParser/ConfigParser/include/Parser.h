#pragma once

// Parser is a stateless, value-oriented service: Parse() consumes a vector
// of tokens and returns either a Document or an Error. Line and column
// numbers carried in Token are one-based for user-facing diagnostics.

#include "Logger.h"
#include "ConfigTypes.h"

class Parser
{
public:
    Parser () = default;
    ~Parser () = default;
    std::variant<Document, Error> Parse (const std::vector<Token> &tokens);

private:
    // No per-instance state. Document member removed to keep Parser stateless.
};
