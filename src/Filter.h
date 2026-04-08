#pragma once

#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <array>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include "ColorMatching.h"
#include "utils.h"

// #define NUM_QUALIFIERS 94
// #define QUALIFIERS {"all", "amethyst", "back", "bamboo", "bar", "bars", "base", "beacon", "body", "bottom", "cactus", "cactus_top", "calibrated_side", "candle", "cocoa", "content", "crop", "cross", "cross_emissive", "dirt", "down", "east", "edge", "end", "end_rod", "eye", "fan", "fire", "flower", "flowerbed", "flowerpot", "front", "glass", "glow_lichen", "hook", "inner_top", "inside", "lantern", "leaf", "leg", "lever", "line", "lit", "lit_log", "lock", "log", "north", "obsidian", "overlay", "pane", "particle", "pattern", "pitcher_bottom", "pitcher_side", "pitcher_top", "pivot", "plant", "platform", "portal", "post", "propagule", "rail", "round", "sapling", "saw", "sculk_vein", "side", "sides", "slab", "south", "stage_1", "stage_2", "stage_3_bottom", "stage_3_top", "stage_4_bottom", "stage_4_top", "stand", "stem", "tendrils", "tentacles", "texture", "tip", "top", "torch", "tripwire", "unlit", "unsticky", "up", "upperstem", "vine", "wall", "west", "wood", "wool"}
// #define QUALIFIER_PATH "pre_parsing/texture_qualifiers.txt"

#define NUM_TAGS 9
#define TAGS {"full block", "top", "bottom", "directional", "cross", "entity", "animated", "TODO", "unused"}
#define TAGS_PATH "pre_parsing/texture_tags.csv"

class Filter {
public:
    Filter() :
        m_tags(TAGS)
    {
        ensureLoaded();
    }

    const std::unordered_set<std::string> tags() const {
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

        auto texture_tags = m_data[filename];
        for (const auto q: m_disallowedTags) {
            if (texture_tags.count(q)) {
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
    static void loadData() {
        parseTagsFile(TAGS_PATH);
    }

    static void ensureLoaded() {
        if (!m_initialized) {
            loadData();
            m_initialized = true;
        }
    }

    static void parseTagsFile(const std::string& path) {
        std::ifstream file(path);

        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + path);
        }

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;

            std::istringstream iss(line);

            std::string filename;
            std::string num_items_str;

            // Read filename
            if (!std::getline(iss, filename, ',')) {
                throw std::runtime_error("Malformed line: " + line);
            }

            // Read number of items
            if (!std::getline(iss, num_items_str, ',')) {
                throw std::runtime_error("Malformed line: " + line);
            }

            size_t num_items = std::stoul(num_items_str);
            
            std::unordered_set<std::string> items = {};
            if (num_items) {
                std::string tag;
    
                while (std::getline(iss, tag, ',')) {
                    if (!tag.empty()) {
                        items.insert(tag);
                    }
                }
            }
            m_data[filename] = std::move(items);
        }
    }

    inline static std::unordered_map<std::string, std::unordered_set<std::string>> m_data;
    inline static bool m_initialized = false;
    std::unordered_set<std::string> m_tags{};
    std::unordered_set<std::string> m_disallowedTags{};
};