#pragma once

// Factory presets and the Randomize button's patch generator.
// Plain C++ so tools/render_test.cpp can check every preset's levels.

#include "Engine.h"

#include <string>
#include <vector>

namespace impossible
{

struct Preset
{
    std::string name;
    Patch patch;
};

inline std::vector<Preset> factoryPresets()
{
    std::vector<Preset> list;
    auto add = [&list] (const char* name, auto&& setup)
    {
        Patch patch;
        setup (patch);
        list.push_back ({ name, patch });
    };

    // ---- Keys & mallets --------------------------------------------------
    add ("Init", [] (Patch&) {});

    add ("Glass Marimba", [] (Patch& P)
    {
        auto& p = P.layers[0];
        p.shape = 0.5f; p.material = 0.95f; p.hardness = 0.45f; p.position = 0.22f;
        p.decay = 1.2f; P.space = 0.3f;
    });

    add ("Rubber Bell", [] (Patch& P)
    {
        auto& p = P.layers[0];
        p.shape = 1.0f; p.material = 0.08f; p.hardness = 0.7f; p.decay = 4.0f; P.space = 0.35f;
    });

    add ("Melting Vibes", [] (Patch& P)
    {
        auto& p = P.layers[0];
        p.shape = 0.5f; p.material = 0.7f; p.transmute = -0.7f; p.evolve = 2.5f; p.decay = 2.0f;
        p.hardness = 0.4f; P.space = 0.35f;
        p.lfoShape[0] = kLfoSine; p.lfoRate[0] = 5.0f;
        p.mod (0, kSrcLfo1, kDstVolume, 0.15f);
    });

    add ("Planet Bell", [] (Patch& P)
    {
        auto& p = P.layers[0];
        p.shape = 1.0f; p.material = 0.7f; p.size = 1.0f; p.decay = 3.0f; p.bloom = 0.4f;
        p.octave = 1; P.space = 0.5f; P.room = 0.9f;
    });

    add ("Cracked Kalimba", [] (Patch& P)
    {
        auto& p = P.layers[0];
        p.shape = 0.55f; p.material = 0.45f; p.fracture = 0.6f; p.evolve = 0.6f; p.decay = 0.8f;
        p.position = 0.12f; p.hardness = 0.6f; P.space = 0.3f;
    });

    // ---- Plucks ----------------------------------------------------------
    add ("Wire Harp", [] (Patch& P)
    {
        auto& p = P.layers[0];
        p.exciter = kPluck; p.shape = 0.0f; p.material = 0.75f; p.position = 0.12f; p.decay = 1.5f;
        P.space = 0.35f;
    });

    add ("Rubber Band Pluck", [] (Patch& P)
    {
        auto& p = P.layers[0];
        p.exciter = kPluck; p.shape = 0.0f; p.material = 0.1f; p.decay = 3.0f; p.hardness = 0.7f;
        p.drift = 0.25f; p.evolve = 0.3f; P.space = 0.2f;
    });

    add ("Bent Koto", [] (Patch& P)
    {
        auto& p = P.layers[0];
        p.exciter = kPluck; p.shape = 0.08f; p.material = 0.45f; p.position = 0.1f;
        p.drift = -0.15f; p.evolve = 0.6f; P.space = 0.3f;
    });

    add ("Glass Rain", [] (Patch& P)
    {
        auto& p = P.layers[0];
        p.exciter = kPluck; p.shape = 0.75f; p.material = 1.0f; p.hardness = 0.8f; p.decay = 0.6f;
        P.space = 0.6f; P.room = 0.85f;
        p.lfoShape[0] = kLfoSampleHold; p.lfoSync[0] = 7; // 1/16
        p.lfoShape[1] = kLfoDrift; p.lfoRate[1] = 0.3f;
        p.mod (0, kSrcLfo1, kDstPosition, 0.4f);
        p.mod (1, kSrcLfo2, kDstShape, 0.2f);
    });

    // ---- Pads ------------------------------------------------------------
    add ("Bowed Glass", [] (Patch& P)
    {
        auto& p = P.layers[0];
        p.exciter = kBow; p.shape = 0.1f; p.material = 1.0f; p.hardness = 0.6f;
        p.attack = 0.6f; p.release = 2.0f; P.space = 0.5f;
    });

    add ("Singing Bowl Pad", [] (Patch& P)
    {
        auto& p = P.layers[0];
        p.exciter = kBow; p.shape = 1.0f; p.material = 0.7f; p.hardness = 0.5f;
        p.attack = 1.0f; p.release = 3.0f; p.bloom = 0.3f; P.space = 0.6f; P.room = 0.85f;
        p.lfoShape[0] = kLfoDrift; p.lfoRate[0] = 0.2f;
        p.mod (0, kSrcLfo1, kDstMaterial, 0.1f);
    });

    add ("Breathing Tubes", [] (Patch& P)
    {
        auto& p = P.layers[0];
        p.exciter = kBreath; p.shape = 0.25f; p.material = 0.8f; p.hardness = 0.35f;
        p.attack = 0.8f; p.release = 2.0f; P.space = 0.55f;
        p.lfoShape[0] = kLfoSine; p.lfoRate[0] = 0.15f;
        p.mod (0, kSrcLfo1, kDstShape, 0.15f);
    });

    add ("Supersaw Cathedral", [] (Patch& P)
    {
        auto& p = P.layers[0];
        p.exciter = kOsc; p.oscWave = 0.66f; p.unison = 6; p.detune = 0.35f;
        p.shape = 1.0f; p.material = 0.75f; p.body = 0.6f;
        p.attack = 0.5f; p.release = 2.5f; p.cutoff = 6000.0f; P.space = 0.6f; P.room = 0.9f; P.output = 3.0f;
    });

    add ("Frozen Choir", [] (Patch& P)
    {
        auto& p = P.layers[0];
        p.exciter = kOsc; p.oscWave = 0.4f; p.unison = 4; p.detune = 0.2f;
        p.shape = 0.25f; p.material = 0.9f; p.body = 0.85f;
        p.attack = 1.2f; p.release = 3.0f; P.space = 0.7f; P.room = 0.9f;
        p.lfoShape[0] = kLfoDrift; p.lfoRate[0] = 0.3f;
        p.mod (0, kSrcLfo1, kDstShape, 0.1f);
    });

    // ---- Bass ------------------------------------------------------------
    add ("Rubber Sub", [] (Patch& P)
    {
        auto& p = P.layers[0];
        p.exciter = kOsc; p.oscWave = 0.2f; p.shape = 0.0f; p.material = 0.15f; p.body = 0.4f;
        p.octave = 1; p.attack = 0.005f; p.release = 0.2f;
        p.cutoff = 900.0f; p.filterEnv = 0.3f; p.envDecay = 0.3f; p.drive = 0.25f; P.space = 0.05f;
    });

    add ("Wooden Growl", [] (Patch& P)
    {
        auto& p = P.layers[0];
        p.exciter = kOsc; p.oscWave = 0.66f; p.unison = 2; p.detune = 0.15f;
        p.shape = 0.0f; p.material = 0.35f; p.body = 0.7f; p.octave = 1;
        p.attack = 0.003f; p.release = 0.15f;
        p.cutoff = 400.0f; p.resonance = 0.45f; p.filterEnv = 0.55f;
        p.envDecay = 0.35f; p.envSustain = 0.2f; p.drive = 0.5f; P.space = 0.05f;
    });

    add ("Iron Pluck Bass", [] (Patch& P)
    {
        auto& p = P.layers[0];
        p.exciter = kPluck; p.shape = 0.0f; p.material = 0.66f; p.octave = 1; p.hardness = 0.7f;
        p.decay = 0.6f; p.release = 0.3f; p.drive = 0.6f;
        p.cutoff = 1200.0f; p.resonance = 0.3f; p.filterEnv = 0.4f; p.envDecay = 0.25f; P.space = 0.05f;
    });

    // ---- Leads -----------------------------------------------------------
    add ("Glass Lead", [] (Patch& P)
    {
        auto& p = P.layers[0];
        p.exciter = kOsc; p.oscWave = 0.85f; p.unison = 1; p.detune = 0.1f;
        p.shape = 0.0f; p.material = 1.0f; p.body = 0.7f; p.attack = 0.01f; p.release = 0.3f;
        p.cutoff = 5000.0f; p.resonance = 0.2f; p.drive = 0.2f; P.space = 0.3f; P.output = 5.0f;
        p.lfoShape[0] = kLfoSine; p.lfoRate[0] = 5.5f;
        p.mod (0, kSrcLfo1, kDstPitch, 0.015f);
    });

    add ("Fractured Lead", [] (Patch& P)
    {
        auto& p = P.layers[0];
        p.exciter = kOsc; p.oscWave = 0.66f; p.shape = 0.75f; p.material = 0.7f; p.body = 0.8f;
        p.fracture = 0.7f; p.evolve = 0.5f; p.drive = 0.35f; p.cutoff = 4000.0f;
        p.attack = 0.01f; p.release = 0.3f; P.space = 0.3f; P.output = 6.0f;
    });

    // ---- Textures & FX ---------------------------------------------------
    add ("Impossible Object", [] (Patch& P)
    {
        auto& p = P.layers[0];
        p.shape = 0.85f; p.material = 0.5f; p.bloom = 0.7f; p.drift = 0.5f; p.transmute = 0.5f;
        p.fracture = 0.4f; p.evolve = 2.5f; p.decay = 3.0f; P.space = 0.6f;
    });

    add ("Collapsing Star", [] (Patch& P)
    {
        auto& p = P.layers[0];
        p.shape = 1.0f; p.material = 0.8f; p.size = 1.0f; p.decay = 5.0f; p.drift = -1.0f;
        p.evolve = 4.0f; p.bloom = 0.5f; p.octave = 1; P.space = 0.7f; P.room = 0.95f;
    });

    add ("Clockwork Insects", [] (Patch& P)
    {
        auto& p = P.layers[0];
        p.exciter = kPluck; p.shape = 0.7f; p.material = 0.75f; p.hardness = 0.9f; p.octave = 3;
        p.decay = 0.3f; P.space = 0.4f;
        p.lfoShape[0] = kLfoSampleHold; p.lfoSync[0] = 7; // 1/16
        p.lfoShape[1] = kLfoSquare; p.lfoSync[1] = 6;     // 1/8
        p.mod (0, kSrcLfo1, kDstShape, 0.5f);
        p.mod (1, kSrcLfo2, kDstPosition, 0.3f);
    });

    add ("Tidal Drone", [] (Patch& P)
    {
        auto& p = P.layers[0];
        p.exciter = kBreath; p.shape = 0.9f; p.material = 0.85f; p.size = 0.95f; p.hardness = 0.5f;
        p.attack = 3.0f; p.release = 5.0f; p.bloom = 0.3f; p.cutoff = 3000.0f;
        P.space = 0.75f; P.room = 0.95f;
        p.lfoShape[0] = kLfoDrift; p.lfoRate[0] = 0.1f;
        p.lfoShape[1] = kLfoSine; p.lfoRate[1] = 0.07f;
        p.mod (0, kSrcLfo1, kDstMaterial, 0.3f);
        p.mod (1, kSrcLfo2, kDstCutoff, 0.3f);
    });

    // ---- Wavetable -------------------------------------------------------
    // Built-in table numbers: 0 Basic Shapes, 1 PWM, 2 Saw Sync, 3 Vowels, 4 Harmonic Sweep,
    // 5 Organ, 6 FM Growl, 7 FM Bell, 8 Wavefold, 9 Steps, 10 Chaos, 11 Spectral Noise,
    // 12 Object Morph, 13 Glass Spectrum, 14 Comb Saw, 15 Bitwise.
    auto wavetableOnly = [] (Patch& P) -> LayerParams&
    {
        auto& w = P.layers[0];
        w.type = kLayerWavetable;
        return w;
    };

    add ("Vowel Pad", [&] (Patch& P)
    {
        auto& w = wavetableOnly (P);
        w.wtTable = 3; w.wtPos = 0.3f; w.wtUnison = 3; w.wtDetune = 0.25f;
        w.attack = 0.6f; w.release = 2.0f; w.cutoff = 5000.0f; P.space = 0.5f;
        w.lfoShape[0] = kLfoDrift; w.lfoRate[0] = 0.15f;
        w.mod (0, kSrcLfo1, kDstWtPos, 0.35f);
    });

    add ("Sync Lead", [&] (Patch& P)
    {
        auto& w = wavetableOnly (P);
        w.wtTable = 2; w.wtPos = 0.15f; w.wtUnison = 1; w.wtDetune = 0.1f;
        w.attack = 0.005f; w.release = 0.25f; w.drive = 0.25f; w.cutoff = 8000.0f;
        w.envDecay = 0.4f; w.mod (0, kSrcEnv, kDstWtPos, 0.5f);
        w.lfoShape[0] = kLfoSine; w.lfoRate[0] = 5.5f; w.mod (1, kSrcLfo1, kDstPitch, 0.012f);
        P.space = 0.25f;
    });

    add ("Growl Bass", [&] (Patch& P)
    {
        auto& w = wavetableOnly (P);
        w.wtTable = 6; w.wtPos = 0.2f; w.octave = 1; w.wtSub = 0.5f;
        w.attack = 0.003f; w.release = 0.15f;
        w.cutoff = 1800.0f; w.resonance = 0.35f; w.drive = 0.45f;
        w.lfoShape[0] = kLfoTriangle; w.lfoSync[0] = 6; // 1/8
        w.mod (0, kSrcLfo1, kDstWtPos, 0.5f);
        P.space = 0.05f;
    });

    add ("Digital Rain", [&] (Patch& P)
    {
        auto& w = wavetableOnly (P);
        w.wtTable = 15; w.wtPos = 0.1f; w.octave = 3; w.volume = 6.0f;
        w.attack = 0.002f; w.ampDecay = 0.35f; w.ampSustain = 0.0f; w.release = 0.4f;
        w.mod (0, kSrcRandom, kDstWtPos, 0.8f);
        P.space = 0.6f; P.room = 0.85f;
    });

    add ("Supersaw Wall", [&] (Patch& P)
    {
        auto& w = wavetableOnly (P);
        w.wtTable = 0; w.wtPos = 0.66f; w.wtUnison = 6; w.wtDetune = 0.45f; w.wtSpread = 1.0f;
        w.attack = 0.05f; w.release = 1.2f; w.cutoff = 9000.0f;
        P.space = 0.45f;
    });

    add ("Chaos Engine", [&] (Patch& P)
    {
        auto& w = wavetableOnly (P);
        w.wtTable = 10; w.wtWarpMode = kWarpFold; w.wtWarp = 0.3f;
        w.filterMode = kFilterBP; w.cutoff = 1500.0f; w.resonance = 0.4f;
        w.lfoShape[0] = kLfoDrift; w.lfoRate[0] = 0.5f; w.mod (0, kSrcLfo1, kDstWtPos, 0.8f);
        w.lfoShape[1] = kLfoSampleHold; w.lfoSync[1] = 7; w.mod (1, kSrcLfo2, kDstCutoff, 0.3f);
        w.release = 0.5f; P.space = 0.4f;
    });

    add ("Morphing Organ", [&] (Patch& P)
    {
        auto& w = wavetableOnly (P);
        w.wtTable = 5; w.attack = 0.02f; w.release = 0.4f;
        w.lfoShape[0] = kLfoSine; w.lfoRate[0] = 0.1f; w.mod (0, kSrcLfo1, kDstWtPos, 0.9f);
        w.wtPos = 0.5f;
        auto& air = P.layers[1];
        air.enabled = 1; air.type = kLayerPhysical; air.exciter = kBreath; air.shape = 0.25f;
        air.material = 0.8f; air.hardness = 0.3f; air.attack = 0.1f; air.release = 0.4f; air.volume = -12.0f;
        P.space = 0.35f;
    });

    // ---- Layered ---------------------------------------------------------
    add ("Glass Over Saw", [] (Patch& P)
    {
        auto& glass = P.layers[0];
        glass.exciter = kBow; glass.shape = 0.1f; glass.material = 1.0f; glass.hardness = 0.6f;
        glass.attack = 0.4f; glass.release = 2.0f;
        glass.mod (0, kSrcMacro1, kDstMaterial, -0.6f);

        auto& saw = P.layers[1];
        saw.enabled = 1; saw.type = kLayerWavetable; saw.wtTable = 0; saw.wtPos = 0.66f;
        saw.wtUnison = 4; saw.wtDetune = 0.3f; saw.volume = -6.0f;
        saw.attack = 0.6f; saw.release = 2.0f; saw.cutoff = 3000.0f;
        saw.mod (0, kSrcMacro1, kDstCutoff, 0.4f);
        P.space = 0.5f;
    });

    add ("Bell Choir", [] (Patch& P)
    {
        auto& bell = P.layers[0];
        bell.shape = 1.0f; bell.material = 0.7f; bell.size = 0.8f; bell.decay = 2.5f; bell.bloom = 0.3f;

        auto& choir = P.layers[1];
        choir.enabled = 1; choir.type = kLayerWavetable; choir.wtTable = 3; choir.wtPos = 0.5f;
        choir.wtUnison = 3; choir.wtDetune = 0.2f; choir.volume = -4.0f;
        choir.attack = 1.2f; choir.release = 3.0f; choir.cutoff = 6000.0f;
        choir.lfoShape[0] = kLfoDrift; choir.lfoRate[0] = 0.2f; choir.mod (0, kSrcLfo1, kDstWtPos, 0.4f);
        choir.mod (1, kSrcMacro1, kDstVolume, -1.0f);
        P.space = 0.6f; P.room = 0.9f;
    });

    add ("Singing Bell", [] (Patch& P)
    {
        // One layer: a vowel wavetable played *through* a bell.
        auto& p = P.layers[0];
        p.exciter = kExcWavetable; p.wtTable = 3; p.wtUnison = 2; p.wtDetune = 0.15f;
        p.shape = 0.9f; p.material = 0.55f; p.body = 0.75f; p.attack = 0.3f; p.release = 2.0f;
        p.volume = 4.0f;
        p.lfoShape[0] = kLfoDrift; p.lfoRate[0] = 0.25f; p.mod (0, kSrcLfo1, kDstWtPos, 0.6f);
        P.space = 0.5f;
    });

    add ("Bass + Marimba Split", [] (Patch& P)
    {
        auto& bass = P.layers[0];
        bass.type = kLayerWavetable; bass.wtTable = 6; bass.wtPos = 0.25f; bass.wtSub = 0.4f;
        bass.keyHigh = 54; bass.attack = 0.003f; bass.release = 0.15f;
        bass.cutoff = 1500.0f; bass.resonance = 0.3f; bass.drive = 0.3f;

        auto& mallet = P.layers[1];
        mallet.enabled = 1; mallet.type = kLayerPhysical; mallet.keyLow = 55;
        mallet.shape = 0.5f; mallet.material = 0.95f; mallet.hardness = 0.45f; mallet.position = 0.22f;
        mallet.decay = 1.2f;
        P.space = 0.25f;
    });

    add ("Velocity Morph", [] (Patch& P)
    {
        // Soft hits play wood, hard hits play a metal bell.
        auto& wood = P.layers[0];
        wood.shape = 0.5f; wood.material = 0.33f; wood.velHigh = 0.65f;

        auto& bell = P.layers[1];
        bell.enabled = 1; bell.type = kLayerPhysical; bell.velLow = 0.65f;
        bell.shape = 1.0f; bell.material = 0.75f; bell.decay = 2.0f; bell.hardness = 0.7f;
        P.space = 0.35f;
    });

    add ("Impossible Stack", [] (Patch& P)
    {
        auto& star = P.layers[0];
        star.shape = 1.0f; star.material = 0.8f; star.size = 1.0f; star.decay = 4.0f; star.drift = -0.6f;
        star.evolve = 3.0f; star.bloom = 0.4f; star.volume = -3.0f;
        star.mod (0, kSrcMacro1, kDstFracture, 0.8f);

        auto& pad = P.layers[1];
        pad.enabled = 1; pad.type = kLayerWavetable; pad.wtTable = 13; pad.wtPos = 0.6f;
        pad.wtUnison = 3; pad.wtDetune = 0.2f; pad.attack = 1.5f; pad.release = 3.0f; pad.volume = -5.0f;
        pad.mod (0, kSrcMacro1, kDstWtPos, -0.6f);

        auto& rain = P.layers[2];
        rain.enabled = 1; rain.type = kLayerPhysical; rain.exciter = kPluck; rain.shape = 0.75f;
        rain.material = 1.0f; rain.hardness = 0.8f; rain.decay = 0.6f; rain.octave = 3; rain.volume = -8.0f;
        rain.lfoShape[0] = kLfoSampleHold; rain.lfoSync[0] = 7; rain.mod (0, kSrcLfo1, kDstPosition, 0.4f);
        rain.mod (1, kSrcMacro2, kDstVolume, -1.0f);

        auto& sub = P.layers[3];
        sub.enabled = 1; sub.type = kLayerWavetable; sub.wtTable = 0; sub.wtPos = 0.0f; sub.octave = 1;
        sub.attack = 0.3f; sub.release = 2.0f; sub.volume = -8.0f;
        P.space = 0.6f; P.room = 0.9f;
    });

    add ("Rubber Wavetable", [] (Patch& P)
    {
        auto& p = P.layers[0];
        p.exciter = kExcWavetable; p.wtTable = 2; p.shape = 0.0f; p.material = 0.15f; p.body = 0.6f;
        p.attack = 0.01f; p.release = 0.3f; p.envDecay = 0.5f;
        p.mod (0, kSrcEnv, kDstWtPos, 0.6f);
        p.cutoff = 6000.0f; P.space = 0.2f;
    });

    return list;
}

namespace detail
{
inline void randomPhysical (LayerParams& p, Rng& r)
{
    auto u = [&r] { return r.uniform(); };
    auto chance = [&r] (float c) { return r.uniform() < c; };
    auto logRange = [&r] (float lo, float hi) { return lo * std::pow (hi / lo, r.uniform()); };

    p.type = kLayerPhysical;
    const float e = u();
    p.exciter = e < 0.3f ? kStrike : e < 0.5f ? kPluck : e < 0.62f ? kBow : e < 0.72f ? kBreath : e < 0.86f ? kOsc : kExcWavetable;
    const bool continuous = p.exciter == kBow || p.exciter == kBreath || p.exciter == kOsc || p.exciter == kExcWavetable;

    p.hardness = 0.2f + 0.7f * u();
    p.position = 0.05f + 0.4f * u();
    p.shape = u();
    p.material = u();
    p.size = 0.2f + 0.75f * u();
    p.decay = logRange (0.4f, 4.0f);
    p.release = logRange (0.1f, 3.0f);
    p.attack = continuous ? logRange (0.005f, 1.5f) : 0.04f;

    if (p.exciter == kOsc)
    {
        p.oscWave = u();
        p.unison = chance (0.5f) ? 0 : (int) (u() * 7.0f) % 7;
        p.detune = 0.1f + 0.4f * u();
    }
    if (p.exciter == kExcWavetable)
    {
        p.wtTable = (int) (u() * kNumBuiltInWavetables) % kNumBuiltInWavetables;
        p.wtPos = u();
    }
    if (p.exciter == kOsc || p.exciter == kExcWavetable)
        p.body = 0.5f + 0.5f * u();

    if (chance (0.5f)) p.bloom = 0.8f * u();
    if (chance (0.5f)) p.drift = 0.6f * r.bipolar();
    if (chance (0.5f)) p.transmute = 0.8f * r.bipolar();
    if (chance (0.4f)) p.fracture = 0.7f * u();
    p.evolve = logRange (0.3f, 4.0f);
}

inline void randomWavetable (LayerParams& p, Rng& r)
{
    auto u = [&r] { return r.uniform(); };
    auto chance = [&r] (float c) { return r.uniform() < c; };
    auto logRange = [&r] (float lo, float hi) { return lo * std::pow (hi / lo, r.uniform()); };

    p.type = kLayerWavetable;
    p.wtTable = (int) (u() * kNumBuiltInWavetables) % kNumBuiltInWavetables;
    p.wtPos = u();
    if (chance (0.4f))
    {
        p.wtWarpMode = 1 + (int) (u() * (kNumWarpModes - 1)) % (kNumWarpModes - 1);
        p.wtWarp = 0.6f * u();
    }
    p.wtUnison = chance (0.4f) ? 0 : (int) (u() * 7.0f) % 7;
    p.wtDetune = 0.1f + 0.35f * u();
    p.wtSpread = 0.4f + 0.6f * u();
    if (chance (0.25f)) p.wtSub = 0.6f * u();

    if (chance (0.4f))
    {
        // Plucky
        p.attack = logRange (0.001f, 0.02f);
        p.ampDecay = logRange (0.1f, 1.5f);
        p.ampSustain = 0.3f * u();
    }
    else
    {
        p.attack = logRange (0.005f, 1.5f);
        p.ampSustain = 0.6f + 0.4f * u();
    }
    p.release = logRange (0.1f, 3.0f);
}

inline void randomCommon (LayerParams& p, Rng& r, bool allowWtTargets)
{
    auto u = [&r] { return r.uniform(); };
    auto chance = [&r] (float c) { return r.uniform() < c; };
    auto logRange = [&r] (float lo, float hi) { return lo * std::pow (hi / lo, r.uniform()); };

    if (chance (0.6f))
    {
        p.cutoff = logRange (600.0f, 16000.0f);
        p.resonance = 0.6f * u();
        p.filterEnv = 0.6f * r.bipolar();
        p.envAttack = logRange (0.001f, 0.5f);
        p.envDecay = logRange (0.1f, 2.0f);
        p.envSustain = u();
        p.envRelease = logRange (0.1f, 2.0f);
    }
    if (chance (0.4f)) p.drive = 0.6f * u();

    const float o = u();
    p.octave = o < 0.2f ? 1 : o < 0.3f ? 3 : 2;

    // A few random modulation routings.
    static const int sources[] = { kSrcLfo1, kSrcLfo2, kSrcEnv, kSrcVelocity, kSrcRandom, kSrcMacro1 };
    static const int physicalTargets[] = { kDstShape, kDstMaterial, kDstPosition, kDstHardness, kDstCutoff,
                                           kDstFracture, kDstDrift, kDstBody, kDstPitch };
    static const int wavetableTargets[] = { kDstWtPos, kDstWtPos, kDstWtWarp, kDstCutoff, kDstDetune, kDstPan, kDstPitch };
    const int numMods = (int) (u() * 4.0f);
    for (int s = 0; s < numMods; ++s)
    {
        const int src = sources[(int) (u() * 6.0f) % 6];
        const int dst = allowWtTargets ? wavetableTargets[(int) (u() * 7.0f) % 7]
                                       : physicalTargets[(int) (u() * 9.0f) % 9];
        float amount = (0.1f + 0.4f * u()) * (chance (0.5f) ? 1.0f : -1.0f);
        if (dst == kDstPitch)
            amount *= 0.05f;
        p.mod (s, src, dst, amount);
    }
    for (int l = 0; l < kNumLfos; ++l)
    {
        p.lfoShape[(size_t) l] = (int) (u() * kNumLfoShapes) % kNumLfoShapes;
        p.lfoRate[(size_t) l] = logRange (0.1f, 8.0f);
        p.lfoSync[(size_t) l] = chance (0.3f) ? 3 + (int) (u() * 6.0f) % 6 : 0;
    }
}
} // namespace detail

// A random but usually musical patch, for the Randomize button: one main
// layer, often with a second (and sometimes third) layer of the other kind.
inline Patch randomPatch (Rng& r)
{
    Patch patch;
    const bool mainIsPhysical = r.uniform() < 0.6f;
    int count = 1;
    if (r.uniform() < 0.55f) count = 2;
    if (r.uniform() < 0.2f) count = 3;

    for (int i = 0; i < count; ++i)
    {
        auto& p = patch.layers[(size_t) i];
        p = LayerParams();
        p.enabled = 1;
        const bool physical = (i % 2 == 0) == mainIsPhysical;
        if (physical)
            detail::randomPhysical (p, r);
        else
            detail::randomWavetable (p, r);
        detail::randomCommon (p, r, ! physical);
        if (i > 0)
            p.volume = -3.0f - 6.0f * r.uniform();
    }
    for (int i = count; i < kNumLayers; ++i)
        patch.layers[(size_t) i].enabled = 0;

    patch.space = 0.15f + 0.45f * r.uniform();
    patch.room = 0.3f + 0.65f * r.uniform();
    return patch;
}

} // namespace impossible
