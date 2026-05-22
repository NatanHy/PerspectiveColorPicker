#include <algorithm>
#include <array>
#include <cmath>
#include <omp.h>
#include <fstream>

#include "raylib.h"
#include "rlImGui.h"
#include "imgui.h"
#include "rlgl.h"

#include "ColorMatching.h"
#include "Renderer.h"
#include "utils.h"
#include "filter.h"
#include "ClipboardWatcher.h"

constexpr std::array<std::string_view, NUM_TAGS> tags = TAGS;

enum class View {
    ColorPicker,
    Filter,
    Image
};

class GUIState {
    public:
    GUIState(const TextureParser parser) :
        m_parser(parser),
        m_optimizer(Optimizer(&m_parser))
    {
        checkedTags.fill(true);
    }

    void update() {
        updateParser();
        imgChanged = true;

        if (currentView == View::Image && imgPath != "") {
            std::cout << "Loading " << imgPath << std::endl;
            m_imgParser.loadImage(imgPath);
            
            double ratio = (double)m_imgParser.height() / (double)m_imgParser.width();
            m_imgWidth = imgSize;
            m_imgHeight = (int)std::ceil(((double)m_imgWidth * ratio));

            m_avgGrid = m_imgParser.averageRGBS(m_imgWidth, m_imgHeight);
            m_seqGrid = Grid<TextureSequence>(m_imgWidth, m_imgHeight);

            using std::chrono::high_resolution_clock;
            using std::chrono::duration_cast;
            using std::chrono::milliseconds;

            auto t1 = high_resolution_clock::now();

            #pragma omp parallel for collapse(2) firstprivate(m_optimizer)
            for (int x = 0; x < m_imgWidth; x++) {
                for (int y = 0 ; y < m_imgHeight; y++) {
                    auto match = m_optimizer.getBestMatch(m_avgGrid.at(x, y), layers, imgLocalSearch);
                    m_seqGrid.at(x, y) = std::move(match);
                }
            }

            auto t2 = high_resolution_clock::now();
            auto ms_int = duration_cast<milliseconds>(t2 - t1);
            
            std::cout << "Execution time: " << (double)ms_int.count() << "ms" << std::endl << std::endl;
                    
        } else {
            Vec3 rgb = {255.0*color[0], 255.0*color[1], 255.0*color[2]};
            m_best = m_optimizer.getBestMatch(rgb, layers);
            m_error = m_best.error(color);
            m_seqColor = m_best.blendRGB();
        }
    }

    Vec3 sequenceColor() {
        return m_seqColor;
    }

    double error() {
        return m_error / 16581.375f; // Divide by 255^3, return as percentage
    }

    const TextureParser& parser() const {
        return m_parser;
    }

    const int imgWidth() const {
        return m_imgWidth;
    }

    const int imgHeight() const {
        return m_imgHeight;
    }

    const TextureSequence& best() const {
        return m_best;
    }

    TextureSequence& best() {
        return m_best;
    }

    const TextureSequence seqAt(size_t x, size_t y) const {
        return m_seqGrid.at(x, y);
    }

    void pollClipboard() {
        std::ifstream file("clipboard/clipboard.meta");
        if (!file.is_open()) {
            return; // no meta yet
        }

        std::string newHash;
        std::getline(file, newHash);

        if (newHash.empty()) {
            return;
        }

        // Compare with stored hash
        if (newHash != mImgHash) {
            mImgHash = newHash;

            // Update path so your system reloads the image
            imgPath = "clipboard/clipboard.png";
            update();
        }
    }
    View currentView;

    int layers = 2;
    float variance = std::numeric_limits<float>::infinity();
    int numTransparent = 256;
    float color[3] = {1.0f, 0.0f, 0.0f};
    std::string imgPath;
    int imgSize = 50;
    bool imgLocalSearch = false;

    RenderTexture2D imgCache;
    bool imgChanged = true;

    std::array<bool, NUM_TAGS> checkedTags;
    
    private:
    void updateParser() {
        bool changed = false;
        if (variance != m_parser.filter.variance) {
            m_parser.filter.variance = variance;
            changed = true;
        }
        if (numTransparent != m_parser.filter.numTransparent) {
            m_parser.filter.numTransparent = numTransparent;
            changed = true;
        }

        auto newTags = filterSelected<NUM_TAGS>(checkedTags, tags);

        if (newTags != m_parser.filter.tags()) {
            m_parser.filter.setTags(newTags);
            changed = true;
        }

        if (changed) {
            m_parser.parseTextures();
        }
    }
    
    TextureParser m_parser;
    ImageParser m_imgParser;
    int m_imgWidth;
    int m_imgHeight;
    std::string mImgHash;

    Grid<Vec3> m_avgGrid;
    Grid<TextureSequence> m_seqGrid;

    Optimizer m_optimizer;

    TextureSequence m_best;
    Vec3 m_seqColor;
    double m_error = 0;
};

void drawNavbar(GUIState& state) {
    float height = 40.0f;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, height));

    ImGui::Begin("Navbar", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings);

    if (ImGui::Button("Color Picker")) {
        state.currentView = View::ColorPicker;
        state.update();
    }     
    ImGui::SameLine();
    if (ImGui::Button("Image")) {
        state.currentView = View::Image;
        state.update();
    }

    ImGui::End();
}

void drawColorPicker(Renderer& renderer, GUIState& state) {
    renderer.drawLayers(50, 50, 10, state.best(), 20, true);

    for (float x = 50.0; x < 300.0; x += 16) {
        for (float y = 350.0; y < 550.0; y += 16) {             
            renderer.drawLayers(x, y, 1, state.best(), 0, false);
        }
    }

    renderer.drawRectangle(350, 350, 50, 50, state.sequenceColor());
    renderer.drawRectangle(350, 410, 50, 50, 255.0 * Vec3(state.color));

    DrawText("Average", 405, 350, 20, WHITE);
    DrawText("Target", 405, 410, 20, WHITE);
    auto err_str = "Error: " + std::to_string(state.error());
    DrawText(err_str.c_str(), 350, 460, 20, WHITE);
    auto var2 = "Variance: " + std::to_string(std::sqrt(state.best().variance().normSquared()));
    DrawText(var2.c_str(), 350, 510, 20, WHITE);
    

    ImGui::Begin("Color Picker");

    ImGui::ColorPicker3("Color", &state.color[0]);

    ImGui::SliderInt("Layers", &state.layers, 1, 10);
    ImGui::SliderInt("Allow transparent", &state.numTransparent, 0, 256);
    ImGui::SliderFloat("Variance", &state.variance, 0.0f, 10000.0f);
    
    if (ImGui::Button("Apply Color"))
    {
        state.update();
    }
    ImGui::End();
    
    ImGui::Begin("Tags");

        if (ImGui::Button("All On"))
        {
            state.checkedTags.fill(true);
        }

        if (ImGui::Button("All Off"))
        {
            state.checkedTags.fill(false);
        }

        for (int i = 0; i < NUM_TAGS; ++i) {
            ImGui::Checkbox(tags[i].data(), &state.checkedTags[i]);
        }
    ImGui::End();
}

void drawFilter(Renderer& renderer, GUIState& state) {
    
}

void createImgCache(Renderer& renderer, GUIState& state)
{
    int width = renderer.width();
    int height = renderer.height();

    state.imgCache = LoadRenderTexture(width, height);
    SetTextureFilter(state.imgCache.texture, TEXTURE_FILTER_POINT);
    rlSetBlendFactorsSeparate(RL_SRC_ALPHA, RL_ONE_MINUS_SRC_ALPHA, RL_ONE, RL_ONE, RL_FUNC_ADD, RL_BLEND_ALPHA);

    BeginTextureMode(state.imgCache);
    BeginBlendMode(RL_BLEND_CUSTOM_SEPARATE);
    ClearBackground(WHITE);

    for (size_t x = 0; x < state.imgWidth(); x++) {
        for (size_t y = 0; y < state.imgHeight(); y++) {
            auto seq = state.seqAt(x, y);
            float px = x * 16;
            float py = y * 16;

            renderer.drawLayers(
                px,
                50 + py,
                1,
                seq,
                0,
                false
            );
        }
    }

    state.imgChanged = false;

    EndBlendMode();
    EndTextureMode();
}

void drawImage(Renderer& renderer, GUIState& state) {
    if (IsFileDropped()) {
        FilePathList files = LoadDroppedFiles();
        
        state.imgPath = files.paths[0];
        UnloadDroppedFiles(files);
        state.update();
    }

    if (state.imgPath == "") {
        return;
    }

    double scale = renderer.width() / (state.imgWidth() * 16.0);
    for (size_t x = 0; x < state.imgWidth(); x++) {
        for (size_t y = 0; y < state.imgHeight(); y++) {
            auto seq = state.seqAt(x, y); 
            renderer.drawLayers(x * 16 * scale, 50 + y * 16 * scale, scale, seq, 0, false); 
        } 
    }
    
    // --- Mouse picking ---
    Vector2 mouse = GetMousePosition();
    
    int gridX = (int)(mouse.x / (16 * scale));
    int gridY = (int)((mouse.y - 50) / (16 * scale));

    // Check bounds
    if (gridX >= 0 && gridX < (int)state.imgWidth() &&
        gridY >= 0 && gridY < (int)state.imgHeight())
    {
        auto hoveredSeq = state.seqAt(gridX, gridY);

        auto width = 500;
        auto height = 290;
        auto recX = 0;
        auto recY = renderer.height() - height - 20;
        if (mouse.x < width && mouse.y > recY) {
            recX = renderer.width() - width - 20;
        }

        renderer.drawRectangle(recX, recY, width, height, {0, 0, 0}, 100);
        renderer.drawLayers(recX + 10, recY + 10, 10.0, hoveredSeq, 20, true);
    }

    ImGui::Begin("Image Size");

    ImGui::SliderInt("Size", &state.imgSize, 5, 300);
    ImGui::Checkbox("Local Search", &state.imgLocalSearch);
    if (ImGui::Button("Apply"))
    {
        state.update();
    }

    ImGui::End();
}

void drawCurrentView(Renderer& renderer, GUIState& state) {
    switch (state.currentView) {
        case View::ColorPicker:
            drawColorPicker(renderer, state);
            break;

        case View::Filter:
            drawFilter(renderer, state);
            break;

        case View::Image:
            drawImage(renderer, state);
            break;
    }
}

int main() {
    SetTraceLogLevel(LOG_WARNING);  // Only warnings + errors

    std::vector<std::string> dirs = {
        "minecraft\\textures\\block"
    };

    Filter filter;
    TextureParser parser(dirs, std::move(filter));
    parser.loadTextureData();
    parser.parseTextures();

    GUIState state(std::move(parser));

    Renderer renderer(1600, 1000, "Texture Viewer");
    renderer.textureBrightness = 0.8;

    rlImGuiSetup(true); // enable docking

    state.currentView = View::ColorPicker;

    // Start clipboard listener
    ClipboardWatcher clipboardWatcher;
    clipboardWatcher.start();
    
    while (!renderer.ShouldClose())
    {
        renderer.Begin();
        rlImGuiBegin();

        drawNavbar(state);
        drawCurrentView(renderer, state);

        if (renderer.sizeChanged()) {
            state.update();
        }

        state.pollClipboard();

        rlImGuiEnd();
        renderer.End();
    }

    // Stop python listener
    clipboardWatcher.stop();

    rlImGuiShutdown();
    return 0;
}