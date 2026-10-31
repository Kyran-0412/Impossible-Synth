#include "Engine.h"

#include <algorithm>
#include <cmath>

namespace impossible
{

namespace
{
    constexpr float kPi = 3.14159265358979f;
    constexpr float kTwoPi = 6.28318530717959f;
    constexpr float kLn1000 = 6.90775527898f; // T60 = time to fall by 60 dB

    // Level trims, tuned with tools/render_test.cpp.
    constexpr float kVoiceGain = 1.0f;
    constexpr float kImpulseGain = 1.0f;
    constexpr float kTonalGain = 1.2f;
    constexpr float kNoiseGain = 1.0f;
    constexpr float kDryGain = 0.35f;
    constexpr float kWavetableGain = 0.3f;
    constexpr float kAgcLevel = 0.3f;  // object level relative to its input, for sustained exciters
    constexpr float kAgcMaxGain = 16.0f;

    float polyBlep (float t, float dt)
    {
        if (t < dt)
        {
            t /= dt;
            return t + t - t * t - 1.0f;
        }
        if (t > 1.0f - dt)
        {
            t = (t - 1.0f) / dt;
            return t * t + t + t + 1.0f;
        }
        return 0.0f;
    }

    bool isContinuous (int exciter)
    {
        return exciter == kBow || exciter == kBreath || exciter == kOsc || exciter == kExcWavetable;
    }

    float hashBipolar (uint32_t x)
    {
        x ^= x >> 16;
        x *= 0x7feb352du;
        x ^= x >> 15;
        x *= 0x846ca68bu;
        x ^= x >> 16;
        return (float) (x >> 8) * (2.0f / 16777216.0f) - 1.0f;
    }

    float lfoValue (int shape, double phase, uint32_t seed)
    {
        const double whole = std::floor (phase);
        const float f = (float) (phase - whole);
        const auto cycle = (uint32_t) (int64_t) whole;

        switch (shape)
        {
            case kLfoSine:     return std::sin (kTwoPi * f);
            case kLfoTriangle: return 1.0f - 4.0f * std::fabs (f - 0.5f);
            case kLfoSawUp:    return 2.0f * f - 1.0f;
            case kLfoSawDown:  return 1.0f - 2.0f * f;
            case kLfoSquare:   return f < 0.5f ? 1.0f : -1.0f;
            case kLfoSampleHold: return hashBipolar (cycle * 0x9E3779B1u ^ seed);
            case kLfoDrift:
            {
                const float a = hashBipolar (cycle * 0x9E3779B1u ^ seed);
                const float b = hashBipolar ((cycle + 1u) * 0x9E3779B1u ^ seed);
                const float t = f * f * (3.0f - 2.0f * f);
                return a + (b - a) * t;
            }
            default: return 0.0f;
        }
    }

    // Oscillator morphing Sine → Triangle → Saw → Square (the Physical "Osc" exciter).
    float oscSample (float phase, float dt, float wave)
    {
        auto shapeAt = [phase, dt] (int i)
        {
            switch (i)
            {
                case 0: return std::sin (kTwoPi * phase);
                case 1: return 1.0f - 4.0f * std::fabs (phase - 0.5f);
                case 2: return 2.0f * phase - 1.0f - polyBlep (phase, dt);
                default:
                {
                    float p2 = phase + 0.5f;
                    if (p2 >= 1.0f)
                        p2 -= 1.0f;
                    return (phase < 0.5f ? 1.0f : -1.0f) + polyBlep (phase, dt) - polyBlep (p2, dt);
                }
            }
        };

        const float pos = std::clamp (wave, 0.0f, 1.0f) * 3.0f;
        const int i = std::min ((int) pos, 2);
        const float frac = pos - (float) i;
        if (frac < 0.001f)
            return shapeAt (i);
        if (frac > 0.999f)
            return shapeAt (i + 1);
        return shapeAt (i) * (1.0f - frac) + shapeAt (i + 1) * frac;
    }

    float wrap01 (float x)
    {
        x -= std::floor (x);
        return x >= 1.0f ? 0.0f : x;
    }

    // Wavetable warps that bend the read position.
    float warpPhase (int mode, float a, float x)
    {
        switch (mode)
        {
            case kWarpBend:
            {
                const float e = 1.0f + 3.0f * a;
                return x < 0.5f ? 0.5f * std::pow (2.0f * x, e) : 1.0f - 0.5f * std::pow (2.0f - 2.0f * x, e);
            }
            case kWarpSync:    return wrap01 (x * (1.0f + 7.0f * a));
            case kWarpMirror:  return x + ((x < 0.5f ? x : 1.0f - x) - x) * a;
            case kWarpSqueeze:
            {
                const float w = 0.5f - 0.45f * a;
                return x < w ? 0.5f * x / w : 0.5f + 0.5f * (x - w) / (1.0f - w);
            }
            case kWarpFM:      return wrap01 (x + a * 0.3f * std::sin (kTwoPi * 2.0f * x));
            default:           return x;
        }
    }

    // Wavetable warps that shape the output.
    float warpOutput (int mode, float a, float s)
    {
        if (mode == kWarpFold)
            return std::sin (kPi * 0.5f * s * (1.0f + 4.0f * a));
        if (mode == kWarpCrush)
        {
            const float levels = std::exp2 (8.0f - 7.0f * a);
            return std::round (s * levels) / levels;
        }
        return s;
    }
} // namespace

//==============================================================================
void Voice::prepare (double sampleRate)
{
    sr = sampleRate;
    invSr = (float) (1.0 / sampleRate);
    fadeStep = 1.0f / (0.006f * (float) sr); // 6 ms fade when a voice is stolen
    agcCoef = 1.0f - std::exp (-(float) kControlInterval / (0.12f * (float) sr));
    agcFastCoef = 1.0f - std::exp (-(float) kControlInterval / (0.008f * (float) sr));
    kill();
}

void Voice::start (int newNote, float vel, const LayerParams& p, const ModContext& ctx, uint32_t seed)
{
    note = newNote;
    velocity = vel;
    active = true;
    keyDown = true;
    sustained = false;
    fading = false;
    fadeGain = 1.0f;
    sampleCount = 0;
    sawPhase = 0.0f;
    envStage = 0;
    envValue = 0.0f;
    ampStage = 0;
    ampEnv = 0.0f;
    lfoPhase.fill (0.0);
    ic1.fill (0.0f);
    ic2.fill (0.0f);
    noteSeed = seed | 1u;
    agcIn = agcOut = blockIn = blockOut = 0.0f;
    agcGain = 1.0f;
    subPhase = 0.0f;

    rng.seed (seed);
    noteRandom = rng.bipolar();
    for (int k = 0; k < kNumModes; ++k)
    {
        rndIrreg[(size_t) k] = rng.bipolar();
        rndFrac[(size_t) k] = rng.bipolar();
        rndSplit[(size_t) k] = 0.3f + 0.7f * rng.uniform();
        rndTau[(size_t) k] = rng.uniform();
    }

    // Spread partials across the stereo field; keep the lowest ones centred.
    for (int j = 0; j < kNumRes; ++j)
    {
        const float spread = 0.8f * std::min (1.0f, (float) (j / 2) / 3.0f);
        const float angle = (spread * rng.bipolar() + 1.0f) * kPi * 0.25f;
        panL[(size_t) j] = std::cos (angle);
        panR[(size_t) j] = std::sin (angle);
    }

    // Unison oscillators: random start phases (except a lone oscillator).
    const int count = p.unison + 1;
    for (int u = 0; u < kMaxUnison; ++u)
    {
        oscPhase[(size_t) u] = count > 1 ? rng.uniform() : 0.0f;
        const float pan = count > 1 ? (2.0f * (float) u / (float) (count - 1) - 1.0f) * 0.8f : 0.0f;
        const float angle = (pan + 1.0f) * kPi * 0.25f;
        oscPanL[(size_t) u] = std::cos (angle);
        oscPanR[(size_t) u] = std::sin (angle);
    }
    const int wtVoices = p.wtUnison + 1;
    for (int u = 0; u < kMaxUnison; ++u)
        wtPhase[(size_t) u] = wtVoices > 1 ? rng.uniform() : 0.0f;

    zr.fill (0.0f);
    zi.fill (0.0f);
    gain.fill (1.0f - 0.9f * p.bloom);

    updateControl (p, ctx, 0);
    gainL = gainLTarget;
    gainR = gainRTarget;
    gainLStep = gainRStep = 0.0f;
    wtFrame = wtFrameTarget;
    wtFrameStep = 0.0f;
    controlCounter = kControlInterval;

    if (type == kLayerPhysical && ! isContinuous (p.exciter))
        excite (vel);
}

void Voice::retrigger (float vel, const LayerParams& p, const ModContext& ctx)
{
    // Hitting an object that is still ringing: keep the ringing, add energy.
    velocity = vel;
    keyDown = true;
    sustained = false;
    sampleCount = 0;
    envStage = 0;
    ampStage = 0;
    for (int l = 0; l < kNumLfos; ++l)
        if (p.lfoRetrig[(size_t) l] != 0)
            lfoPhase[(size_t) l] = 0.0;

    updateControl (p, ctx, 0);
    controlCounter = kControlInterval;

    if (type == kLayerPhysical && ! isContinuous (p.exciter))
        excite (vel);
}

void Voice::excite (float vel)
{
    for (int j = 0; j < numLive; ++j)
        zr[(size_t) j] += resAmp[(size_t) j] * vel * kImpulseGain;
}

void Voice::kill()
{
    active = false;
    keyDown = false;
    sustained = false;
    fading = false;
    note = -1;
    level = 0.0f;
    numLive = 0;
    zr.fill (0.0f);
    zi.fill (0.0f);
}

LayerParams Voice::applyModulation (const LayerParams& p, const ModContext& ctx, int sampleInCall)
{
    std::array<float, kNumSources> src {};
    for (int l = 0; l < kNumLfos; ++l)
    {
        const bool retrig = p.lfoRetrig[(size_t) l] != 0;
        const double phase = retrig ? lfoPhase[(size_t) l]
                                    : ctx.lfoPhase[(size_t) l] + (double) ctx.lfoHz[(size_t) l] * sampleInCall / sr;
        const uint32_t seed = retrig ? noteSeed * (uint32_t) (l + 3) : 0xA5A5u + (uint32_t) l;
        src[(size_t) (kSrcLfo1 + l)] = lfoValue (p.lfoShape[(size_t) l], phase, seed);
    }
    src[kSrcEnv] = envValue;
    src[kSrcVelocity] = velocity;
    src[kSrcModWheel] = ctx.modWheel;
    src[kSrcNote] = std::clamp (((float) note - 60.0f) / 48.0f, -1.0f, 1.0f);
    src[kSrcRandom] = noteRandom;
    for (int i = 0; i < kNumMacros; ++i)
        src[(size_t) (kSrcMacro1 + i)] = ctx.macros[(size_t) i];

    std::array<float, kNumTargets> sum {};
    std::array<bool, kNumTargets> used {};
    for (int s = 0; s < kNumModSlots; ++s)
    {
        const int source = p.modSource[(size_t) s];
        const int target = p.modTarget[(size_t) s];
        if (source <= kSrcOff || source >= kNumSources || target <= kDstOff || target >= kNumTargets)
            continue;
        sum[(size_t) target] += p.modAmount[(size_t) s] * src[(size_t) source];
        used[(size_t) target] = true;
    }

    LayerParams m = p;
    pitchMod = sum[kDstPitch] * 12.0f;

    auto apply = [&] (float& field, const Range& range, int target)
    {
        if (used[(size_t) target])
            field = range.fromNorm (range.toNorm (field) + sum[(size_t) target]);
    };
    apply (m.hardness, ranges::hardness, kDstHardness);
    apply (m.position, ranges::position, kDstPosition);
    apply (m.shape, ranges::shape, kDstShape);
    apply (m.material, ranges::material, kDstMaterial);
    apply (m.size, ranges::size, kDstSize);
    apply (m.decay, ranges::decay, kDstDecay);
    apply (m.bloom, ranges::bloom, kDstBloom);
    apply (m.drift, ranges::drift, kDstDrift);
    apply (m.transmute, ranges::transmute, kDstTransmute);
    apply (m.fracture, ranges::fracture, kDstFracture);
    apply (m.oscWave, ranges::oscWave, kDstOscWave);
    apply (m.detune, ranges::detune, kDstDetune);
    apply (m.wtDetune, ranges::wtDetune, kDstDetune);
    apply (m.body, ranges::body, kDstBody);
    apply (m.cutoff, ranges::cutoff, kDstCutoff);
    apply (m.resonance, ranges::resonance, kDstResonance);
    apply (m.drive, ranges::drive, kDstDrive);
    apply (m.wtPos, ranges::wtPos, kDstWtPos);
    apply (m.wtWarp, ranges::wtWarp, kDstWtWarp);
    apply (m.pan, ranges::pan, kDstPan);

    // Volume: the layer level, scaled by modulation (0x .. 2x).
    m.volume = std::pow (10.0f, m.volume / 20.0f) * std::clamp (1.0f + sum[kDstVolume], 0.0f, 2.0f);
    return m;
}

void Voice::updateWavetable (const LayerParams& m, const ModContext& ctx, float f0)
{
    const Wavetable* table = ctx.table;
    if (table == nullptr)
    {
        wtGain = 0.0f;
        return;
    }

    const float fTable = f0 / (float) table->pitchDivisor();
    wtCount = std::clamp (m.wtUnison + 1, 1, kMaxUnison);
    wtWarpMode = m.wtWarpMode;
    wtWarp = std::clamp (m.wtWarp, 0.0f, 1.0f);

    float maxInc = 0.0f;
    for (int u = 0; u < wtCount; ++u)
    {
        const float spread = wtCount > 1 ? 2.0f * (float) u / (float) (wtCount - 1) - 1.0f : 0.0f;
        const float cents = spread * m.wtDetune * 60.0f;
        wtInc[(size_t) u] = std::min (0.45f, fTable * std::exp2 (cents / 1200.0f) * invSr);
        maxInc = std::max (maxInc, wtInc[(size_t) u]);

        const float angle = (spread * std::clamp (m.wtSpread, 0.0f, 1.0f) + 1.0f) * kPi * 0.25f;
        wtPanL[(size_t) u] = std::cos (angle);
        wtPanR[(size_t) u] = std::sin (angle);
    }

    // Pick the mip level from how many harmonics fit below Nyquist (warps that
    // speed up the read position need fewer).
    float speedUp = 1.0f;
    if (wtWarpMode == kWarpSync) speedUp = 1.0f + 7.0f * wtWarp;
    if (wtWarpMode == kWarpFM || wtWarpMode == kWarpBend) speedUp = 1.0f + 2.0f * wtWarp;
    wtLevel = Wavetable::levelFor (0.45f / std::max (1.0e-6f, maxInc * speedUp));

    wtFrameTarget = std::clamp (m.wtPos, 0.0f, 1.0f) * (float) (table->numFrames() - 1);
    wtFrameStep = (wtFrameTarget - wtFrame) / (float) kControlInterval;

    subInc = std::min (0.45f, f0 * 0.5f * invSr);
    subLevel = m.wtSub;
    wtGain = 1.0f / std::sqrt ((float) wtCount);
}

void Voice::renderWavetable (float& outL, float& outR, float& mono, const ModContext& ctx)
{
    outL = outR = mono = 0.0f;
    const Wavetable* table = ctx.table;
    if (table == nullptr)
        return;

    wtFrame += wtFrameStep;
    for (int u = 0; u < wtCount; ++u)
    {
        const auto uu = (size_t) u;
        wtPhase[uu] += wtInc[uu];
        if (wtPhase[uu] >= 1.0f)
            wtPhase[uu] -= 1.0f;
        const float x = warpPhase (wtWarpMode, wtWarp, wtPhase[uu]);
        const float s = warpOutput (wtWarpMode, wtWarp, table->read (wtFrame, wtLevel, x));
        outL += s * wtPanL[uu];
        outR += s * wtPanR[uu];
        mono += s;
    }
    outL *= wtGain;
    outR *= wtGain;
    mono *= wtGain;

    if (subLevel > 0.001f)
    {
        subPhase += subInc;
        if (subPhase >= 1.0f)
            subPhase -= 1.0f;
        const float sub = std::sin (kTwoPi * subPhase) * subLevel;
        outL += sub * 0.7071f;
        outR += sub * 0.7071f;
        mono += sub;
    }
}

void Voice::updateControl (const LayerParams& p, const ModContext& ctx, int sampleInCall)
{
    const float dt = (float) kControlInterval * invSr;
    const bool released = ! keyDown && ! sustained;

    // Mod envelope.
    if (released && envStage < 2)
        envStage = 2;
    switch (envStage)
    {
        case 0:
            envValue += dt / std::max (0.001f, p.envAttack);
            if (envValue >= 1.0f)
            {
                envValue = 1.0f;
                envStage = 1;
            }
            break;
        case 1:
            envValue = p.envSustain + (envValue - p.envSustain) * std::exp (-dt * 3.0f / std::max (0.005f, p.envDecay));
            break;
        default:
            envValue *= std::exp (-dt * 3.0f / std::max (0.005f, p.envRelease));
            break;
    }

    const LayerParams m = applyModulation (p, ctx, sampleInCall);

    for (int l = 0; l < kNumLfos; ++l)
        lfoPhase[(size_t) l] += (double) ctx.lfoHz[(size_t) l] * dt;

    type = m.type;

    const float semis = (float) note - 69.0f + ctx.bend + pitchMod + 12.0f * (float) (m.octave - 2) + m.fine / 100.0f;
    const float f0 = 440.0f * std::exp2 (semis / 12.0f);

    // Amp envelope (attack aims past 1 so it rises at a steady-feeling rate).
    ampAttackCoef = 1.0f - std::exp (-1.0f / (std::max (0.0005f, m.attack / 1.79f) * (float) sr));
    ampDecayCoef = 1.0f - std::exp (-1.0f / (std::max (0.005f, m.ampDecay / 3.0f) * (float) sr));
    ampReleaseCoef = 1.0f - std::exp (-1.0f / (std::max (0.005f, m.release / 3.0f) * (float) sr));
    ampSustain = type == kLayerWavetable ? std::clamp (m.ampSustain, 0.0f, 1.0f) : 1.0f;

    if (type == kLayerWavetable)
    {
        updateWavetable (m, ctx, f0);
        numLive = 0;
    }
    else
    {
        sawInc = std::min (0.5f, f0 * invSr);

        // Oscillator exciter.
        oscCount = std::clamp (m.unison + 1, 1, kMaxUnison);
        oscWave = m.oscWave;
        for (int u = 0; u < oscCount; ++u)
        {
            const float spread = oscCount > 1 ? 2.0f * (float) u / (float) (oscCount - 1) - 1.0f : 0.0f;
            const float cents = spread * m.detune * 50.0f;
            oscInc[(size_t) u] = std::min (0.45f, f0 * std::exp2 (cents / 1200.0f) * invSr);
        }
        if (m.exciter == kExcWavetable)
            updateWavetable (m, ctx, f0);

        const float t = (float) sampleCount * invSr;
        const float evolveTime = std::max (0.02f, m.evolve);
        const float evo = 1.0f - std::exp (-t / evolveTime);

        const float material = std::clamp (m.material + m.transmute * evo, 0.0f, 1.0f);
        const MaterialProps mp = materialAt (material);

        const float sizeDecay = std::exp2 ((m.size - 0.5f) * 4.5f);
        const float noteDecay = std::clamp (std::pow (220.0f / f0, 0.35f), 0.35f, 3.0f);
        const float baseT60 = mp.t60 * m.decay * sizeDecay * noteDecay * (1.0f + 2.0f * m.bloom);

        // Brightness of the excitation (mallet hardness, pluck brightness, sustained tone).
        float cutoff, positionAmount;
        switch (m.exciter)
        {
            case kStrike: cutoff = 150.0f * std::exp2 (m.hardness * 6.5f + velocity * 1.5f); positionAmount = 0.8f; break;
            case kPluck:  cutoff = 600.0f * std::exp2 (m.hardness * 5.0f + velocity * 1.5f); positionAmount = 0.97f; break;
            default:      cutoff = 250.0f * std::exp2 (m.hardness * 6.0f); positionAmount = 0.5f; break;
        }

        const float tilt = mp.beta + (m.size - 0.5f) * 1.0f; // bigger objects: more low end
        const float irregularity = std::max (0.0f, (m.size - 0.5f) * 2.0f) * 0.006f;
        const float split = m.fracture * (0.25f + 0.75f * evo);
        const float nyquistLimit = 0.46f * (float) sr;
        const float bloomTarget = 1.0f + 3.0f * m.bloom;

        float ampSum = 0.0f;
        int lastLive = 0;
        std::array<float, kNumRes> resR {};

        for (int k = 0; k < kNumModes; ++k)
        {
            float ratio = shapeRatio (m.shape, k);
            ratio *= std::sqrt (1.0f + mp.stiff * (float) ((k + 1) * (k + 1)));

            if (k > 0)
            {
                ratio *= 1.0f + irregularity * rndIrreg[(size_t) k];
                ratio *= 1.0f + m.fracture * 0.06f * rndFrac[(size_t) k];
            }

            // Drift: upper partials bend more than the fundamental (up to ±1 octave).
            const float weight = 0.15f + 0.85f * std::clamp (std::log2 (std::max (ratio, 1.0f)) / 5.0f, 0.0f, 1.0f);
            ratio *= std::exp2 (m.drift * weight * evo);

            const float freq = f0 * ratio;

            float t60 = baseT60 / std::pow (std::max (ratio, 0.5f), mp.alpha);
            t60 = std::clamp (t60, 0.004f, 120.0f);
            if (released)
                t60 = std::min (t60, std::max (0.01f, m.release));
            const float r = std::exp (-kLn1000 / (t60 * (float) sr));

            float a = std::pow (std::max (ratio, 0.25f), -tilt);
            const float x = freq / cutoff;
            a /= std::sqrt (1.0f + x * x * x * x);
            const float hitShape = std::fabs (std::sin (kPi * (float) (k + 1) * m.position));
            a *= (1.0f - positionAmount) + positionAmount * hitShape;

            const float detune = split * 0.07f * rndSplit[(size_t) k];
            const float tau = evolveTime * (0.35f + 1.3f * rndTau[(size_t) k]);
            const float coef = 1.0f - std::exp (-(float) kControlInterval / (tau * (float) sr));

            for (int side = 0; side < 2; ++side)
            {
                const auto j = (size_t) (2 * k + side);
                const float fs = freq * (1.0f + (side == 0 ? 0.5f : -0.5f) * detune);

                if (fs > nyquistLimit || fs < 12.0f)
                {
                    pr[j] = pi[j] = 0.0f;
                    zr[j] = zi[j] = 0.0f;
                    resAmp[j] = inTonal[j] = inNoise[j] = 0.0f;
                    continue;
                }

                const float w = kTwoPi * fs * invSr;
                pr[j] = r * std::cos (w);
                pi[j] = r * std::sin (w);
                resR[j] = r;
                resAmp[j] = 0.5f * a;
                ampSum += 0.5f * a;
                lastLive = (int) j + 1;

                gainTarget[j] = bloomTarget;
                gainCoef[j] = coef;
            }
        }

        numLive = lastLive;
        const float norm = ampSum > 0.0f ? 1.0f / ampSum : 0.0f;

        for (int jj = 0; jj < numLive; ++jj)
        {
            const auto j = (size_t) jj;
            resAmp[j] *= norm;
            const float r = resR[j];
            inTonal[j] = resAmp[j] * (1.0f - r) * kTonalGain;
            inNoise[j] = resAmp[j] * std::sqrt (std::max (0.0f, 1.0f - r * r)) * kNoiseGain;
            gain[j] += (gainTarget[j] - gain[j]) * gainCoef[j]; // Bloom moves at control rate
        }
    }

    // Output stage: body mix, drive, filter, volume, pan.
    body = m.body;
    driveOn = m.drive > 0.001f;
    driveGain = 1.0f + 24.0f * m.drive * m.drive;
    driveMakeup = 0.25f / std::tanh (0.25f * driveGain); // keeps a moderate signal at the same level

    filterMode = m.filterMode;
    if (filterMode != kFilterOff)
    {
        const float fc = std::clamp (m.cutoff * std::exp2 (m.filterEnv * envValue * 6.0f), 20.0f, 0.45f * (float) sr);
        const float g = std::tan (kPi * fc * invSr);
        fK = 2.0f - 1.88f * std::clamp (m.resonance, 0.0f, 1.0f);
        fComp = std::pow (fK * 0.5f, 0.35f); // tame the resonant peak
        fA1 = 1.0f / (1.0f + g * (g + fK));
        fA2 = g * fA1;
        fA3 = g * fA2;
    }

    // m.volume is already linear (layer level x modulation). Equal-power pan, unity in the centre.
    const float angle = (std::clamp (m.pan, -1.0f, 1.0f) + 1.0f) * kPi * 0.25f;
    gainLTarget = m.volume * std::cos (angle) * 1.41421356f;
    gainRTarget = m.volume * std::sin (angle) * 1.41421356f;
    gainLStep = (gainLTarget - gainL) / (float) kControlInterval;
    gainRStep = (gainRTarget - gainR) / (float) kControlInterval;
}

void Voice::render (float* left, float* right, int numSamples, const LayerParams& p, const ModContext& ctx)
{
    if (! active)
        return;

    const float h = p.hardness;

    for (int i = 0; i < numSamples; ++i)
    {
        if (--controlCounter <= 0)
        {
            updateControl (p, ctx, i);
            controlCounter = kControlInterval;

            const bool continuous = type == kLayerWavetable || isContinuous (p.exciter);

            float energy = 0.0f;
            for (int j = 0; j < numLive; ++j)
            {
                const auto jj = (size_t) j;
                energy += (zr[jj] * zr[jj] + zi[jj] * zi[jj]) * gain[jj] * gain[jj];
            }
            level = type == kLayerWavetable ? ampEnv : energy;

            if (type == kLayerPhysical && continuous && (keyDown || sustained) && ! fading)
            {
                agcIn += (blockIn - agcIn) * agcCoef;
                agcOut += (blockOut - agcOut) * agcCoef;
                const float target = agcOut > 1.0e-12f
                                       ? std::clamp (std::sqrt (agcIn / agcOut) * kAgcLevel, 0.25f, kAgcMaxGain)
                                       : agcGain;
                // Turn down fast (no spikes when a resonance lines up), up slowly.
                agcGain += (target - agcGain) * (target < agcGain ? agcFastCoef : agcCoef);
            }
            else if (! continuous)
            {
                agcGain = 1.0f;
            }
            blockIn = blockOut = 0.0f;

            const bool held = keyDown || sustained;
            if (type == kLayerWavetable)
            {
                if (! held && ampStage == 2 && ampEnv < 1.0e-4f)
                {
                    kill();
                    return;
                }
            }
            else
            {
                const bool exciting = continuous && (held || ampEnv > 1.0e-4f);
                if (! exciting && sampleCount > (int64_t) (0.02 * sr) && energy < 1.0e-10f)
                {
                    kill();
                    return;
                }
            }
        }

        // Amp envelope.
        if (! (keyDown || sustained) || fading)
            ampStage = 2;
        switch (ampStage)
        {
            case 0:
                ampEnv += (1.2f - ampEnv) * ampAttackCoef;
                if (ampEnv >= 1.0f)
                {
                    ampEnv = 1.0f;
                    ampStage = 1;
                }
                break;
            case 1:  ampEnv += (ampSustain - ampEnv) * ampDecayCoef; break;
            default: ampEnv -= ampEnv * ampReleaseCoef; break;
        }

        float outL = 0.0f, outR = 0.0f;

        if (type == kLayerWavetable)
        {
            float wl, wr, mono;
            renderWavetable (wl, wr, mono, ctx);
            const float a = ampEnv * velocity * kWavetableGain;
            outL = wl * a;
            outR = wr * a;
        }
        else
        {
            // Exciter.
            const bool continuous = isContinuous (p.exciter);
            float eT = 0.0f, eN = 0.0f, dryL = 0.0f, dryR = 0.0f;
            if (continuous)
            {
                const float amount = ampEnv * velocity;

                if (p.exciter == kBow)
                {
                    sawPhase += sawInc;
                    if (sawPhase >= 1.0f)
                        sawPhase -= 1.0f;
                    const float saw = 2.0f * sawPhase - 1.0f - polyBlep (sawPhase, sawInc);
                    eT = amount * saw * (0.3f + 0.7f * h);
                    // Bow noise pulses with the stick-slip cycle.
                    eN = amount * rng.bipolar() * (1.0f - 0.6f * h) * (0.4f + 1.2f * sawPhase);
                    dryL = dryR = (eT + eN * 0.3f);
                }
                else if (p.exciter == kBreath)
                {
                    eN = amount * rng.bipolar();
                    dryL = dryR = eN * 0.5f;
                }
                else if (p.exciter == kExcWavetable)
                {
                    float wl, wr, mono;
                    renderWavetable (wl, wr, mono, ctx);
                    eT = mono * amount;
                    dryL = wl * amount;
                    dryR = wr * amount;
                }
                else // Osc
                {
                    float mono = 0.0f, oL = 0.0f, oR = 0.0f;
                    for (int u = 0; u < oscCount; ++u)
                    {
                        const auto uu = (size_t) u;
                        oscPhase[uu] += oscInc[uu];
                        if (oscPhase[uu] >= 1.0f)
                            oscPhase[uu] -= 1.0f;
                        const float s = oscSample (oscPhase[uu], oscInc[uu], oscWave);
                        mono += s;
                        oL += s * oscPanL[uu];
                        oR += s * oscPanR[uu];
                    }
                    const float scale = amount / std::sqrt ((float) oscCount);
                    eT = mono * scale;
                    dryL = oL * scale;
                    dryR = oR * scale;
                }
            }

            // Object: the resonator bank.
            float l = 0.0f, r = 0.0f;
            for (int jj = 0; jj < numLive; ++jj)
            {
                const auto j = (size_t) jj;
                const float nr = pr[j] * zr[j] - pi[j] * zi[j] + inTonal[j] * eT + inNoise[j] * eN;
                const float ni = pr[j] * zi[j] + pi[j] * zr[j];
                zr[j] = nr;
                zi[j] = ni;
                const float out = ni * gain[j];
                l += out * panL[j];
                r += out * panR[j];
            }

            if (continuous)
            {
                blockIn += eT * eT + eN * eN;
                blockOut += l * l + r * r;
                l *= agcGain;
                r *= agcGain;
            }

            outL = (l * body + dryL * kDryGain * (1.0f - body)) * kVoiceGain;
            outR = (r * body + dryR * kDryGain * (1.0f - body)) * kVoiceGain;
        }

        if (driveOn)
        {
            outL = std::tanh (outL * driveGain) * driveMakeup;
            outR = std::tanh (outR * driveGain) * driveMakeup;
        }

        if (filterMode != kFilterOff)
        {
            float* outs[2] = { &outL, &outR };
            for (int ch = 0; ch < 2; ++ch)
            {
                const auto c = (size_t) ch;
                const float v0 = *outs[ch];
                const float v3 = v0 - ic2[c];
                const float v1 = fA1 * ic1[c] + fA2 * v3;
                const float v2 = ic2[c] + fA2 * ic1[c] + fA3 * v3;
                ic1[c] = 2.0f * v1 - ic1[c];
                ic2[c] = 2.0f * v2 - ic2[c];

                switch (filterMode)
                {
                    case kFilterLP: *outs[ch] = v2 * fComp; break;
                    case kFilterBP: *outs[ch] = v1 * fK; break;
                    case kFilterHP: *outs[ch] = (v0 - fK * v1 - v2) * fComp; break;
                    default:        *outs[ch] = v0 - fK * v1; break; // notch
                }
            }
        }

        gainL += gainLStep;
        gainR += gainRStep;
        float g = 1.0f;
        if (fading)
        {
            fadeGain -= fadeStep;
            if (fadeGain <= 0.0f)
            {
                kill();
                return;
            }
            g = fadeGain;
        }

        left[i] += outL * gainL * g;
        right[i] += outR * gainR * g;
        ++sampleCount;
    }
}

//==============================================================================
void LayerEngine::prepare (double sampleRate, uint32_t seed)
{
    sr = sampleRate;
    for (auto& v : voices)
        v.prepare (sampleRate);
    rng.seed (seed);
    freePhase.fill (0.0);
}

bool LayerEngine::accepts (int note, float velocity) const
{
    return note >= params.keyLow && note <= params.keyHigh
           && velocity >= params.velLow - 1.0e-4f && velocity <= params.velHigh + 1.0e-4f;
}

ModContext LayerEngine::context (const ModContext& shared, double bpm, bool hostPlaying, double ppq) const
{
    ModContext ctx = shared;
    for (int l = 0; l < kNumLfos; ++l)
    {
        const auto ll = (size_t) l;
        const int sync = std::clamp (params.lfoSync[ll], 0, kNumSyncOptions - 1);
        if (sync == 0)
        {
            ctx.lfoHz[ll] = params.lfoRate[ll];
            ctx.lfoPhase[ll] = freePhase[ll];
        }
        else
        {
            const double beats = kSyncBeats[sync];
            ctx.lfoHz[ll] = (float) (bpm / 60.0 / beats);
            // Lock to the song position while the host is playing.
            ctx.lfoPhase[ll] = hostPlaying ? ppq / beats : freePhase[ll];
        }
    }
    return ctx;
}

void LayerEngine::noteOn (int note, float velocity, const ModContext& ctx)
{
    // Same note still sounding: hit it again rather than starting fresh.
    for (auto& v : voices)
    {
        if (v.active && ! v.fading && v.note == note)
        {
            v.retrigger (velocity, params, ctx);
            v.age = ++ageCounter;
            return;
        }
    }

    // Too many notes: fade out the oldest (prefer ones already released).
    int sounding = 0;
    for (auto& v : voices)
        if (v.active && ! v.fading)
            ++sounding;

    if (sounding >= kMaxPolyphony)
    {
        Voice* victim = nullptr;
        for (int pass = 0; pass < 2 && victim == nullptr; ++pass)
            for (auto& v : voices)
                if (v.active && ! v.fading && (pass == 1 || ! v.keyDown))
                    if (victim == nullptr || v.age < victim->age)
                        victim = &v;
        if (victim != nullptr)
            victim->fadeOut();
    }

    Voice* slot = nullptr;
    for (auto& v : voices)
        if (! v.active)
        {
            slot = &v;
            break;
        }

    if (slot == nullptr) // every slot busy: take the oldest
    {
        for (auto& v : voices)
            if (slot == nullptr || v.age < slot->age)
                slot = &v;
        slot->kill();
    }

    slot->start (note, velocity, params, ctx, rng.next());
    slot->age = ++ageCounter;
}

void LayerEngine::noteOff (int note, bool sustainDown)
{
    for (auto& v : voices)
    {
        if (v.active && v.keyDown && v.note == note)
        {
            v.keyDown = false;
            v.sustained = sustainDown;
        }
    }
}

void LayerEngine::releaseSustained()
{
    for (auto& v : voices)
        v.sustained = false;
}

void LayerEngine::allNotesOff()
{
    for (auto& v : voices)
    {
        v.keyDown = false;
        v.sustained = false;
    }
}

void LayerEngine::allSoundOff()
{
    for (auto& v : voices)
        v.kill();
}

void LayerEngine::fadeAll()
{
    for (auto& v : voices)
        if (v.active)
            v.fadeOut();
}

void LayerEngine::render (float* left, float* right, int numSamples, const ModContext& ctx)
{
    for (auto& v : voices)
        v.render (left, right, numSamples, params, ctx);

    // Advance the free-running LFO clocks past this chunk.
    for (int l = 0; l < kNumLfos; ++l)
        freePhase[(size_t) l] = ctx.lfoPhase[(size_t) l] + (double) ctx.lfoHz[(size_t) l] * numSamples / sr;
}

bool LayerEngine::anyActive() const
{
    for (auto& v : voices)
        if (v.active)
            return true;
    return false;
}

int LayerEngine::activeVoiceCount() const
{
    int n = 0;
    for (auto& v : voices)
        if (v.active)
            ++n;
    return n;
}

//==============================================================================
void Engine::prepare (double sampleRate)
{
    sr = sampleRate;
    builtInWavetables(); // make sure the tables exist before audio starts
    for (int i = 0; i < kNumLayers; ++i)
        layers[(size_t) i].prepare (sampleRate, 0x1234567u + (uint32_t) i * 7919u);
}

void Engine::setPatch (const Patch& p)
{
    for (int i = 0; i < kNumLayers; ++i)
        layers[(size_t) i].setParams (p.layers[(size_t) i]);
    shared.macros = p.macros;
}

void Engine::setTransport (double newBpm, bool playing, double ppqAtBlockStart)
{
    bpm = newBpm > 1.0 ? newBpm : 120.0;
    hostPlaying = playing;
    ppq = ppqAtBlockStart;
}

void Engine::setImportedTable (int layer, const Wavetable* table)
{
    if (layer >= 0 && layer < kNumLayers)
        imported[(size_t) layer].store (table);
}

const Wavetable* Engine::tableFor (int layer) const
{
    const auto& builtIns = builtInWavetables();
    const int index = layers[(size_t) layer].getParams().wtTable;
    if (index >= 0 && index < (int) builtIns.size())
        return builtIns[(size_t) index].get();
    if (const Wavetable* t = imported[(size_t) layer].load())
        return t;
    return builtIns.empty() ? nullptr : builtIns[0].get();
}

void Engine::noteOn (int note, float velocity)
{
    velocity = std::clamp (velocity, 0.0f, 1.0f);
    for (int i = 0; i < kNumLayers; ++i)
    {
        auto& layer = layers[(size_t) i];
        if (layer.getParams().enabled != 0 && layer.accepts (note, velocity))
        {
            ModContext ctx = layer.context (shared, bpm, hostPlaying, ppq);
            ctx.table = tableFor (i);
            layer.noteOn (note, velocity, ctx);
        }
    }
}

void Engine::noteOff (int note)
{
    for (auto& layer : layers)
        layer.noteOff (note, sustainDown);
}

void Engine::setSustain (bool down)
{
    sustainDown = down;
    if (! down)
        for (auto& layer : layers)
            layer.releaseSustained();
}

void Engine::allNotesOff()
{
    for (auto& layer : layers)
        layer.allNotesOff();
    sustainDown = false;
}

void Engine::allSoundOff()
{
    for (auto& layer : layers)
        layer.allSoundOff();
    sustainDown = false;
}

void Engine::render (float* left, float* right, int numSamples)
{
    std::fill (left, left + numSamples, 0.0f);
    std::fill (right, right + numSamples, 0.0f);

    for (int i = 0; i < kNumLayers; ++i)
    {
        auto& layer = layers[(size_t) i];
        const bool enabled = layer.getParams().enabled != 0;

        // A layer switched off fades out its notes instead of cutting them.
        if (wasEnabled[(size_t) i] && ! enabled)
            layer.fadeAll();
        wasEnabled[(size_t) i] = enabled;

        ModContext ctx = layer.context (shared, bpm, hostPlaying, ppq);
        ctx.table = tableFor (i);
        layer.render (left, right, numSamples, ctx);
    }

    ppq += numSamples * bpm / 60.0 / sr;
}

int Engine::activeVoiceCount() const
{
    int n = 0;
    for (auto& layer : layers)
        n += layer.activeVoiceCount();
    return n;
}

} // namespace impossible
