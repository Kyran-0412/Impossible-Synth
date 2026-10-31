#pragma once

// The Impossible Synth sound engine. Plain C++17, no JUCE, so it can be built
// and tested on its own (see tools/render_test.cpp).
//
//   Engine        4 layers, shared transport / macros / MIDI state
//   LayerEngine   one layer: a pool of voices plus its LFO clocks
//   Voice         one note: Physical (exciter -> resonator bank) or Wavetable,
//                 then drive, filter, volume and pan

#include "ModeTables.h"
#include "Params.h"
#include "Wavetable.h"

#include <array>
#include <atomic>
#include <cstdint>

namespace impossible
{

constexpr int kNumRes = kNumModes * 2; // each mode is a pair of resonators (for Fracture)
constexpr int kNumVoices = 16;         // voice slots per layer, including ones fading out
constexpr int kMaxPolyphony = 12;      // notes per layer that can sound at once
constexpr int kControlInterval = 32;   // samples between coefficient/modulation updates

class Rng
{
public:
    void seed (uint32_t s) { state = s ? s : 0x9E3779B9u; }
    uint32_t next()
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }
    float uniform() { return (float) (next() >> 8) * (1.0f / 16777216.0f); } // [0, 1)
    float bipolar() { return uniform() * 2.0f - 1.0f; }                      // [-1, 1)

private:
    uint32_t state = 0x9E3779B9u;
};

// Shared, per-render-call information the voices of one layer need.
struct ModContext
{
    float bend = 0.0f;                        // semitones
    float modWheel = 0.0f;                    // 0..1
    std::array<float, kNumMacros> macros {};  // 0..1
    std::array<double, kNumLfos> lfoPhase {}; // free-running LFO phase (cycles) at the start of this render call
    std::array<float, kNumLfos> lfoHz {};
    const Wavetable* table = nullptr;         // this layer's wavetable
};

class Voice
{
public:
    void prepare (double sampleRate);
    void start (int note, float velocity, const LayerParams& p, const ModContext& ctx, uint32_t seed);
    void retrigger (float velocity, const LayerParams& p, const ModContext& ctx);
    void fadeOut() { fading = true; }
    void kill();

    // Adds this voice's output into left/right.
    void render (float* left, float* right, int numSamples, const LayerParams& p, const ModContext& ctx);

    bool active = false;
    bool keyDown = false;
    bool sustained = false;
    bool fading = false;
    int note = -1;
    uint64_t age = 0;
    float level = 0.0f; // rough energy, for picking voices to steal

private:
    void updateControl (const LayerParams& p, const ModContext& ctx, int sampleInCall);
    LayerParams applyModulation (const LayerParams& p, const ModContext& ctx, int sampleInCall);
    void updateWavetable (const LayerParams& m, const ModContext& ctx, float f0);
    void renderWavetable (float& outL, float& outR, float& mono, const ModContext& ctx);
    void excite (float vel);

    double sr = 44100.0;
    float invSr = 1.0f / 44100.0f;
    float velocity = 0.0f;
    int64_t sampleCount = 0;
    int controlCounter = 0;
    uint32_t noteSeed = 1;
    Rng rng;
    int type = kLayerPhysical;

    // Amp envelope: Wavetable layers, and the sustained Physical exciters.
    int ampStage = 0; // 0 attack, 1 decay/sustain, 2 release
    float ampEnv = 0.0f, ampAttackCoef = 0.0f, ampDecayCoef = 0.0f, ampReleaseCoef = 0.0f, ampSustain = 1.0f;

    // Physical exciters.
    float sawPhase = 0.0f, sawInc = 0.0f;
    std::array<float, kMaxUnison> oscPhase {}, oscInc {}, oscPanL {}, oscPanR {};
    int oscCount = 1;
    float oscWave = 0.66f;

    // Wavetable oscillator.
    std::array<float, kMaxUnison> wtPhase {}, wtInc {}, wtPanL {}, wtPanR {};
    int wtCount = 1;
    int wtLevel = 0;
    int wtWarpMode = kWarpOff;
    float wtWarp = 0.0f;
    float wtFrame = 0.0f, wtFrameTarget = 0.0f, wtFrameStep = 0.0f;
    float subPhase = 0.0f, subInc = 0.0f, subLevel = 0.0f;
    float wtGain = 1.0f;

    // Modulation state.
    int envStage = 0; // mod envelope: 0 attack, 1 decay/sustain, 2 release
    float envValue = 0.0f;
    std::array<double, kNumLfos> lfoPhase {};
    float noteRandom = 0.0f;
    float pitchMod = 0.0f;

    // Output stage.
    float body = 1.0f;
    float driveGain = 1.0f, driveMakeup = 1.0f;
    bool driveOn = false;
    int filterMode = kFilterOff;
    float fA1 = 0.0f, fA2 = 0.0f, fA3 = 0.0f, fK = 2.0f, fComp = 1.0f;
    std::array<float, 2> ic1 {}, ic2 {};
    float gainL = 1.0f, gainR = 1.0f, gainLTarget = 1.0f, gainRTarget = 1.0f, gainLStep = 0.0f, gainRStep = 0.0f;
    float fadeGain = 1.0f, fadeStep = 0.0f;

    // Auto-level for sustained exciters: keeps the object's level steady as
    // Shape/Material change how much of the input resonates.
    float agcIn = 0.0f, agcOut = 0.0f, agcGain = 1.0f, agcCoef = 0.0f, agcFastCoef = 0.0f;
    float blockIn = 0.0f, blockOut = 0.0f;

    // Resonator state (complex one-pole: z = p*z + input; output = imag(z)).
    int numLive = 0;
    std::array<float, kNumRes> zr {}, zi {}, pr {}, pi {};
    std::array<float, kNumRes> resAmp {}, inTonal {}, inNoise {};
    std::array<float, kNumRes> gain {}, gainTarget {}, gainCoef {};
    std::array<float, kNumRes> panL {}, panR {};

    // Per-note random character, fixed for the life of the note.
    std::array<float, kNumModes> rndIrreg {}, rndFrac {}, rndSplit {}, rndTau {};
};

class LayerEngine
{
public:
    void prepare (double sampleRate, uint32_t seed);
    void setParams (const LayerParams& p) { params = p; }
    const LayerParams& getParams() const { return params; }

    bool accepts (int note, float velocity) const;

    // The shared context plus this layer's LFO clocks.
    ModContext context (const ModContext& shared, double bpm, bool hostPlaying, double ppq) const;

    void noteOn (int note, float velocity, const ModContext& ctx);
    void noteOff (int note, bool sustainDown);
    void releaseSustained();
    void allNotesOff();
    void allSoundOff();
    void fadeAll();

    // Adds this layer into left/right.
    void render (float* left, float* right, int numSamples, const ModContext& ctx);

    bool anyActive() const;
    int activeVoiceCount() const;

private:
    std::array<Voice, kNumVoices> voices;
    LayerParams params;
    double sr = 44100.0;
    std::array<double, kNumLfos> freePhase {};
    uint64_t ageCounter = 0;
    Rng rng;
};

class Engine
{
public:
    void prepare (double sampleRate);
    void setPatch (const Patch& p);

    // Call once per host block, before rendering it.
    void setTransport (double bpm, bool playing, double ppqAtBlockStart);

    void noteOn (int note, float velocity);
    void noteOff (int note);
    void setSustain (bool down);
    void setPitchBend (float semitones) { shared.bend = semitones; }
    void setModWheel (float value) { shared.modWheel = value; }
    void allNotesOff();
    void allSoundOff();

    // An imported wavetable for one layer (used when its table choice is "Imported").
    // The table must stay alive while the engine might use it.
    void setImportedTable (int layer, const Wavetable* table);

    // Writes (overwrites) numSamples into left/right.
    void render (float* left, float* right, int numSamples);

    int activeVoiceCount() const;

private:
    const Wavetable* tableFor (int layer) const;

    std::array<LayerEngine, kNumLayers> layers;
    std::array<bool, kNumLayers> wasEnabled {};
    std::array<std::atomic<const Wavetable*>, kNumLayers> imported {};
    ModContext shared;
    double sr = 44100.0;
    double bpm = 120.0;
    bool hostPlaying = false;
    double ppq = 0.0; // host position, advanced as we render
    bool sustainDown = false;
};

} // namespace impossible
