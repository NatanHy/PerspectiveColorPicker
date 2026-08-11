#pragma once

#include <vector>
#include <string>
#include <filesystem>
#include <iostream>
#include <unordered_set>
#include <unordered_map>

#include <chrono>
#include <thread>

#include "stb_image.h"
#include "Vectors.h"
#include "Tags.h"

namespace fs = std::filesystem;

enum FACING {
    NORTH_SOUTH,
    EAST_WEST
};

struct RGBA {
    unsigned char r, g, b, a;
};

inline Vec3 fromRGBA(RGBA rgba) {
    return {(double)rgba.r, (double)rgba.g, (double)rgba.b};
}

struct ParsedData {
    Vec3 avgRGB;
    Vec3 varRGB;
    double alphaRatio;
    int numTransparent;
};

class TextureData {
public:
    std::string path;
    int width = 0;
    int height = 0;
    int channels = 0; // original channels from stb
    std::vector<unsigned char> data; // MUST be RGBA (4 bytes per pixel)

    // ===== Pixel access =====
    RGBA* pixels() {
        validate();
        return reinterpret_cast<RGBA*>(data.data());
    }

    const RGBA* pixels() const {
        validate();
        return reinterpret_cast<const RGBA*>(data.data());
    }

    size_t pixelCount() const {
        return static_cast<size_t>(width) * height;
    }

    // ===== Iterators =====
    RGBA* begin() {
        return pixels();
    }

    RGBA* end() {
        return pixels() + pixelCount();
    }

    const RGBA* begin() const {
        return pixels();
    }

    const RGBA* end() const {
        return pixels() + pixelCount();
    }

    const RGBA* cbegin() const {
        return pixels();
    }

    const RGBA* cend() const {
        return pixels() + pixelCount();
    }

    // ===== 2D access (optional but useful) =====
    RGBA& at(int x, int y) {
        validate();
        return pixels()[y * width + x];
    }

    const RGBA& at(int x, int y) const {
        validate();
        return pixels()[y * width + x];
    }

private:
    void validate() const {
        if (data.empty()) {
            throw std::runtime_error("Texture data is empty");
        }

        if (data.size() % 4 != 0) {
            throw std::runtime_error("Texture data is not RGBA-aligned");
        }
    }
};

class TextureInfo {
    public:
    TextureInfo()=default;
    TextureInfo(TextureData textureData, std::unordered_set<std::string> tags) :
        textureData(textureData),
        tags(tags)
    {}

    bool operator==(const TextureInfo& other) const {
        return textureData.path == other.textureData.path;
    }
    
    bool shouldShade() const {
        if (tags.find("cross") != tags.end() || tags.find("TODO") != tags.end()) {
            return false;
        }
        return true;
    }

    TextureData textureData;
    ParsedData parsedTexture;
    std::unordered_set<std::string> tags;
};

template <typename T>
class Grid {
    public:
    Grid() : 
        m_data()
    {}
    Grid(int width, int height) {
        for (int i = 0; i < height; ++i) {
            std::vector<T> v;
            for (int j = 0; j < width; ++j) {
                v.push_back(T());
            }
            m_data.push_back(std::move(v));
        }
    }

    const T at(size_t x, size_t y) const {
        return m_data[y][x];
    }

    T& at(size_t x, size_t y) {
        return m_data[y][x];
    }

    private:
    std::vector<std::vector<T>> m_data;
};

#include "Filter.h"

class TextureParser {
public:
    TextureParser(
        const std::vector<std::string>& directories, 
        Filter filter,
        FACING facing=NORTH_SOUTH
    )
        : m_directories(directories), filter(filter), facing(facing)
    {
        parseTagsFile(TAGS_PATH);
    }

    TextureData loadTextureFromFile(std::string path) {
        TextureData tex;
        tex.path = path;

        int forceChannels = 4;

        unsigned char* data = stbi_load(
            tex.path.c_str(),
            &tex.width,
            &tex.height,
            &tex.channels,
            forceChannels // Force RGBA
        );

        if (!data) {
            throw std::runtime_error("Failed to load data: " + tex.path);
        }

        size_t size = tex.width * tex.height * forceChannels;

        tex.data.assign(data, data + size);

        stbi_image_free(data);

        return tex;
    }

    void loadTextureData() {
        for (const auto& dir : m_directories) {
            if (!fs::exists(dir) || !fs::is_directory(dir)) {
                std::cerr << "Invalid directory: " << dir << std::endl;
                continue;
            }

            for (const auto& entry : fs::recursive_directory_iterator(dir)) {
                if (entry.is_regular_file() && entry.path().extension() == ".png") {
                    auto path = entry.path().string();
                    auto tex = loadTextureFromFile(path);
                    if (tex.width == 16 && tex.height == 16) {
                        TextureInfo info(tex, tags(getFilename(path)));
                        m_textures.push_back(info);
                    }
                }
            }
        }
    }

    void parseTextures() {
        m_opaqueTextures.clear();
        m_translucentTextures.clear();

        for (auto& tex : m_textures) {
            double shade = 1.0;
            if (tex.shouldShade()) {
                switch (facing) {
                    case (NORTH_SOUTH):
                        shade = 0.8;
                        break;
                    case (EAST_WEST):
                        shade = 0.6;
                        break;
                }
            }

            ParsedData parsed;
            int sumR = 0; 
            int sumG = 0; 
            int sumB = 0;
            int sumA = 0;

            int sumR2 = 0;
            int sumG2 = 0;
            int sumB2 = 0;

            int numTransparent = 0;
            
            for (const auto& pixel : tex.textureData) {
                // Some textures have alpha = 1, functionally transparent
                if (pixel.a <= 1) {
                    numTransparent += 1;
                } else {
                    sumR += pixel.r;
                    sumG += pixel.g;
                    sumB += pixel.b;
                    sumA += pixel.a;
    
                    sumR2 += pixel.r * pixel.r;
                    sumG2 += pixel.g * pixel.g;
                    sumB2 += pixel.b * pixel.b;
                }
            }

            double count = (double)(tex.textureData.pixelCount() - numTransparent);

            if (count == 0) {
                parsed.avgRGB = Vec3({NAN, NAN, NAN});
                parsed.varRGB = {NAN, NAN, NAN};
                parsed.alphaRatio = 1.0;
                parsed.numTransparent = numTransparent;

                tex.parsedTexture = parsed;
            } else {
                double avgR = (double)sumR / count;
                double avgG = (double)sumG / count;
                double avgB = (double)sumB / count;
    
                double varR = ((double)sumR2 / count) - (avgR * avgR);
                double varG = ((double)sumG2 / count) - (avgG * avgG);
                double varB = ((double)sumB2 / count) - (avgB * avgB);
                
                parsed.avgRGB = shade * Vec3({avgR, avgG, avgB});
                parsed.varRGB = {varR, varG, varB};
                parsed.alphaRatio = (double)sumA / (double)(255.0 * tex.textureData.pixelCount());
                parsed.numTransparent = numTransparent;
                
                tex.parsedTexture = parsed;
            }

            if (filter.check(tex)) {
                if (numTransparent == 0 && 1.0 - parsed.alphaRatio < 1e-6) {
                    m_opaqueTextures.push_back(tex);
                } else {
                    m_translucentTextures.push_back(tex);
                }
            }
        }
    }

    void parseTagsFile(const std::string& path) {
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
            m_tags[filename] = std::move(items);
        }
    }

    const std::vector<TextureInfo>& allTextures() const {
        return m_textures;
    }
    
    const std::vector<TextureInfo>& opaqueTextures() const {
        return m_opaqueTextures;
    }
    
    const std::vector<TextureInfo>& translucentTextures() const {
        return m_translucentTextures;
    }

    const std::unordered_set<std::string>& tags(std::string filename) const {
        return m_tags[filename];
    }
    
    Filter filter;
    FACING facing;
    private:

    inline static std::unordered_map<std::string, std::unordered_set<std::string>> m_tags;
    std::vector<std::string> m_directories;
    std::vector<TextureInfo> m_textures;
    std::vector<TextureInfo> m_opaqueTextures;
    std::vector<TextureInfo> m_translucentTextures;
};

class ImageParser {
    public:
    void loadImage(std::string path) {
        m_image.path = path;
        int forceChannels = 4;
        int tries = 3;


        while (tries > 0) {
            unsigned char* data = stbi_load(
                m_image.path.c_str(),
                &m_image.width,
                &m_image.height,
                &m_image.channels,
                forceChannels // Force RGBA
            );
    
            if (!data) {
                std::cerr << "WARNING: Failed to load image data from " + m_image.path + " retrying...\n";
                tries--;

                std::this_thread::sleep_for(std::chrono::seconds(1));
            } else {
                size_t size = m_image.width * m_image.height * forceChannels;
        
                m_image.data.assign(data, data + size);
        
                stbi_image_free(data);
                return;
            }
        }
        throw std::runtime_error("Failed to load image data: " + m_image.path);
    }

    Grid<Vec3> averageRGBS(int width, int height) {
        Grid<Vec3> grid(width, height);
        Grid<double> counts(width, height);

        for (size_t x = 0; x < m_image.width; ++x) {
            for (size_t y = 0; y < m_image.height; ++y) {

                size_t gridX = (x * width) / m_image.width;
                size_t gridY = (y * height) / m_image.height;

                grid.at(gridX, gridY) =
                    grid.at(gridX, gridY) + fromRGBA(m_image.at(x, y));

                counts.at(gridX, gridY) += 1;
            }
        }

        for (size_t x = 0; x < width; ++x) {
            for (size_t y = 0; y < height; ++y) {
                if (counts.at(x, y) > 0) {
                    grid.at(x, y) =
                        grid.at(x, y) * (1.0 / counts.at(x, y));
                }
            }
        }

        return grid;
    }

    const std::string path() const {
        return m_image.path;
    }

    const int width() const {
        return m_image.width;
    }

    const int height() const {
        return m_image.height;
    }

    private:
    TextureData m_image;
};