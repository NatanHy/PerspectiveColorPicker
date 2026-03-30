#include <iostream>
#include <array>
#include <string>
#include <numeric>
#include <chrono>

#include "ColorMatching.h"

void printSeq(TextureSequence& seq) {
    int i = 1;
    for (const auto& s : seq.seq()) {
        std::cout << getFilename(s.textureData.path);
        if (i != seq.seq().size()) {
            std::cout << " + ";
        }
        ++i;
    }
    std::cout << " = " << seq.blendRGB() << std::endl;
}

int main() {
    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::duration;
    using std::chrono::milliseconds;

    std::vector<std::string> dirs = {
        "minecraft\\textures\\block"
    };

    Filter filter;
    TextureParser parser(dirs, filter);
    parser.loadTextureData();
    parser.parseTextures();

    std::vector<double> errors;

    const int numLayers = 4;
    const int numTests = 1000;

    Optimizer optimizer(&parser);

    auto t1 = high_resolution_clock::now();

    #pragma omp parallel
    {
        auto tOptimizer = optimizer;
        #pragma omp for
        for (int i = 0; i < numTests; ++i) {
            TextureSequence randomSeq = tOptimizer.rng().randomSequence(numLayers);
            auto match = tOptimizer.getBestMatch(randomSeq.blendRGB(), numLayers);
            auto err = match.error(randomSeq.blendRGB());
            errors.push_back(err);
        }
        
    }
    
    auto t2 = high_resolution_clock::now();

    /* Getting number of milliseconds as an integer. */
    auto ms_int = duration_cast<milliseconds>(t2 - t1);
    
    std::cout << "Average error: " <<  std::reduce(errors.begin(), errors.end()) / errors.size() << std::endl;
    std::cout << "Average Execution time: " << (double)ms_int.count() / (double)numTests << "ms" << std::endl << std::endl;
}