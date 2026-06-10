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

struct GridInfo {
    // Size of resulting build size
    int blockWidth, blockHeight;
    int beginX = 0, beginY = 0, endX = 0, endY = 0;

    // Grids for avg RGBs and resulting sequencesx
    Grid<Vec3> avgGrid;
    Grid<TextureSequence> seqGrid;
};

struct Zoom {
    double mouseX = 0.0, mouseY = 0.0;
    int level = 0;
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

        if (currentView == View::Image && imgPath != "") {
            updateGrid();                    
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

    const int gridWidth() const {
        return m_gridInfo.endX - m_gridInfo.beginX;
    }

    const int gridHeight() const {
        return m_gridInfo.endY - m_gridInfo.beginY;
    }

    const int gridMaxWidth() const {
        return m_gridInfo.blockWidth;
    }

    const int gridMaxHeight() const {
        return m_gridInfo.blockHeight;
    }

    const TextureSequence& best() const {
        return m_best;
    }

    TextureSequence& best() {
        return m_best;
    }

    const TextureSequence seqAt(size_t x, size_t y) const {
        return m_gridInfo.seqGrid.at(x + m_gridInfo.beginX, y + m_gridInfo.beginY);
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
    // Window
    View currentView;

    // Settings
    int layers = 2;
    float variance = std::numeric_limits<float>::infinity();
    int numTransparent = 256;
    float color[3] = {1.0f, 0.0f, 0.0f};
    int imgSize = 50;
    bool imgLocalSearch = false;
    Zoom zoom;
    std::array<bool, NUM_TAGS> checkedTags;

    // Path to user-chosen image
    std::string imgPath;
    
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

    void applyZoom()
    {
        const double zoomFactor = std::pow(0.5, zoom.level);

        const double visibleW = m_gridInfo.blockWidth  * zoomFactor;
        const double visibleH = m_gridInfo.blockHeight * zoomFactor;

        // Convert normalized mouse pos from viewport-space to absolute grid-space
        const double currentW = m_gridInfo.endX - m_gridInfo.beginX;
        const double currentH = m_gridInfo.endY - m_gridInfo.beginY;

        const double absMouseX = m_gridInfo.beginX + zoom.mouseX * currentW;
        const double absMouseY = m_gridInfo.beginY + zoom.mouseY * currentH;

        // Center the new window around that absolute point
        const double rawBeginX = absMouseX - visibleW * 0.5;
        const double rawBeginY = absMouseY - visibleH * 0.5;

        m_gridInfo.beginX = std::clamp(rawBeginX, 0.0, (double)m_gridInfo.blockWidth);
        m_gridInfo.beginY = std::clamp(rawBeginY, 0.0, (double)m_gridInfo.blockHeight);
        m_gridInfo.endX   =  std::clamp(m_gridInfo.beginX + visibleW, 0.0, (double)m_gridInfo.blockWidth);
        m_gridInfo.endY   =  std::clamp(m_gridInfo.beginY + visibleH, 0.0, (double)m_gridInfo.blockHeight);
    }

    void updateGrid() {
        std::cout << "Loading " << imgPath << std::endl;
            
        m_imgParser.loadImage(imgPath);
        
        // Update image size
        if (m_imgParser.width() >= m_imgParser.height()) {
            double ratio = (double)m_imgParser.height() / (double)m_imgParser.width();
            m_gridInfo.blockWidth = imgSize;
            m_gridInfo.blockHeight = (int)std::round(((double)imgSize * ratio));
        } else {
            double ratio = (double)m_imgParser.width() / (double)m_imgParser.height();
            m_gridInfo.blockHeight = imgSize;
            m_gridInfo.blockWidth = (int)std::round(((double)imgSize * ratio));
        }
        
        applyZoom();

        using std::chrono::high_resolution_clock;
        using std::chrono::duration_cast;
        using std::chrono::milliseconds;
        
        auto t1 = high_resolution_clock::now();
        
        // Update grid sequences
        m_gridInfo.avgGrid = m_imgParser.averageRGBS(m_gridInfo.blockWidth,  m_gridInfo.blockHeight);
        m_gridInfo.seqGrid = Grid<TextureSequence>(m_gridInfo.blockWidth,  m_gridInfo.blockHeight);

        #pragma omp parallel for collapse(2) firstprivate(m_optimizer)
        for (int x = m_gridInfo.beginX; x < m_gridInfo.endX; x++) {
            for (int y = m_gridInfo.beginY; y < m_gridInfo.endY; y++) {
                auto match = m_optimizer.getBestMatch(m_gridInfo.avgGrid.at(x, y), layers, imgLocalSearch);
                m_gridInfo.seqGrid.at(x, y) = std::move(match);
            }
        }
        
        auto t2 = high_resolution_clock::now();
        auto ms_int = duration_cast<milliseconds>(t2 - t1);
        std::cout << "Execution time: " << (double)ms_int.count() << "ms" << std::endl << std::endl;
    }
    
    // Objects for parsing and computing results
    TextureParser m_parser;
    ImageParser m_imgParser;
    Optimizer m_optimizer;

    // Sequence info for color picker
    TextureSequence m_best;
    Vec3 m_seqColor;
    double m_error = 0;

    // Image hash for clipboard
    std::string mImgHash;

    GridInfo m_gridInfo;
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

    double scale = std::min(
        renderer.width() / (state.gridWidth() * 16.0), 
        renderer.height() / (state.gridHeight() * 16.0)
    );
    
    for (int x = 0; x < state.gridWidth(); x++) {
        for (int y = 0; y < state.gridHeight(); y++) {
            auto seq = state.seqAt(x, y); 
            renderer.drawLayers(x * 16 * scale, 50 + y * 16 * scale, scale, seq, 0, false); 
        } 
    }
    
    // --- Mouse picking ---
    Vector2 mouse = GetMousePosition();
    
    double relX = mouse.x / (double)(16 * scale * state.gridWidth());
    double relY = (mouse.y - 50) / (double)(16 * scale * state.gridHeight());

    if (relX >= 0 && relX <= 1.0 && relY >= 0.0 && relY <= 1.0) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            state.zoom = {relX, relY, state.zoom.level + 1};
            state.update();
        } else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            state.zoom.level -= 1;
            if (state.zoom.level < 0) {
                state.zoom.level = 0;
            } else {
                state.update();
            }
        }
    }

    int gridX = (int)(relX * state.gridWidth());
    int gridY = (int)(relY * state.gridHeight());

    // Check bounds
    if (gridX >= 0 && gridX < (int)state.gridWidth() &&
        gridY >= 0 && gridY < (int)state.gridHeight())
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