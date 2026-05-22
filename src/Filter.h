#pragma once

#include <vector>
#include <array>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include "ColorMatching.h"
#include "utils.h"
#include "Tags.h"

class Filter {
public:
    Filter() :
        m_tags(TAGS)
    {}

    const std::unordered_set<std::string>& tags() const {
        return m_tags;
    }

    void setTags(std::unordered_set<std::string> tags) {
        m_tags = tags;
        
        for (const auto& x : TAGS) {
            if (!tags.count(x)) {
                m_disallowedTags.insert(x);
            }
        }
    }

    bool check(const TextureInfo& textureInfo) const {
        if (textureInfo.parsedTexture.avgRGB.isnan()) {
            return false;
        }
        if (textureInfo.parsedTexture.varRGB.normSquared() > variance * variance) {
            return false;
        }
        if(textureInfo.parsedTexture.numTransparent > numTransparent) {
            return false;
        }

        std::string filename = getFilename(textureInfo.textureData.path);

        for (const auto q: m_disallowedTags) {
            if (textureInfo.tags.count(q)) {
                return false;
            }
        }
        return true;
    }

    bool checkVariance(double v) const {
        return v <= variance * variance;
    }

    float variance = std::numeric_limits<float>::infinity();
    int numTransparent = 256;

private:
    std::unordered_set<std::string> m_tags{};
    std::unordered_set<std::string> m_disallowedTags{};
};