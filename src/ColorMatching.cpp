#include <cmath>
#include <limits>
#include <cstdlib>

#include "ColorMatching.h"
#include "Filter.h"
#include "ColorUtils.h"


bool accept(TextureRandomizer& rng, double current_cost, double candidate_cost, double T) {
    double delta = candidate_cost - current_cost;

    if (delta < 0) return true;

    double probability = exp(-delta / T);
    double r = rng.rand01();

    return r < probability;
}

double computeError(const Vec3& rgb, const Vec3& targetRGB) {
    Vec3 lab1 = rgbToLab(rgb);
    Vec3 lab2 = rgbToLab(targetRGB);

    return deltaE94(lab1, lab2);
}

TextureInfo closest(const std::vector<TextureInfo>& textures, const Vec3 targetRGB) {
    const TextureInfo* best = &textures[0];
    double bestError = computeError(best->parsedTexture.avgRGB, targetRGB);

    for (const auto& tex: textures) {
        double error = computeError(tex.parsedTexture.avgRGB, targetRGB);
        if (error < bestError) {
            bestError = error;
            best = &tex;
        }
    }

    return *best;
}

double TextureSequence::error(const Vec3& targetRGB) const {
    return computeError(blendRGB(), targetRGB);
}

TextureSequence Optimizer::getBestMatch(const Vec3& targetRGB, int maxLayers, bool localSearch) {
    auto bestGreedy = getBestMatchGreedy(targetRGB, maxLayers);
    
    if (localSearch) {
        return getBestMatchLocalSearch(bestGreedy, targetRGB, maxLayers);
    } 

    return bestGreedy;
}

TextureSequence Optimizer::getBestMatchGreedy(const Vec3& targetRGB, int maxLayers) const {
    TextureSequence seq;

    auto closestOpaqe = closest(m_parser->opaqueTextures(), targetRGB);
    seq.setOpaque(closestOpaqe);

    double error = seq.error(targetRGB);
    int backIndex = 0;
    
    while (seq.length() < maxLayers) {
        seq.addTranslucent({});
        bool foundCandidate = false;
        TextureInfo candidateTexture;

        for (const auto& tex : m_parser->translucentTextures()) {
            seq.replaceTranslucent(backIndex, tex);
    
            auto newError = seq.error(targetRGB);
    
            if (
                newError < error && 
                m_parser->filter.checkVariance(seq.variance().normSquared())
            ) {
                foundCandidate = true;
                candidateTexture = tex;
                error = newError;
            } 
        }
        if (!foundCandidate) {
            seq.popTranslucent();
            break;
        } else {
            seq.replaceTranslucent(backIndex, candidateTexture);
            backIndex++;
        }
    }

    return seq;
}

TextureSequence Optimizer::getBestMatchLocalSearch(TextureSequence& startingState, const Vec3& targetRGB, int maxLayers) {
    if (m_parser->opaqueTextures().empty() || m_parser->translucentTextures().empty()) {
        std::cerr << "WARNING: No textures match filter." << std::endl;
        return {};
    }

    // Simulated annealing parameters
    const int neighborhoodSize = 10;
    
    const double T0 = 1000.0;
    const double Tend = 1e-4;
    const double alpha = pow(Tend / T0, 1.0 / maxIters);
    
    double T = T0;
    
    // Initial state
    TextureSequence seq = startingState;
    double bestErr = seq.error(targetRGB);

    double bestGlobalErr = bestErr;
    TextureSequence bestGlobal = seq;

    // --- Local search ---
    for (int iter = 0; iter < maxIters; ++iter) {
        for (int n = 0; n < neighborhoodSize; ++n) {
            bool success = seq.mutate(m_rng, maxLayers);

            while (!success) {
                success = seq.mutate(m_rng, maxLayers);
            }
    
            double candidateErr = seq.error(targetRGB);
    
            if (
                accept(m_rng, bestErr, candidateErr, T) && 
                m_parser->filter.checkVariance(seq.variance().normSquared())
            ){
                bestErr = candidateErr;
                
                // Track global best
                if (bestErr < bestGlobalErr) {
                    bestGlobal = seq;
                    bestGlobalErr = bestErr;
                }
            } else {
                seq.undoMutation();
            }
        }

        T *= alpha;
    }

    return bestGlobal;
}

bool TextureSequence::mutate(TextureRandomizer& rng, int maxLayers) {
    bool success = false;
    switch (rng.mutation()) {
        case REPLACE_TRANSLUCENT:
            if (numTranslucent()) {
                success = true;
                size_t indx = rng.randTranslucentIndex(this);
                m_prevChange = {REPLACE_TRANSLUCENT, m_translucentSeq[indx], indx};
                replaceTranslucent(indx, rng.randTraslucent());
            }
            break;
        case ADD:
            if (length() < maxLayers - 1) {
                success = true;
                m_prevChange = {ADD, {}, {}};
                addTranslucent(rng.randTraslucent());
            }
            break;
        case REMOVE:
            if (numTranslucent()) {
                success = true;
                size_t indx = rng.randTranslucentIndex(this);
                m_prevChange = {REMOVE, m_translucentSeq[indx], indx};
                eraseTranslucent(indx);
            }
            break;
        case REPLACE_OPAQUE:
            success = true;
            m_prevChange = {REPLACE_OPAQUE, m_opaque, {}};
            setOpaque(rng.randOpaque());
            break;
    }
    return success;
}