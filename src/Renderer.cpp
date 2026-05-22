#include <iostream>

#include "Renderer.h"
#include "utils.h"

Renderer::Renderer(int width, int height, const std::string& title)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(width, height, title.c_str());
    SetTargetFPS(60);
}

const int Renderer::width() const {
    return GetScreenWidth();
}

const int Renderer::height() const {
    return GetScreenHeight();
}

const bool Renderer::sizeChanged() {
    bool changed = false;
    if (m_prevWidth != width()) {
        m_prevWidth = width();
        changed = true;
    }
    if (m_prevHeight != height()) {
        m_prevHeight = height();
        changed = true;
    }
    return changed;
}

Renderer::~Renderer()
{   
    for (const auto& [path, tex2d] : loadedTextures) {
        deleteTexture(path);
    }
    CloseWindow();
}

bool Renderer::ShouldClose() const
{
    return WindowShouldClose();
}

void Renderer::Begin()
{
    BeginDrawing();
    ClearBackground(DARKGRAY);
}

void Renderer::End()
{
    EndDrawing();
}

void Renderer::deleteTexture(std::string texPath) {
    auto tex2d = loadedTextures[texPath];
    UnloadTexture(tex2d);
    loadedTextures.erase(texPath);
}

void Renderer::resetTextures() {
    loadedTextures.clear();
}

void Renderer::loadTexture(const TextureInfo& tex)
{ 
    Image image = LoadImage(tex.textureData.path.c_str());
    Texture2D tex2d = LoadTextureFromImage(image);
    SetTextureFilter(tex2d, TEXTURE_FILTER_POINT);
    loadedTextures.insert({tex.textureData.path, tex2d});
    UnloadImage(image);
}

void Renderer::drawTexture(float x, float y, float scale, const TextureInfo& tex)
{
    if (loadedTextures.find(tex.textureData.path) == loadedTextures.end()) {
        loadTexture(tex);
    }

    auto tex2d = loadedTextures[tex.textureData.path];

    unsigned char t = 255;
    if (tex.shouldShade()) {
        t = (unsigned char)(255 * textureBrightness);
    } 

    Color tint = {t, t, t, 255};

    DrawTextureEx(tex2d, (Vector2){ x, y }, 0.0f, scale, tint);
}

void Renderer::drawLayers(float x, float y, float scale, const TextureSequence& textures, float offset, bool text) {
    const auto seq = textures.seq();
    for (int i = 0; i < seq.size(); ++i) {
        float xi = x + offset * i;
        float yi = y + offset * i;
        auto tex = seq[i];
        drawTexture(xi, yi, scale, tex);

        if (text) {
            DrawText(textureName(tex).c_str(), xi + 16 * scale, yi - scale , 20, WHITE);
        }
    }
}

void Renderer::drawRectangle(float x, float y, float width, float height, Vec3 color, unsigned char alpha)
{
    Color rectColor = {
        (unsigned char)(color.x),
        (unsigned char)(color.y),
        (unsigned char)(color.z),
        alpha
    };

    ::DrawRectangle(
        static_cast<int>(x),
        static_cast<int>(y),
        static_cast<int>(width),
        static_cast<int>(height),
        rectColor
    );
}

void Renderer::drawText(std::string text, float x, float y, int font_size) {
    DrawText(text.c_str(), x, y, font_size, WHITE);
}

std::string Renderer::textureName(const TextureInfo& tex) {
    return formatFilename(getFilename(tex.textureData.path));
}