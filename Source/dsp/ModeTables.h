#pragma once

// Frequency ratios for each object shape, and the material presets.
// Kept free of JUCE so the engine can be tested on its own.

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace impossible
{

constexpr int kNumModes = 32;
constexpr int kNumShapes = 5;   // String, Tube, Bar, Plate, Bell
constexpr int kNumMaterials = 4; // Rubber, Wood, Metal, Glass

inline const char* const kShapeNames[kNumShapes] = { "String", "Tube", "Bar", "Plate", "Bell" };
inline const char* const kMaterialNames[kNumMaterials] = { "Rubber", "Wood", "Metal", "Glass" };

using RatioTable = std::array<std::array<float, kNumModes>, kNumShapes>;

// Log of each shape's mode ratios (relative to the fundamental), so shapes
// can be morphed smoothly in the log-frequency domain.
inline const RatioTable& shapeLogRatios()
{
    static const RatioTable table = []
    {
        RatioTable t {};
        const double pi = 3.14159265358979323846;

        // String: plain harmonic series.
        for (int k = 0; k < kNumModes; ++k)
            t[0][(size_t) k] = (float) (k + 1);

        // Tube closed at one end: odd harmonics only.
        for (int k = 0; k < kNumModes; ++k)
            t[1][(size_t) k] = (float) (2 * k + 1);

        // Free-free bar (xylophone / marimba style): ratios of beta^2.
        {
            const double beta[3] = { 4.730040745, 7.853204624, 10.995607838 };
            for (int k = 0; k < kNumModes; ++k)
            {
                const double b = k < 3 ? beta[k] : (2.0 * k + 3.0) * pi * 0.5;
                t[2][(size_t) k] = (float) ((b * b) / (beta[0] * beta[0]));
            }
        }

        // Simply supported rectangular plate, aspect ratio 1.37 (avoids
        // repeated frequencies): f ~ m^2 + (n/a)^2.
        {
            std::vector<double> f;
            const double a = 1.37;
            for (int m = 1; m <= 12; ++m)
                for (int n = 1; n <= 12; ++n)
                    f.push_back (m * m + (n * n) / (a * a));
            std::sort (f.begin(), f.end());
            for (int k = 0; k < kNumModes; ++k)
                t[3][(size_t) k] = (float) (f[(size_t) k] / f[0]);
        }

        // Church-bell partials (prime, hum, tierce, quint, nominal, ...),
        // extended upwards with gradually tightening spacing. The prime (1.0)
        // comes first so every shape keeps a mode at the played note while
        // morphing; the hum an octave below sits second.
        {
            const float bell[16] = { 1.0f, 0.5f, 1.183f, 1.506f, 2.0f, 2.514f, 2.662f, 3.011f,
                                     3.36f, 4.0f, 4.209f, 4.86f, 5.43f, 6.0f, 6.67f, 7.31f };
            for (int k = 0; k < kNumModes; ++k)
                t[4][(size_t) k] = k < 16 ? bell[k]
                                          : t[4][(size_t) k - 1] * (1.0f + 0.9f / (float) (k - 8));
        }

        for (auto& shape : t)
            for (auto& r : shape)
                r = std::log (r);

        return t;
    }();
    return table;
}

// Ratio of mode k for a shape position 0..1 that morphs String→Tube→Bar→Plate→Bell.
inline float shapeRatio (float shape, int k)
{
    const auto& t = shapeLogRatios();
    const float pos = std::clamp (shape, 0.0f, 1.0f) * (kNumShapes - 1);
    const int i = std::min ((int) pos, kNumShapes - 2);
    const float f = pos - (float) i;
    return std::exp (t[(size_t) i][(size_t) k] * (1.0f - f) + t[(size_t) i + 1][(size_t) k] * f);
}

struct MaterialProps
{
    float t60;   // ring time of the fundamental, seconds
    float alpha; // how much faster higher modes die (T60 ~ ratio^-alpha)
    float beta;  // how much quieter higher modes start (amp ~ ratio^-beta)
    float stiff; // inharmonic stretch of upper modes
};

// Material position 0..1 morphs Rubber→Wood→Metal→Glass.
inline MaterialProps materialAt (float m)
{
    static const MaterialProps anchors[kNumMaterials] = {
        { 0.12f, 1.6f, 1.4f, 0.0f },    // rubber: dull thud
        { 0.6f, 1.1f, 0.9f, 0.0003f },  // wood: warm knock
        { 5.0f, 0.35f, 0.5f, 0.0001f }, // metal: long bright ring
        { 3.0f, 0.15f, 0.25f, 0.0008f } // glass: pure, glassy, slightly stretched
    };

    const float pos = std::clamp (m, 0.0f, 1.0f) * (kNumMaterials - 1);
    const int i = std::min ((int) pos, kNumMaterials - 2);
    const float f = pos - (float) i;
    const auto& a = anchors[i];
    const auto& b = anchors[i + 1];
    auto lerp = [f] (float x, float y) { return x + (y - x) * f; };

    return { std::exp (lerp (std::log (a.t60), std::log (b.t60))),
             lerp (a.alpha, b.alpha),
             lerp (a.beta, b.beta),
             lerp (a.stiff, b.stiff) };
}

} // namespace impossible
