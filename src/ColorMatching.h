#pragma once

#include <random>
#include <utility>
#include <algorithm>

#include "TextureParser.h"

enum Mutation {
    REPLACE_TRANSLUCENT,
    ADD,
    REMOVE,
    REPLACE_OPAQUE
};

struct MutationInfo {
    Mutation mut;
    TextureInfo tex;
    size_t indx;
};

class TextureRandomizer;

class TextureSequence {
    public:
    bool operator== (const TextureSequence& other) const {
        if (!(m_opaque == other.m_opaque)) {
            return false;
        }
        if (length() != other.length()) {
            return false;
        }
        return std::equal(m_translucentSeq.begin(), m_translucentSeq.end(), other.m_translucentSeq.begin());
    }

    std::vector<TextureInfo> seq() const {
        std::vector<TextureInfo> s = {m_opaque};
        for (auto& x : m_translucentSeq) {
            s.push_back(x);
        }

        return s;
    }

    double error(const Vec3& targetRGB) const;

    int length() const {
        return m_translucentSeq.size() + 1;
    }

    int numTranslucent() const {
        return m_translucentSeq.size();
    }

    void setOpaque(const TextureInfo& tex) {
        m_opaque = tex;
    }

    const TextureInfo opaque() const {
        return m_opaque;
    }

    void addTranslucent(const TextureInfo& tex) {
        m_translucentSeq.push_back(tex);
    }

    void popTranslucent() {
        m_translucentSeq.pop_back();
    }

    void eraseTranslucent(size_t indx) {
        m_translucentSeq.erase(m_translucentSeq.begin() + indx);
    }

    void replaceTranslucent(size_t indx, const TextureInfo& tex) {
        m_translucentSeq[indx] = tex;
    }

    void insertTranslucent(size_t indx, const TextureInfo& tex) {
        m_translucentSeq.insert(m_translucentSeq.begin() + indx, tex);
    }

    bool mutate(TextureRandomizer& rng, int maxLayers);

    void undoMutation() {
        switch (m_prevChange.mut) {
            case REPLACE_TRANSLUCENT:
                replaceTranslucent(m_prevChange.indx, std::move(m_prevChange.tex));
                break;
            case ADD:
                popTranslucent();
                break;
            case REMOVE:
                insertTranslucent(m_prevChange.indx, std::move(m_prevChange.tex));
                break;
            case REPLACE_OPAQUE:
                setOpaque(std::move(m_prevChange.tex));
                break;
        }
    }

    Vec3 blendRGB() const {
        Vec3 accRGB = srgbToLinear(m_opaque.parsedTexture.avgRGB);

        for (const auto& x : m_translucentSeq) {
            double a = x.parsedTexture.alphaRatio;
            Vec3 layerLinear = srgbToLinear(x.parsedTexture.avgRGB);
            accRGB *= (1. - a);
            accRGB += a * layerLinear;
        }

        return linearToSrgb(accRGB);
    }

    Vec3 blendRGBDebug() const {
        Vec3 accRGB = srgbToLinear(m_opaque.parsedTexture.avgRGB);

        for (const auto& x : m_translucentSeq) {
            std::cout << "acc: " << accRGB << std::endl;
            std::cout << "trns: " << x.parsedTexture.avgRGB << " " << "alph: " << x.parsedTexture.alphaRatio << std::endl;
            double a = x.parsedTexture.alphaRatio;
            Vec3 layerLinear = srgbToLinear(x.parsedTexture.avgRGB);
            accRGB *= (1. - a);
            accRGB += a * layerLinear;
        }

        std::cout << "final: " << linearToSrgb(accRGB) << std::endl;

        return linearToSrgb(accRGB);
    }

    Vec3 variance() const {
        Vec3 accRGB = m_opaque.parsedTexture.avgRGB; 
        Vec3 accVar = m_opaque.parsedTexture.varRGB;

        for (const auto& x : m_translucentSeq) {
            double a = x.parsedTexture.alphaRatio;
            double transparentRatio = x.parsedTexture.numTransparent / (x.textureData.width * x.textureData.height);

            accRGB = (1. - a) * accRGB + a * x.parsedTexture.avgRGB;
            auto within = 0.5 * ((1. - a) * accVar + a * x.parsedTexture.varRGB);
            auto avgDiff = accRGB - x.parsedTexture.avgRGB;
            auto between = transparentRatio * 0.25 * elementwiseMul(avgDiff, avgDiff);
            accVar = within + between;
        }

        return accVar; 
    }
    
    private:
    static inline double srgbToLinear(double c) {
        // expects c in [0, 1]
        if (c <= 0.04045)
            return c / 12.92;
        return std::pow((c + 0.055) / 1.055, 2.4);
    }

    static inline double linearToSrgb(double c) {
        // expects c in [0, 1]
        if (c <= 0.0031308)
            return c * 12.92;
        return 1.055 * std::pow(c, 1.0 / 2.4) - 0.055;
    }

    static inline Vec3 srgbToLinear(const Vec3& v) {
        return Vec3(srgbToLinear(v.x), srgbToLinear(v.y), srgbToLinear(v.z));
    }

    static inline Vec3 linearToSrgb(const Vec3& v) {
        return Vec3(linearToSrgb(v.x), linearToSrgb(v.y), linearToSrgb(v.z));
    }

    TextureInfo m_opaque;
    std::vector<TextureInfo> m_translucentSeq;
    MutationInfo m_prevChange;
};

class TextureRandomizer {
    public:
    TextureRandomizer()=delete;
    TextureRandomizer(const TextureParser* parser) : 
        m_parser(parser),
        m_prob(0.0, 1.0)
    {
        m_rng = std::mt19937(std::time(nullptr));
    }

    TextureInfo randTraslucent() {
        auto indx = m_rng() % m_parser->translucentTextures().size();
        return m_parser->translucentTextures()[indx];
    }

    TextureInfo randOpaque() {
        auto indx = m_rng() % m_parser->opaqueTextures().size();
        return m_parser->opaqueTextures()[indx];
    }

    size_t randTranslucentIndex(TextureSequence* seq) {
        return m_rng() % seq->numTranslucent();
    }

    size_t randTranslucentIndex(TextureSequence& seq) {
        return m_rng() % seq.numTranslucent();
    }

    TextureSequence randomSequence(int numLayers) {
        TextureSequence seq;

        TextureInfo opaque = randOpaque();
        seq.setOpaque(opaque);

        for (int i = 0; i < numLayers; ++i) {
            seq.addTranslucent(randTraslucent());
        }

        return seq;
    }

    uint_fast32_t rng() {
        return m_rng();
    }

    double rand01() {
        return m_prob(m_rng);
    }

    Mutation mutation() {
        double move = m_prob(m_rng);

        if (move < 0.25) {
            return REPLACE_TRANSLUCENT;
        }
        else if (move < 0.5) {
            return ADD;
        }
        else if (move < 0.75) {
            return REMOVE;
        }
        else {
            return REPLACE_OPAQUE;
        }
    }

    private:
    std::mt19937 m_rng;
    std::uniform_real_distribution<> m_prob;

    const TextureParser* m_parser;
};

class Optimizer {
    public:
    Optimizer(
        const TextureParser* const parser
    ) :
        m_parser(parser),
        m_rng(parser)
    {}

    TextureRandomizer& rng() {
        return m_rng;
    }

    TextureSequence getBestMatch(const Vec3& targetRGB, int maxLayers, bool localSearch=true);
    
    int maxIters = 100;

    private:

    TextureSequence getBestMatchGreedy(const Vec3& targetRGB, int maxLayers) const;
    TextureSequence getBestMatchLocalSearch(TextureSequence& startingState, const Vec3& targetRGB, int maxLayers) ;

    const TextureParser* m_parser;
    TextureRandomizer m_rng;
};