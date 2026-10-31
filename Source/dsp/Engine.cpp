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
    constexpr float kCellsGain = 0.55f;
    constexpr float kSwarmGain = 0.35f;
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
        return exciter == kBow || exciter == kBreath || exciter == kOsc || exciter == kExcWavetable
               || exciter == kExcCells || exciter == kExcSwarm;
    }

    // The sound source a Physical exciter borrows from another engine (or -1).
    int borrowedSource (int exciter)
    {
        switch (exciter)
        {
            case kExcWavetable: return kLayerWavetable;
            case kExcCells:     return kLayerCells;
            case kExcSwarm:     return kLayerSwarm;
            default:            return -1;
        }
    }

    // One cycle of a sine, for cheap lookups.
    constexpr int kSineSize = 2048;
    const std::array<float, kSineSize + 1>& sineTable()
    {
        static const auto table = []
        {
            std::array<float, kSineSize + 1> t {};
            for (int i = 0; i <= kSineSize; ++i)
                t[(size_t) i] = (float) std::sin (2.0 * 3.14159265358979323846 * i / kSineSize);
            return t;
        }();
        return table;
    }

    float fastSin (float phase) // phase in [0, 1)
    {
        const auto& t = sineTable();
        const float pos = phase * (float) kSineSize;
        const int i = std::min ((int) pos, kSineSize - 1);
        const float f = pos - (float) i;
        return t[(size_t) i] + (t[(size_t) i + 1] - t[(size_t) i]) * f;
    }

    // Birth / survival rules as bit masks over the neighbour count (0..8).
    struct CellRuleMasks { uint32_t birth, survive; };
    CellRuleMasks ruleMasks (int rule)
    {
        auto bits = [] (std::initializer_list<int> counts)
        {
            uint32_t m = 0;
            for (int c : counts)
                m |= 1u << c;
            return m;
        };
        switch (rule)
        {
            case kRuleHighLife: return { bits ({ 3, 6 }), bits ({ 2, 3 }) };
            case kRuleSeeds:    return { bits ({ 2 }), 0u };
            case kRuleDayNight: return { bits ({ 3, 6, 7, 8 }), bits ({ 3, 4, 6, 7, 8 }) };
            case kRuleMaze:     return { bits ({ 3 }), bits ({ 1, 2, 3, 4, 5 }) };
            case kRuleAnneal:   return { bits ({ 4, 6, 7, 8 }), bits ({ 3, 5, 6, 7, 8 }) };
            default:            return { bits ({ 3 }), bits ({ 2, 3 }) }; // Life
        }
    }

    // Home pitch (octaves above the note) of particle i of n.
    float swarmHomeX (int home, int i, int n)
    {
        switch (home)
        {
            case kHomeUnison:    return 0.0f;
            case kHomeHarmonics: return std::log2 ((float) (i + 1));
            case kHomeOctaves:   return (float) (i % 4);
            case kHomeFifths:    return std::fmod ((float) i * 7.0f / 12.0f, 3.0f);
            case kHomeChord:
            {
                static const int tones[5] = { 0, 4, 7, 11, 14 };
                return (float) (tones[i % 5] + 12 * (i / 5)) / 12.0f;
            }
            default: // Cluster: semitone steps around the note
                return ((float) i - (float) (n - 1) * 0.5f) / 12.0f;
        }
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

    cellRng.seed (seed ^ 0x51ED270Bu);
    swarmRng.seed (seed ^ 0x2545F491u);
    seedCells (p);
    initSwarm (p);

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
    apply (m.cellSpeed, ranges::cellSpeed, kDstCellSpeed);
    apply (m.cellTilt, ranges::cellTilt, kDstCellTilt);
    apply (m.cellMutate, ranges::cellMutate, kDstCellMutate);
    apply (m.swarmPull, ranges::swarmPull, kDstSwarmPull);
    apply (m.swarmChaos, ranges::swarmChaos, kDstSwarmChaos);
    apply (m.swarmFlock, ranges::swarmFlock, kDstSwarmFlock);
    apply (m.swarmSpread, ranges::swarmSpread, kDstSwarmSpread);

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
}

void Voice::renderSub (float& outL, float& outR, float& mono)
{
    if (subLevel <= 0.001f)
        return;
    subPhase += subInc;
    if (subPhase >= 1.0f)
        subPhase -= 1.0f;
    const float sub = fastSin (subPhase) * subLevel;
    outL += sub * 0.7071f;
    outR += sub * 0.7071f;
    mono += sub;
}

void Voice::renderSource (int source, float& outL, float& outR, float& mono, const ModContext& ctx)
{
    switch (source)
    {
        case kLayerWavetable: renderWavetable (outL, outR, mono, ctx); break;
        case kLayerCells:     renderCells (outL, outR, mono); break;
        case kLayerSwarm:     renderSwarm (outL, outR, mono); break;
        default:              outL = outR = mono = 0.0f; return;
    }
    renderSub (outL, outR, mono);
}

//==============================================================================
// Cells

void Voice::seedCells (const LayerParams& p)
{
    grid.fill (0u);
    auto set = [this] (int x, int y)
    {
        x = ((x % kCellCols) + kCellCols) % kCellCols;
        y = ((y % kCellRows) + kCellRows) % kCellRows;
        grid[(size_t) y] |= 1u << x;
    };
    auto place = [&set] (std::initializer_list<std::pair<int, int>> cells, int ox, int oy)
    {
        for (auto [x, y] : cells)
            set (ox + x, oy + y);
    };
    auto randomX = [this] { return (int) (cellRng.uniform() * kCellCols); };
    auto randomY = [this] { return (int) (cellRng.uniform() * kCellRows); };
    const float density = std::clamp (p.cellDensity, 0.0f, 1.0f);

    switch (p.cellSeed)
    {
        case kSeedGliders:
            for (int g = 0, n = 1 + (int) (density * 5.0f); g < n; ++g)
                place ({ { 1, 0 }, { 2, 1 }, { 0, 2 }, { 1, 2 }, { 2, 2 } }, randomX(), randomY());
            break;
        case kSeedRPentomino:
            place ({ { 1, 0 }, { 2, 0 }, { 0, 1 }, { 1, 1 }, { 1, 2 } }, 15, 7);
            break;
        case kSeedAcorn:
            place ({ { 1, 0 }, { 3, 1 }, { 0, 2 }, { 1, 2 }, { 4, 2 }, { 5, 2 }, { 6, 2 } }, 12, 6);
            break;
        case kSeedSymmetric:
            for (int y = 0; y < kCellRows; ++y)
                for (int x = 0; x < kCellCols / 2; ++x)
                    if (cellRng.uniform() < density)
                    {
                        set (x, y);
                        set (kCellCols - 1 - x, y);
                    }
            break;
        case kSeedSpaceships:
            for (int g = 0, n = 1 + (int) (density * 2.0f); g < n; ++g)
                place ({ { 1, 0 }, { 4, 0 }, { 0, 1 }, { 0, 2 }, { 4, 2 }, { 0, 3 }, { 1, 3 }, { 2, 3 }, { 3, 3 } },
                       randomX(), (g * 6 + randomY() / 3) % kCellRows);
            break;
        default:
            for (int y = 0; y < kCellRows; ++y)
                for (int x = 0; x < kCellCols; ++x)
                    if (cellRng.uniform() < density)
                        set (x, y);
            break;
    }

    if (std::all_of (grid.begin(), grid.end(), [] (uint32_t r) { return r == 0u; }))
        place ({ { 1, 0 }, { 2, 0 }, { 0, 1 }, { 1, 1 }, { 1, 2 } }, 15, 7); // never start empty

    cellClock = 0.0;
    cellFade = 1.0f;
    cellsDirty = true;
}

void Voice::stepCells (const LayerParams& m)
{
    const auto masks = ruleMasks (m.cellRule);
    std::array<uint32_t, kCellRows> next {};

    for (int y = 0; y < kCellRows; ++y)
    {
        const uint32_t up = grid[(size_t) ((y + kCellRows - 1) % kCellRows)];
        const uint32_t mid = grid[(size_t) y];
        const uint32_t down = grid[(size_t) ((y + 1) % kCellRows)];
        uint32_t row = 0;
        for (int x = 0; x < kCellCols; ++x)
        {
            const int xl = (x + kCellCols - 1) % kCellCols;
            const int xr = (x + 1) % kCellCols;
            auto bit = [] (uint32_t r, int i) { return (int) ((r >> i) & 1u); };
            const int n = bit (up, xl) + bit (up, x) + bit (up, xr) + bit (mid, xl) + bit (mid, xr)
                          + bit (down, xl) + bit (down, x) + bit (down, xr);
            const bool alive = bit (mid, x) != 0;
            if (alive ? ((masks.survive >> n) & 1u) : ((masks.birth >> n) & 1u))
                row |= 1u << x;
        }
        next[(size_t) y] = row;
    }
    grid = next;

    // Mutation: flip a few random cells to keep things moving.
    const float mutate = std::clamp (m.cellMutate, 0.0f, 1.0f);
    if (mutate > 0.0f && cellRng.uniform() < 0.3f + 0.7f * mutate)
    {
        const int flips = 1 + (int) (mutate * 6.0f);
        for (int f = 0; f < flips; ++f)
            grid[(size_t) ((int) (cellRng.uniform() * kCellRows) % kCellRows)] ^= 1u << ((int) (cellRng.uniform() * kCellCols) % kCellCols);
    }

    // A dead grid comes back to life so the note keeps sounding.
    if (std::all_of (grid.begin(), grid.end(), [] (uint32_t r) { return r == 0u; }))
    {
        LayerParams reseed = m;
        reseed.cellSeed = kSeedRandom;
        reseed.cellDensity = std::max (0.25f, m.cellDensity);
        seedCells (reseed);
    }
}

void Voice::buildCellWave (float f0)
{
    // Columns -> harmonics (amplitude = how many cells are alive in the column),
    // rows -> stereo position (centre of the live cells in the column).
    std::array<float, kCellCols> amp {}, gL {}, gR {};
    float sumSq = 0.0f;
    const float nyquist = 0.45f * (float) sr;
    for (int x = 0; x < kCellCols; ++x)
    {
        int count = 0, rowSum = 0;
        for (int y = 0; y < kCellRows; ++y)
            if ((grid[(size_t) y] >> x) & 1u)
            {
                ++count;
                rowSum += y;
            }
        const int h = x + 1;
        if (count == 0 || f0 * (float) h >= nyquist)
            continue;
        const float a = (float) count / (float) kCellRows * std::pow ((float) h, -cellTiltNow);
        const float pan = (float) rowSum / (float) count / (float) (kCellRows - 1) * 2.0f - 1.0f;
        const float angle = (pan * 0.9f + 1.0f) * kPi * 0.25f;
        amp[(size_t) x] = a;
        gL[(size_t) x] = std::cos (angle);
        gR[(size_t) x] = std::sin (angle);
        sumSq += a * a;
    }

    // Keep the level steady as the population changes.
    const float norm = 0.5f / std::sqrt (sumSq * 0.5f + 0.002f);

    cellWaveL[0] = cellWaveL[1];
    cellWaveR[0] = cellWaveR[1];
    auto& wl = cellWaveL[1];
    auto& wr = cellWaveR[1];
    wl.fill (0.0f);
    wr.fill (0.0f);
    const auto& sine = sineTable();
    constexpr int step = kSineSize / kCellTable;
    for (int x = 0; x < kCellCols; ++x)
    {
        const float a = amp[(size_t) x] * norm;
        if (a <= 0.0f)
            continue;
        const float al = a * gL[(size_t) x], ar = a * gR[(size_t) x];
        const int h = x + 1;
        for (int n = 0; n < kCellTable; ++n)
        {
            const float s = sine[(size_t) (((h * n) % kCellTable) * step)];
            wl[(size_t) n] += al * s;
            wr[(size_t) n] += ar * s;
        }
    }
    wl[kCellTable] = wl[0];
    wr[kCellTable] = wr[0];
}

void Voice::updateCells (const LayerParams& m, const ModContext& ctx, float f0)
{
    const int sync = std::clamp (m.cellSync, 0, kNumSyncOptions - 1);
    const float gensPerSecond = sync > 0 ? (float) (ctx.bpm / 60.0 / kSyncBeats[sync]) : std::max (0.05f, m.cellSpeed);
    cellTiltNow = std::clamp (m.cellTilt, 0.0f, 2.0f);
    cellInc = std::min (0.45f, f0 * invSr);

    if (cellsDirty)
    {
        buildCellWave (f0);
        cellWaveL[0] = cellWaveL[1];
        cellWaveR[0] = cellWaveR[1];
        cellF0 = f0;
        cellsDirty = false;
    }

    cellClock += (double) gensPerSecond * kControlInterval * invSr;
    for (int guard = 0; cellClock >= 1.0 && guard < 4; ++guard)
    {
        cellClock -= 1.0;
        stepCells (m);
        buildCellWave (f0);
        cellF0 = f0;
        const float periodSamples = (float) sr / gensPerSecond;
        cellFade = 0.0f;
        cellFadeInc = 1.0f / std::max (1.0f, std::clamp (m.cellGlide, 0.0f, 1.0f) * periodSamples);
    }
    if (cellClock >= 1.0)
        cellClock = 0.0;

    // A big pitch change (e.g. pitch bend) can push harmonics past Nyquist: rebuild.
    if (std::fabs (f0 - cellF0) > cellF0 * 0.06f)
    {
        buildCellWave (f0);
        cellWaveL[0] = cellWaveL[1];
        cellWaveR[0] = cellWaveR[1];
        cellF0 = f0;
    }
}

void Voice::renderCells (float& outL, float& outR, float& mono)
{
    cellPhase += cellInc;
    if (cellPhase >= 1.0f)
        cellPhase -= 1.0f;
    const float pos = cellPhase * (float) kCellTable;
    const int i = std::min ((int) pos, kCellTable - 1);
    const float t = pos - (float) i;

    auto read = [i, t] (const std::array<float, kCellTable + 1>& w) { return w[(size_t) i] + (w[(size_t) i + 1] - w[(size_t) i]) * t; };
    const float l1 = read (cellWaveL[1]), r1 = read (cellWaveR[1]);
    if (cellFade < 1.0f)
    {
        const float l0 = read (cellWaveL[0]), r0 = read (cellWaveR[0]);
        outL = l0 + (l1 - l0) * cellFade;
        outR = r0 + (r1 - r0) * cellFade;
        cellFade = std::min (1.0f, cellFade + cellFadeInc);
    }
    else
    {
        outL = l1;
        outR = r1;
    }
    mono = (outL + outR) * 0.7071f;
}

//==============================================================================
// Swarm

void Voice::initSwarm (const LayerParams& p)
{
    for (int i = 0; i < kMaxParticles; ++i)
    {
        auto& q = particles[(size_t) i];
        q.x = swarmHomeX (p.swarmHome, i, kMaxParticles) * p.swarmSpread * 2.0f + 0.3f * swarmRng.bipolar();
        q.y = swarmRng.bipolar();
        q.vx = q.vy = 0.0f;
        q.phase = swarmRng.uniform();
    }
}

void Voice::updateSwarm (const LayerParams& m, const ModContext&, float f0, float dt)
{
    particleCount = 4 << std::clamp (m.swarmCount, 0, 3);
    swarmWave = m.swarmWave;

    const float pull = std::clamp (m.swarmPull, 0.0f, 1.0f);
    const float chaos = std::clamp (m.swarmChaos, 0.0f, 1.0f);
    const float flock = std::clamp (m.swarmFlock, -1.0f, 1.0f);
    const float stretch = std::clamp (m.swarmSpread, 0.0f, 1.0f) * 2.0f; // 1 = exact home notes
    const float stiffness = 4.0f + 120.0f * pull * pull;
    const float damping = 1.0f + 0.7f * std::sqrt (stiffness);
    const float kick = chaos * chaos * 60.0f + chaos * 4.0f;
    const float step = dt * std::max (0.01f, m.swarmSpeed);
    const float decay = std::exp (-damping * step);

    float meanX = 0.0f, meanY = 0.0f;
    for (int i = 0; i < particleCount; ++i)
    {
        meanX += particles[(size_t) i].x;
        meanY += particles[(size_t) i].y;
    }
    meanX /= (float) particleCount;
    meanY /= (float) particleCount;

    float weightSq = 0.0f;
    const float nyquist = 0.45f * (float) sr;
    for (int i = 0; i < particleCount; ++i)
    {
        auto& q = particles[(size_t) i];
        q.homeX = swarmHomeX (m.swarmHome, i, particleCount) * stretch;
        q.homeY = particleCount > 1 ? (float) ((i * 7) % particleCount) / (float) (particleCount - 1) * 1.6f - 0.8f : 0.0f;

        // Spring toward home, random kicks, and flocking toward (or away from) the crowd.
        const float noiseX = (swarmRng.bipolar() + swarmRng.bipolar()) * 0.5f;
        const float noiseY = (swarmRng.bipolar() + swarmRng.bipolar()) * 0.5f;
        const float ax = stiffness * (q.homeX - q.x) + kick * noiseX + flock * 10.0f * (meanX - q.x);
        const float ay = stiffness * 0.5f * (q.homeY - q.y) + kick * noiseY + flock * 10.0f * (meanY - q.y);
        q.vx = (q.vx + ax * step) * decay;
        q.vy = (q.vy + ay * step) * decay;
        q.x += q.vx * step;
        q.y += q.vy * step;

        if (q.x < -3.0f) { q.x = -3.0f; q.vx = std::fabs (q.vx) * 0.5f; }
        if (q.x > 5.0f)  { q.x = 5.0f;  q.vx = -std::fabs (q.vx) * 0.5f; }
        if (q.y < -1.0f) { q.y = -1.0f; q.vy = std::fabs (q.vy) * 0.5f; }
        if (q.y > 1.0f)  { q.y = 1.0f;  q.vy = -std::fabs (q.vy) * 0.5f; }

        const float freq = f0 * std::exp2 (q.x);
        q.inc = std::min (0.45f, freq * invSr);
        const float weight = m.swarmHome == kHomeHarmonics ? 1.0f / std::sqrt ((float) (i + 1)) : 1.0f;
        q.amp = (freq < nyquist && freq > 15.0f) ? weight : 0.0f;
        weightSq += q.amp * q.amp;

        const float angle = (q.y * 0.9f + 1.0f) * kPi * 0.25f;
        q.panL = std::cos (angle);
        q.panR = std::sin (angle);
    }

    const float norm = weightSq > 0.0f ? 1.0f / std::sqrt (weightSq) : 0.0f;
    for (int i = 0; i < particleCount; ++i)
        particles[(size_t) i].amp *= norm;
}

void Voice::renderSwarm (float& outL, float& outR, float& mono)
{
    outL = outR = 0.0f;
    const float wave = std::clamp (swarmWave, 0.0f, 1.0f) * 2.0f;
    for (int i = 0; i < particleCount; ++i)
    {
        auto& q = particles[(size_t) i];
        if (q.amp <= 0.0f)
            continue;
        q.phase += q.inc;
        if (q.phase >= 1.0f)
            q.phase -= 1.0f;

        float s = fastSin (q.phase);
        if (wave > 0.001f)
        {
            const float tri = 1.0f - 4.0f * std::fabs (q.phase - 0.5f);
            if (wave <= 1.0f)
                s += (tri - s) * wave;
            else
            {
                const float saw = 2.0f * q.phase - 1.0f - polyBlep (q.phase, q.inc);
                s = tri + (saw - tri) * (wave - 1.0f);
            }
        }
        outL += s * q.amp * q.panL;
        outR += s * q.amp * q.panR;
    }
    mono = (outL + outR) * 0.7071f;
}

void Voice::publish (const ModContext& ctx)
{
    if (ctx.display == nullptr || age != ctx.newestAge)
        return;
    for (int y = 0; y < kCellRows; ++y)
        ctx.display->cells[(size_t) y].store (grid[(size_t) y], std::memory_order_relaxed);
    for (int i = 0; i < particleCount; ++i)
    {
        const auto& q = particles[(size_t) i];
        const auto px = (uint32_t) std::clamp ((q.x + 3.0f) / 8.0f * 65535.0f, 0.0f, 65535.0f);
        const auto py = (uint32_t) std::clamp ((q.y + 1.0f) * 0.5f * 65535.0f, 0.0f, 65535.0f);
        ctx.display->particles[(size_t) i].store ((px << 16) | py, std::memory_order_relaxed);
    }
    ctx.display->particleCount.store (particleCount, std::memory_order_relaxed);
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
    ampSustain = type != kLayerPhysical ? std::clamp (m.ampSustain, 0.0f, 1.0f) : 1.0f;

    subInc = std::min (0.45f, f0 * 0.5f * invSr);
    subLevel = m.wtSub;

    const int source = type != kLayerPhysical ? type : borrowedSource (m.exciter);
    if (source == kLayerWavetable)
        updateWavetable (m, ctx, f0);
    else if (source == kLayerCells)
        updateCells (m, ctx, f0);
    else if (source == kLayerSwarm)
        updateSwarm (m, ctx, f0, dt);

    if (type != kLayerPhysical)
    {
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

            const bool continuous = type != kLayerPhysical || isContinuous (p.exciter);
            publish (ctx);

            float energy = 0.0f;
            for (int j = 0; j < numLive; ++j)
            {
                const auto jj = (size_t) j;
                energy += (zr[jj] * zr[jj] + zi[jj] * zi[jj]) * gain[jj] * gain[jj];
            }
            level = type != kLayerPhysical ? ampEnv : energy;

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
            if (type != kLayerPhysical)
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

        if (type != kLayerPhysical)
        {
            float wl, wr, mono;
            renderSource (type, wl, wr, mono, ctx);
            const float engineGain = type == kLayerWavetable ? kWavetableGain : type == kLayerCells ? kCellsGain : kSwarmGain;
            const float a = ampEnv * velocity * engineGain;
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
                else if (borrowedSource (p.exciter) >= 0)
                {
                    float wl, wr, mono;
                    renderSource (borrowedSource (p.exciter), wl, wr, mono, ctx);
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
    ctx.bpm = bpm;
    ctx.newestAge = newestAge();
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

uint64_t LayerEngine::newestAge() const
{
    uint64_t newest = 0;
    for (auto& v : voices)
        if (v.active && v.age > newest)
            newest = v.age;
    return newest;
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
            ctx.display = &displays[(size_t) i];
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
        ctx.display = &displays[(size_t) i];
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
