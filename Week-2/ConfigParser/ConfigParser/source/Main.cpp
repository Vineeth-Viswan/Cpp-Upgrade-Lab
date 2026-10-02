
#include "Logger.h"
#include "Tokenizer.h"
#include "Parser.h"

#include <fstream>
#include <iterator>

std::variant<std::string, Error> ReadFileContent (const std::string &filePath)
{
    std::ifstream file (filePath);
    if (!file.is_open ())
    {
        Logger::Log (Logger::Level::ERROR, "Failed to open file: {}", filePath);
        return Error {0, 0, "Failed to open file: " + filePath};
    }
    std::string content ((std::istreambuf_iterator<char> (file)), std::istreambuf_iterator<char> ());
    file.close ();
    return content;
}

int main (int argc, char *argv[])
{
    Logger::InitializeFromEnvironment ();

    Logger::Log (Logger::Level::INFO, "Config INI Parser!!", argc);

    std::string configFilePath = "test_config.ini";
    std::variant<std::string, Error> fileContentResult = ReadFileContent (configFilePath);

    if (std::holds_alternative<Error> (fileContentResult))
    {
        const Error& err = std::get<Error> (fileContentResult);
        Logger::Log (Logger::Level::ERROR, "File read error at line {}, column {}: {}",
                    err.lineNumber, err.columnNumber, err.message);
        return 1;
    }

    Tokenizer tokenizer;
    std::variant<std::vector<Token>, Error> tokenResult = tokenizer.Tokenize (std::get<std::string> (fileContentResult));

    if (std::holds_alternative<Error> (tokenResult))
    {
        const Error& err = std::get<Error> (tokenResult);
        Logger::Log (Logger::Level::ERROR, "Tokenization error at line {}, column {}: {}",
                    err.lineNumber, err.columnNumber, err.message);
        return 1;
    }

    std::vector<Token> tokens = std::get<std::vector<Token>> (tokenResult);

    Parser parser;
    std::variant<Document, Error> parseResult = parser.Parse (tokens);

    if (std::holds_alternative<Error> (parseResult))
    {
        const Error& err = std::get<Error> (parseResult);
        Logger::Log (Logger::Level::ERROR, "Parsing error at line {}, column {}: {}",
                    err.lineNumber, err.columnNumber, err.message);
        return 1;
    }

    Document doc = std::get<Document> (parseResult);

    return 0;
}
