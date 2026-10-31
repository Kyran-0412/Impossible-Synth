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
    Params params;
};

inline std::vector<Preset> factoryPresets()
{
    std::vector<Preset> list;
    auto add = [&list] (const char* name, auto&& setup)
    {
        Params p;
        setup (p);
        list.push_back ({ name, p });
    };

    // ---- Keys & mallets --------------------------------------------------
    add ("Init", [] (Params&) {});

    add ("Glass Marimba", [] (Params& p)
    {
        p.shape = 0.5f; p.material = 0.95f; p.hardness = 0.45f; p.position = 0.22f;
        p.decay = 1.2f; p.space = 0.3f;
    });

    add ("Rubber Bell", [] (Params& p)
    {
        p.shape = 1.0f; p.material = 0.08f; p.hardness = 0.7f; p.decay = 4.0f; p.space = 0.35f;
    });

    add ("Melting Vibes", [] (Params& p)
    {
        p.shape = 0.5f; p.material = 0.7f; p.transmute = -0.7f; p.evolve = 2.5f; p.decay = 2.0f;
        p.hardness = 0.4f; p.space = 0.35f;
        p.lfoShape[0] = kLfoSine; p.lfoRate[0] = 5.0f;
        p.mod (0, kSrcLfo1, kDstVolume, 0.15f);
    });

    add ("Planet Bell", [] (Params& p)
    {
        p.shape = 1.0f; p.material = 0.7f; p.size = 1.0f; p.decay = 3.0f; p.bloom = 0.4f;
        p.octave = 1; p.space = 0.5f; p.room = 0.9f;
    });

    add ("Cracked Kalimba", [] (Params& p)
    {
        p.shape = 0.55f; p.material = 0.45f; p.fracture = 0.6f; p.evolve = 0.6f; p.decay = 0.8f;
        p.position = 0.12f; p.hardness = 0.6f; p.space = 0.3f;
    });

    // ---- Plucks ----------------------------------------------------------
    add ("Wire Harp", [] (Params& p)
    {
        p.exciter = kPluck; p.shape = 0.0f; p.material = 0.75f; p.position = 0.12f; p.decay = 1.5f;
        p.space = 0.35f;
    });

    add ("Rubber Band Pluck", [] (Params& p)
    {
        p.exciter = kPluck; p.shape = 0.0f; p.material = 0.1f; p.decay = 3.0f; p.hardness = 0.7f;
        p.drift = 0.25f; p.evolve = 0.3f; p.space = 0.2f;
    });

    add ("Bent Koto", [] (Params& p)
    {
        p.exciter = kPluck; p.shape = 0.08f; p.material = 0.45f; p.position = 0.1f;
        p.drift = -0.15f; p.evolve = 0.6f; p.space = 0.3f;
    });

    add ("Glass Rain", [] (Params& p)
    {
        p.exciter = kPluck; p.shape = 0.75f; p.material = 1.0f; p.hardness = 0.8f; p.decay = 0.6f;
        p.space = 0.6f; p.room = 0.85f;
        p.lfoShape[0] = kLfoSampleHold; p.lfoSync[0] = 7; // 1/16
        p.lfoShape[1] = kLfoDrift; p.lfoRate[1] = 0.3f;
        p.mod (0, kSrcLfo1, kDstPosition, 0.4f);
        p.mod (1, kSrcLfo2, kDstShape, 0.2f);
    });

    // ---- Pads ------------------------------------------------------------
    add ("Bowed Glass", [] (Params& p)
    {
        p.exciter = kBow; p.shape = 0.1f; p.material = 1.0f; p.hardness = 0.6f;
        p.attack = 0.6f; p.release = 2.0f; p.space = 0.5f;
    });

    add ("Singing Bowl Pad", [] (Params& p)
    {
        p.exciter = kBow; p.shape = 1.0f; p.material = 0.7f; p.hardness = 0.5f;
        p.attack = 1.0f; p.release = 3.0f; p.bloom = 0.3f; p.space = 0.6f; p.room = 0.85f;
        p.lfoShape[0] = kLfoDrift; p.lfoRate[0] = 0.2f;
        p.mod (0, kSrcLfo1, kDstMaterial, 0.1f);
    });

    add ("Breathing Tubes", [] (Params& p)
    {
        p.exciter = kBreath; p.shape = 0.25f; p.material = 0.8f; p.hardness = 0.35f;
        p.attack = 0.8f; p.release = 2.0f; p.space = 0.55f;
        p.lfoShape[0] = kLfoSine; p.lfoRate[0] = 0.15f;
        p.mod (0, kSrcLfo1, kDstShape, 0.15f);
    });

    add ("Supersaw Cathedral", [] (Params& p)
    {
        p.exciter = kOsc; p.oscWave = 0.66f; p.unison = 6; p.detune = 0.35f;
        p.shape = 1.0f; p.material = 0.75f; p.body = 0.6f;
        p.attack = 0.5f; p.release = 2.5f; p.cutoff = 6000.0f; p.space = 0.6f; p.room = 0.9f; p.output = 3.0f;
    });

    add ("Frozen Choir", [] (Params& p)
    {
        p.exciter = kOsc; p.oscWave = 0.4f; p.unison = 4; p.detune = 0.2f;
        p.shape = 0.25f; p.material = 0.9f; p.body = 0.85f;
        p.attack = 1.2f; p.release = 3.0f; p.space = 0.7f; p.room = 0.9f;
        p.lfoShape[0] = kLfoDrift; p.lfoRate[0] = 0.3f;
        p.mod (0, kSrcLfo1, kDstShape, 0.1f);
    });

    // ---- Bass ------------------------------------------------------------
    add ("Rubber Sub", [] (Params& p)
    {
        p.exciter = kOsc; p.oscWave = 0.2f; p.shape = 0.0f; p.material = 0.15f; p.body = 0.4f;
        p.octave = 1; p.attack = 0.005f; p.release = 0.2f;
        p.cutoff = 900.0f; p.filterEnv = 0.3f; p.envDecay = 0.3f; p.drive = 0.25f; p.space = 0.05f;
    });

    add ("Wooden Growl", [] (Params& p)
    {
        p.exciter = kOsc; p.oscWave = 0.66f; p.unison = 2; p.detune = 0.15f;
        p.shape = 0.0f; p.material = 0.35f; p.body = 0.7f; p.octave = 1;
        p.attack = 0.003f; p.release = 0.15f;
        p.cutoff = 400.0f; p.resonance = 0.45f; p.filterEnv = 0.55f;
        p.envDecay = 0.35f; p.envSustain = 0.2f; p.drive = 0.5f; p.space = 0.05f;
    });

    add ("Iron Pluck Bass", [] (Params& p)
    {
        p.exciter = kPluck; p.shape = 0.0f; p.material = 0.66f; p.octave = 1; p.hardness = 0.7f;
        p.decay = 0.6f; p.release = 0.3f; p.drive = 0.6f;
        p.cutoff = 1200.0f; p.resonance = 0.3f; p.filterEnv = 0.4f; p.envDecay = 0.25f; p.space = 0.05f;
    });

    // ---- Leads -----------------------------------------------------------
    add ("Glass Lead", [] (Params& p)
    {
        p.exciter = kOsc; p.oscWave = 0.85f; p.unison = 1; p.detune = 0.1f;
        p.shape = 0.0f; p.material = 1.0f; p.body = 0.7f; p.attack = 0.01f; p.release = 0.3f;
        p.cutoff = 5000.0f; p.resonance = 0.2f; p.drive = 0.2f; p.space = 0.3f; p.output = 5.0f;
        p.lfoShape[0] = kLfoSine; p.lfoRate[0] = 5.5f;
        p.mod (0, kSrcLfo1, kDstPitch, 0.015f);
    });

    add ("Fractured Lead", [] (Params& p)
    {
        p.exciter = kOsc; p.oscWave = 0.66f; p.shape = 0.75f; p.material = 0.7f; p.body = 0.8f;
        p.fracture = 0.7f; p.evolve = 0.5f; p.drive = 0.35f; p.cutoff = 4000.0f;
        p.attack = 0.01f; p.release = 0.3f; p.space = 0.3f; p.output = 6.0f;
    });

    // ---- Textures & FX ---------------------------------------------------
    add ("Impossible Object", [] (Params& p)
    {
        p.shape = 0.85f; p.material = 0.5f; p.bloom = 0.7f; p.drift = 0.5f; p.transmute = 0.5f;
        p.fracture = 0.4f; p.evolve = 2.5f; p.decay = 3.0f; p.space = 0.6f;
    });

    add ("Collapsing Star", [] (Params& p)
    {
        p.shape = 1.0f; p.material = 0.8f; p.size = 1.0f; p.decay = 5.0f; p.drift = -1.0f;
        p.evolve = 4.0f; p.bloom = 0.5f; p.octave = 1; p.space = 0.7f; p.room = 0.95f;
    });

    add ("Clockwork Insects", [] (Params& p)
    {
        p.exciter = kPluck; p.shape = 0.7f; p.material = 0.75f; p.hardness = 0.9f; p.octave = 3;
        p.decay = 0.3f; p.space = 0.4f;
        p.lfoShape[0] = kLfoSampleHold; p.lfoSync[0] = 7; // 1/16
        p.lfoShape[1] = kLfoSquare; p.lfoSync[1] = 6;     // 1/8
        p.mod (0, kSrcLfo1, kDstShape, 0.5f);
        p.mod (1, kSrcLfo2, kDstPosition, 0.3f);
    });

    add ("Tidal Drone", [] (Params& p)
    {
        p.exciter = kBreath; p.shape = 0.9f; p.material = 0.85f; p.size = 0.95f; p.hardness = 0.5f;
        p.attack = 3.0f; p.release = 5.0f; p.bloom = 0.3f; p.cutoff = 3000.0f;
        p.space = 0.75f; p.room = 0.95f;
        p.lfoShape[0] = kLfoDrift; p.lfoRate[0] = 0.1f;
        p.lfoShape[1] = kLfoSine; p.lfoRate[1] = 0.07f;
        p.mod (0, kSrcLfo1, kDstMaterial, 0.3f);
        p.mod (1, kSrcLfo2, kDstCutoff, 0.3f);
    });

    return list;
}

// A random but usually musical patch, for the Randomize button.
inline Params randomPatch (Rng& r)
{
    Params p;
    auto u = [&r] { return r.uniform(); };
    auto chance = [&r] (float c) { return r.uniform() < c; };
    auto logRange = [&r] (float lo, float hi) { return lo * std::pow (hi / lo, r.uniform()); };

    const float e = u();
    p.exciter = e < 0.3f ? kStrike : e < 0.5f ? kPluck : e < 0.65f ? kBow : e < 0.75f ? kBreath : kOsc;
    const bool continuous = p.exciter == kBow || p.exciter == kBreath || p.exciter == kOsc;

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
        p.body = 0.5f + 0.5f * u();
    }

    if (chance (0.5f)) p.bloom = 0.8f * u();
    if (chance (0.5f)) p.drift = 0.6f * r.bipolar();
    if (chance (0.5f)) p.transmute = 0.8f * r.bipolar();
    if (chance (0.4f)) p.fracture = 0.7f * u();
    p.evolve = logRange (0.3f, 4.0f);

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
    static const int sources[] = { kSrcLfo1, kSrcLfo2, kSrcEnv, kSrcVelocity, kSrcRandom };
    static const int targets[] = { kDstShape, kDstMaterial, kDstPosition, kDstHardness, kDstCutoff,
                                   kDstFracture, kDstDrift, kDstBody, kDstPitch };
    const int numMods = (int) (u() * 4.0f);
    for (int s = 0; s < numMods; ++s)
    {
        const int src = sources[(int) (u() * 5.0f) % 5];
        const int dst = targets[(int) (u() * 9.0f) % 9];
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

    p.space = 0.15f + 0.45f * u();
    p.room = 0.3f + 0.65f * u();
    return p;
}

} // namespace impossible
