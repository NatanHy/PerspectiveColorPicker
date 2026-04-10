#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include "raylib.h"
#include "Vectors.h"

#include "ColorMatching.h"

class Renderer
{
public:
    Renderer(int width, int height, const std::string& title);
    ~Renderer();

    bool ShouldClose() const;
    void Begin();
    void End();

    void deleteTexture(std::string texPath);
    void resetTextures();

    void drawTexture(float x, float y, float scale, const TextureInfo& tex);
    void drawLayers(float x, float y, float scale, const TextureSequence& textures, float offset, bool text);
    void drawRectangle(float x, float y, float width, float height, Vec3 color, unsigned char alpha=255);
    void drawText(std::string text, float x, float y, int font_size);
    
    const int width() const;
    const int height() const;
    const bool sizeChanged();

    std::string textureName(const TextureInfo& tex);
    
    int numTextures=0;
    double textureBrightness = 1.0;
    
    private:
    void loadTexture(const TextureInfo& tex);

    int m_prevWidth=0;
    int m_prevHeight=0;

    size_t nextID=0;
    std::unordered_map<std::string, Texture2D> loadedTextures;
};