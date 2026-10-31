#pragma once

// The Impossible Synth sound engine: a modal (resonator bank) physical model
// with controls that bend the physics. Plain C++17, no JUCE, so it can be
// built and tested on its own (see tools/render_test.cpp).

#include "ModeTables.h"

#include <array>
#include <cstdint>

namespace impossible
{

constexpr int kNumRes = kNumModes * 2; // each mode is a pair of resonators (for Fracture)
constexpr int kNumVoices = 16;         // voice slots, including ones fading out
constexpr int kMaxPolyphony = 12;      // notes that can sound at once
constexpr int kControlInterval = 32;   // samples between coefficient updates

enum Exciter
{
    kStrike = 0,
    kPluck,
    kBow,
    kBreath
};

struct Params
{
    int exciter = kStrike;
    float hardness = 0.5f;  // 0..1  mallet hardness / brightness of excitation
    float position = 0.2f;  // 0..0.5 where the object is hit
    float shape = 0.0f;     // 0..1  String→Tube→Bar→Plate→Bell
    float material = 0.6f;  // 0..1  Rubber→Wood→Metal→Glass
    float size = 0.5f;      // 0..1  tiny→huge (ring time, body, irregularity)
    float decay = 1.0f;     // ring-time multiplier
    float release = 1.0f;   // seconds, damping after key release
    int octave = 0;         // -2..+2
    float bloom = 0.0f;     // 0..1  sound swells after the hit
    float drift = 0.0f;     // -1..1 partials bend down/up as they ring
    float transmute = 0.0f; // -1..1 material changes during the note
    float fracture = 0.0f;  // 0..1  partials split apart
    float evolve = 1.5f;    // seconds, speed of Bloom/Drift/Transmute/Fracture
};

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

class Voice
{
public:
    void prepare (double sampleRate);
    void start (int note, float velocity, const Params& p, float bend, uint32_t seed);
    void retrigger (float velocity, const Params& p, float bend);
    void fadeOut();
    void kill();

    // Adds this voice's output into left/right.
    void render (float* left, float* right, int numSamples, const Params& p, float bend);

    bool active = false;
    bool keyDown = false;
    bool sustained = false;
    bool fading = false;
    int note = -1;
    uint64_t age = 0;
    float level = 0.0f; // rough energy, for picking voices to steal

private:
    void updateControl (const Params& p, float bend);
    void excite (float vel);

    double sr = 44100.0;
    float invSr = 1.0f / 44100.0f;
    float velocity = 0.0f;
    int64_t sampleCount = 0;
    int controlCounter = 0;
    float contEnv = 0.0f, attackCoef = 0.0f, releaseCoef = 0.0f;
    float sawPhase = 0.0f, sawInc = 0.0f;
    float fadeGain = 1.0f, fadeStep = 0.0f;
    int numLive = 0;
    Rng rng;

    // Resonator state (complex one-pole: z = p*z + input; output = imag(z)).
    std::array<float, kNumRes> zr {}, zi {}, pr {}, pi {};
    std::array<float, kNumRes> resAmp {}, inTonal {}, inNoise {};
    std::array<float, kNumRes> gain {}, gainTarget {}, gainCoef {};
    std::array<float, kNumRes> panL {}, panR {};

    // Per-note random character, fixed for the life of the note.
    std::array<float, kNumModes> rndIrreg {}, rndFrac {}, rndSplit {}, rndTau {};
};

class Engine
{
public:
    void prepare (double sampleRate);
    void setParams (const Params& p) { params = p; }

    void noteOn (int note, float velocity);
    void noteOff (int note);
    void setSustain (bool down);
    void setPitchBend (float semitones) { bend = semitones; }
    void allNotesOff();
    void allSoundOff();

    // Writes (overwrites) numSamples into left/right.
    void render (float* left, float* right, int numSamples);

    int activeVoiceCount() const;

private:
    std::array<Voice, kNumVoices> voices;
    Params params;
    float bend = 0.0f;
    bool sustainDown = false;
    uint64_t ageCounter = 0;
    Rng rng;
};

} // namespace impossible
