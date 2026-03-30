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

#define NUM_QUALIFIERS 94
#define QUALIFIERS {"all", "amethyst", "back", "bamboo", "bar", "bars", "base", "beacon", "body", "bottom", "cactus", "cactus_top", "calibrated_side", "candle", "cocoa", "content", "crop", "cross", "cross_emissive", "dirt", "down", "east", "edge", "end", "end_rod", "eye", "fan", "fire", "flower", "flowerbed", "flowerpot", "front", "glass", "glow_lichen", "hook", "inner_top", "inside", "lantern", "leaf", "leg", "lever", "line", "lit", "lit_log", "lock", "log", "north", "obsidian", "overlay", "pane", "particle", "pattern", "pitcher_bottom", "pitcher_side", "pitcher_top", "pivot", "plant", "platform", "portal", "post", "propagule", "rail", "round", "sapling", "saw", "sculk_vein", "side", "sides", "slab", "south", "stage_1", "stage_2", "stage_3_bottom", "stage_3_top", "stage_4_bottom", "stage_4_top", "stand", "stem", "tendrils", "tentacles", "texture", "tip", "top", "torch", "tripwire", "unlit", "unsticky", "up", "upperstem", "vine", "wall", "west", "wood", "wool"}
#define QUALIFIER_PATH "pre_parsing/texture_qualifiers.txt"

class Filter {
public:
    Filter() :
        m_qualifiers(QUALIFIERS)
    {
        ensureLoaded();
    }

    const std::unordered_set<std::string> qualifiers() const {
        return m_qualifiers;
    }

    void setQualifiers(std::unordered_set<std::string> qualifiers) {
        m_qualifiers = qualifiers;
        
        for (const auto& x : QUALIFIERS) {
            if (!qualifiers.count(x)) {
                m_disallowedQualifiers.insert(x);
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

        auto texture_qualifiers = m_data[filename];
        for (const auto q: m_disallowedQualifiers) {
            if (texture_qualifiers.count(q)) {
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
        parseFile(QUALIFIER_PATH);
    }

    static void ensureLoaded() {
        if (!m_initialized) {
            loadData();
            m_initialized = true;
        }
    }

    static void parseFile(const std::string& path) {
        std::ifstream file(path);

        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + path);
        }

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;

            std::istringstream iss(line);

            std::string filename;
            size_t num_items;
            std::string items_str;

            // Read filename and number
            if (!(iss >> filename >> num_items)) {
                throw std::runtime_error("Skipping malformed line: " + line);
            }
            
            if (num_items) {
                // Read the rest of the line (items)
                if (!(iss >> items_str)) {
                    throw std::runtime_error("Empty line: " + line);
                }
            }

            std::unordered_set<std::string> items = {};
            std::stringstream ss(items_str);
            std::string item;

            while (std::getline(ss, item, ',')) {
                if (!item.empty()) {
                    items.insert(item);
                }
            }
            
            m_data[filename] = std::move(items);
        }
    }

    inline static std::unordered_map<std::string, std::unordered_set<std::string>> m_data;
    inline static bool m_initialized = false;
    std::unordered_set<std::string> m_qualifiers{};
    std::unordered_set<std::string> m_disallowedQualifiers{};
};