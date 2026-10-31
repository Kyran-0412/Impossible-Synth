#pragma once

// Band-limited wavetables. Each table is a set of single-cycle frames; each
// frame is stored at several "mip levels" with fewer harmonics, so high notes
// don't alias. Plain C++ (no JUCE).

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace impossible
{

class Wavetable
{
public:
    static constexpr int kFrameSize = 2048; // input frame length (the Serum/Vital standard)
    static constexpr int kNumLevels = 10;   // level l holds up to 512 >> l harmonics
    static constexpr int kMaxFrames = 256;

    // Builds a table from single-cycle frames of kFrameSize samples each.
    // pitchDivisor > 1 means the table's fundamental sits that many times below
    // the played note (used for near-inharmonic "object" spectra).
    static std::shared_ptr<Wavetable> fromFrames (const std::vector<std::vector<float>>& frames,
                                                  std::string name, int pitchDivisor = 1);

    // Builds a table from audio: splits it into kFrameSize frames, or treats a
    // short file as one cycle. Returns nullptr if there's nothing usable.
    static std::shared_ptr<Wavetable> fromAudio (const float* samples, int numSamples, std::string name);

    int numFrames() const { return frames; }
    int pitchDivisor() const { return divisor; }
    const std::string& name() const { return label; }

    // Level to use when at most maxHarmonics harmonics fit below Nyquist.
    static int levelFor (float maxHarmonics);

    // phase in [0, 1), framePos in [0, numFrames - 1].
    float read (float framePos, int level, float phase) const;

    // Raw access (for drawing): samples of one frame at one level.
    const float* frameData (int frame, int level) const;
    int levelSize (int level) const { return sizes[(size_t) level]; }

private:
    int frames = 0;
    int divisor = 1;
    std::string label;
    std::vector<float> data;
    int stride = 0;
    std::array<int, kNumLevels> sizes {}, offsets {};
};

// The built-in tables (generated once, on first use).
const std::vector<std::shared_ptr<Wavetable>>& builtInWavetables();
constexpr int kNumBuiltInWavetables = 16;

} // namespace impossible
