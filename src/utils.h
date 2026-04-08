#pragma once

#include <unordered_set>

std::string getFilename(const std::string& path);
std::string formatFilename(const std::string& input);

template <size_t N>
std::unordered_set<std::string> filterSelected(
    const std::array<bool, N>& flags,
    const std::array<std::string_view, N>& tags)
{
    std::unordered_set<std::string> result;

    for (size_t i = 0; i < N; ++i)
    {
        if (flags[i])
        {
            result.insert(std::string(tags[i]));
        }
    }

    return result;
}