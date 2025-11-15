#include "Helper.h"

#include <regex>

bool endsWith(const std::string& value, const std::string& ending)
{
    if (ending.size() > value.size())
        return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

std::string sanitizeIdentifier(const std::string& string)
{
    return std::regex_replace(string, std::regex("[^A-Za-z0-9_]"), "_");
}
