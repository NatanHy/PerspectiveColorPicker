#include <filesystem>
#include <string>
#include <algorithm>
#include "utils.h"

std::string getFilename(const std::string& path) {
    return std::filesystem::path(path).filename().string();
}

std::string formatFilename(const std::string& input)
{
    std::string result = input;

    // Remove ".png" suffix if present
    if (result.ends_with(".png"))
    {
        result.erase(result.size() - 4);
    }

    // Replace '_' with ' '
    std::replace(result.begin(), result.end(), '_', ' ');

    return result;
}