
#include "Parser.h"
#include "Logger.h"
#include "StringUtil.h"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <optional>
#include <system_error>

namespace
{

    std::optional<bool> TryParseBoolean(const std::string& value)
    {
        if (value == "true")
        {
            return true;
        }

        if (value == "false")
        {
            return false;
        }

        return std::nullopt;
    }

    std::optional<std::int64_t> TryParseInteger(const std::string& value)
    {
        if (value.empty())
        {
            return std::nullopt;
        }

        const char* begin = value.data();
        const char* end = begin + value.size();
        std::int64_t parsed = 0;
        const auto status = std::from_chars(begin, end, parsed);

        if (status.ec != std::errc() || status.ptr != end)
        {
            return std::nullopt;
        }

        return parsed;
    }

    bool IsArray(const std::string& value)
    {
        const std::string trimmed = Trim(value);
        return !trimmed.empty() && trimmed.front() == '[' && trimmed.back() == ']';
    }

    std::variant<ScalarValue, Error> ParseScalarValue(const std::string& rawValue, int lineNumber)
    {
        const std::string value = Trim(rawValue);
        if (value.empty())
        {
            return Error{ lineNumber, 0, "Empty value" };
        }

        if (const auto boolValue = TryParseBoolean(value))
        {
            return ScalarValue{ *boolValue };
        }

        if (const auto integerValue = TryParseInteger(value))
        {
            return ScalarValue{ *integerValue };
        }

        return ScalarValue{ value };
    }

    std::variant<ArrayValue, Error> ParseArrayValue(const std::string& rawValue, int lineNumber)
    {
        const std::string trimmed = Trim(rawValue);
        if (trimmed.size() < 2 || trimmed.front() != '[' || trimmed.back() != ']')
        {
            return Error{ lineNumber, 0, "Malformed array" };
        }

        const std::string content = trimmed.substr(1, trimmed.size() - 2);
        if (content.empty())
        {
            return Error{ lineNumber, 0, "Array must contain at least one element" };
        }

        if (content.find('[') != std::string::npos || content.find(']') != std::string::npos)
        {
            return Error{ lineNumber, 0, "Nested arrays are not supported" };
        }

        std::vector<std::string> items;
        std::string current;
        for (char ch : content)
        {
            if (ch == ',')
            {
                items.push_back(current);
                current.clear();
            }
            else
            {
                current.push_back(ch);
            }
        }
        items.push_back(current);

        std::vector<ScalarValue> parsedValues;
        parsedValues.reserve(items.size());

        for (const std::string& item : items)
        {
            const std::string trimmedItem = Trim(item);
            if (trimmedItem.empty())
            {
                return Error{ lineNumber, 0, "Array elements cannot be empty" };
            }

            const auto parsed = ParseScalarValue(trimmedItem, lineNumber);
            if (std::holds_alternative<Error>(parsed))
            {
                return std::get<Error>(parsed);
            }

            parsedValues.push_back(std::get<ScalarValue>(parsed));
        }

        if (parsedValues.empty())
        {
            return Error{ lineNumber, 0, "Array must contain at least one element" };
        }

        bool allBool = true;
        bool allInt = true;
        bool allString = true;

        for (const auto& value : parsedValues)
        {
            if (!std::holds_alternative<bool>(value))
            {
                allBool = false;
            }

            if (!std::holds_alternative<std::int64_t>(value))
            {
                allInt = false;
            }

            if (!std::holds_alternative<std::string>(value))
            {
                allString = false;
            }
        }

        if (allBool)
        {
            BoolArray boolArray;
            boolArray.reserve(parsedValues.size());
            for (const auto& value : parsedValues)
            {
                boolArray.push_back(std::get<bool>(value));
            }
            return ArrayValue{ boolArray };
        }

        if (allInt)
        {
            IntArray intArray;
            intArray.reserve(parsedValues.size());
            for (const auto& value : parsedValues)
            {
                intArray.push_back(std::get<std::int64_t>(value));
            }
            return ArrayValue{ intArray };
        }

        if (allString)
        {
            StringArray stringArray;
            stringArray.reserve(parsedValues.size());
            for (const auto& value : parsedValues)
            {
                stringArray.push_back(std::get<std::string>(value));
            }
            return ArrayValue{ stringArray };
        }

        return Error{ lineNumber, 0, "Array elements must all be the same scalar type" };
    }

    std::variant<Value, Error> ParseValue(const std::string& valueStr, int lineNumber)
    {
        const std::string trimmed = Trim(valueStr);

        if (IsArray(trimmed))
        {
            const auto arrayResult = ParseArrayValue(trimmed, lineNumber);
            if (std::holds_alternative<Error>(arrayResult))
            {
                return std::get<Error>(arrayResult);
            }

            return Value{ std::get<ArrayValue>(arrayResult) };
        }

        const auto scalarResult = ParseScalarValue(trimmed, lineNumber);
        if (std::holds_alternative<Error>(scalarResult))
        {
            return std::get<Error>(scalarResult);
        }

        return Value{ std::get<ScalarValue>(scalarResult) };
    }
}

std::variant<Document, Error> Parser::Parse (const std::vector<Token>& tokens)
{
    Logger::Log(Logger::Level::INFO, "Starting to parse tokens. Total tokens: {}", tokens.size());

    Document doc;
    std::string currentSection;

    for (size_t i = 0; i < tokens.size(); ++i)
    {
        const Token& token = tokens[i];

        switch (token.type)
        {
            case TokenType::BlankLine:
            case TokenType::Comment:
                // Ignored on their own
                break;

            case TokenType::SectionHeader:
            {
                currentSection = token.value;
                Logger::Log(Logger::Level::DEBUG, "Found section: [{}] at line {}", currentSection, token.lineNumber);

                const auto [sectionIt, inserted] = doc.emplace(currentSection, Section{});
                if (!inserted)
                {
                    Logger::Log(Logger::Level::ERROR, "Section '{}' already exists at line {}", currentSection, token.lineNumber);
                    return Error { token.lineNumber, token.columnNumber, "Section already exists" };
                }
                break;
            }

            case TokenType::Key:
            {
                // Expect pattern: Key, Equals, Value
                if (currentSection.empty())
                {
                    Logger::Log(Logger::Level::ERROR, "Key '{}' found at line {} without a section header", token.value, token.lineNumber);
                    return Error { token.lineNumber, token.columnNumber, "Key found without section header" };
                }

                if (i + 2 >= tokens.size())
                {
                    Logger::Log(Logger::Level::ERROR, "Incomplete key-value pair at line {}", token.lineNumber);
                    return Error { token.lineNumber, token.columnNumber, "Incomplete key-value pair" };
                }

                const Token& next = tokens[i + 1];
                const Token& next2 = tokens[i + 2];

                if (next.type != TokenType::Equals)
                {
                    Logger::Log(Logger::Level::ERROR, "Expected '=' after key at line {}", next.lineNumber);
                    return Error { next.lineNumber, next.columnNumber, "Expected '=' after key" };
                }

                if (next2.type != TokenType::Value)
                {
                    Logger::Log(Logger::Level::ERROR, "Expected value after '=' at line {}", next2.lineNumber);
                    return Error { next2.lineNumber, next2.columnNumber, "Expected value after '='" };
                }

                const std::string& key = token.value;
                const std::string& valueStr = next2.value;

                Logger::Log(Logger::Level::DEBUG, "Parsing key '{}' with value '{}' in section [{}]", key, valueStr, currentSection);

                const auto valueOrError = ParseValue(valueStr, token.lineNumber);
                if (std::holds_alternative<Error>(valueOrError))
                {
                    return std::get<Error>(valueOrError);
                }

                auto& section = doc[currentSection];
                const auto [_, inserted] = section.emplace(key, std::get<Value>(valueOrError));
                if (!inserted)
                {
                    Logger::Log(Logger::Level::ERROR, "Key '{}' already exists in section [{}] at line {}", key, currentSection, token.lineNumber);
                    return Error { token.lineNumber, token.columnNumber, "Key already exists" };
                }

                // advance past Equals and Value
                i += 2;
                break;
            }

            case TokenType::Equals:
                Logger::Log(Logger::Level::ERROR, "Unexpected '=' at line {} column {}", token.lineNumber, token.columnNumber);
                return Error { token.lineNumber, token.columnNumber, "Unexpected '='" };

            case TokenType::Value:
                Logger::Log(Logger::Level::ERROR, "Unexpected value token at line {} column {}", token.lineNumber, token.columnNumber);
                return Error { token.lineNumber, token.columnNumber, "Unexpected value token" };

            default:
                Logger::Log(Logger::Level::ERROR, "Unknown token type at line {} column {}", token.lineNumber, token.columnNumber);
                return Error { token.lineNumber, token.columnNumber, "Unknown token type" };
        }
    }

    Logger::Log(Logger::Level::INFO, "Parsing completed successfully. Total sections: {}", doc.size());
    return doc;
}
