
#include "Tokenizer.h"
#include "Logger.h"
#include "StringUtil.h"

std::variant<std::vector<Token>, Error> Tokenizer::Tokenize (const std::string &input)
{    
    // Do not print file contents to console/logs. Log only a non-sensitive summary.
    Logger::Log(Logger::Level::INFO, "Tokenizing input ({} bytes)", static_cast<int>(input.size()));

    std::vector<Token> tokens;
    std::string line;
    int lineNumber = 1;

    std::istringstream iss(input);
    while (std::getline(iss, line))
    {
        // Skip blank lines
        if (line.empty() || line.find_first_not_of(" \t") == std::string::npos)
        {
            tokens.push_back({ TokenType::BlankLine, "", lineNumber, 0 });
            ++lineNumber;
            continue;
        }

        // Skip comments
        if (line[line.find_first_not_of(" \t")] == ';')
        {
            tokens.push_back({ TokenType::Comment, line, lineNumber, 0 });
            ++lineNumber;
            continue;
        }

        // Parse section headers [SectionName]
        size_t firstNonSpace = line.find_first_not_of(" \t");
        if (firstNonSpace != std::string::npos && line[firstNonSpace] == '[')
        {
            size_t closePos = line.find(']', firstNonSpace);
            if (closePos != std::string::npos)
            {
                // Require closing bracket to be the final non-whitespace character
                size_t after = line.find_first_not_of(" \t", closePos + 1);
                if (after != std::string::npos)
                {
                    Logger::Log(Logger::Level::ERROR, "Unexpected characters after section header at line {} column {}", lineNumber, static_cast<int>(after));
                    return Error { lineNumber, static_cast<int>(after), "Unexpected characters after section header" };
                }

                std::string sectionName = line.substr(firstNonSpace + 1, closePos - firstNonSpace - 1);

                // Trim leading/trailing whitespace from section name
                size_t nameStart = sectionName.find_first_not_of(" \t");
                size_t nameEnd = sectionName.find_last_not_of(" \t");
                if (nameStart == std::string::npos || nameEnd == std::string::npos || nameStart > nameEnd)
                {
                    Logger::Log(Logger::Level::ERROR, "Empty section name in section header at line {} column {}", lineNumber, static_cast<int>(firstNonSpace + 1));
                    return Error { lineNumber, static_cast<int>(firstNonSpace + 1), "Empty section name in section header" };
                }

                sectionName = sectionName.substr(nameStart, nameEnd - nameStart + 1);

                tokens.push_back({ TokenType::SectionHeader, sectionName, lineNumber, static_cast<int>(firstNonSpace) });
                ++lineNumber;
                continue;
            }
            else
            {                
                Logger::Log(Logger::Level::ERROR, "Mismatched brackets in section header at line {}", lineNumber);
                return Error { lineNumber, static_cast<int>(firstNonSpace), "Mismatched brackets in section header" };
            }
        }

        // Parse key=value pairs
        size_t equalsPos = line.find('=');
        if (equalsPos != std::string::npos)
        {
            std::string rawKey = line.substr(0, equalsPos);
            std::string rawValue = line.substr(equalsPos + 1);

            // Trim key and value using centralized Trim
            std::string key = Trim(rawKey);
            std::string value = Trim(rawValue);

            // Calculate actual column for value (first non-space after '=' in the original rawValue)
            int valueColumn = static_cast<int>(equalsPos + 1);
            size_t valueStart = rawValue.find_first_not_of(" \t");
            if (valueStart != std::string::npos)
            {
                valueColumn += static_cast<int>(valueStart);
            }

            if (!key.empty())
            {
                tokens.push_back({ TokenType::Key, key, lineNumber, 0 });
                tokens.push_back({ TokenType::Equals, "=", lineNumber, static_cast<int>(equalsPos) });
                tokens.push_back({ TokenType::Value, value, lineNumber, valueColumn });
            }
            else
            {
                Logger::Log(Logger::Level::ERROR, "Empty key in key=value pair at line {}", lineNumber);
                return Error { lineNumber, 0, "Empty key in key=value pair" };
            }
        }
        else
        {
            Logger::Log(Logger::Level::ERROR, "Invalid line format at line {}: expected key=value pair", lineNumber);
            return Error { lineNumber, 0, "Invalid line format: expected key=value pair" };
        }

        ++lineNumber;
    }
    
    Logger::Log(Logger::Level::INFO, "Tokenization completed successfully. Total tokens: {}", tokens.size());
    return tokens;
}
