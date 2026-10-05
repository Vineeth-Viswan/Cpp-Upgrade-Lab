#pragma once

#ifndef LOG_LOADING
#define LOG_LOADING

#include <fstream>
#include <string>
#include <stdexcept>
#include <vector>

#include "log_types.h"

class LogSource
{
public:
    LogSource (const std::string &filePath)
    {
        std::ifstream file (filePath);
        if (!file.is_open ())
        {
            throw std::runtime_error ("Could not open log file: " + filePath);
        }
        std::string line;
        while (std::getline (file, line))
        {
            lines.push_back ({ lines.size () + 1, line });
        }
    }
    const std::vector<RawLine> &GetLines () const { return lines; }

private:
    std::vector<RawLine> lines;
};

#endif // LOG_LOADING
