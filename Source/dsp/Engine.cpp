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

    bool isContinuous (int exciter) { return exciter == kBow || exciter == kBreath; }
} // namespace

//==============================================================================
void Voice::prepare (double sampleRate)
{
    sr = sampleRate;
    invSr = (float) (1.0 / sampleRate);
    attackCoef = 1.0f - std::exp (-1.0f / (0.04f * (float) sr));
    releaseCoef = 1.0f - std::exp (-1.0f / (0.08f * (float) sr));
    fadeStep = 1.0f / (0.006f * (float) sr); // 6 ms fade when a voice is stolen
    kill();
}

void Voice::start (int newNote, float vel, const Params& p, float bend, uint32_t seed)
{
    note = newNote;
    velocity = vel;
    active = true;
    keyDown = true;
    sustained = false;
    fading = false;
    fadeGain = 1.0f;
    sampleCount = 0;
    contEnv = 0.0f;
    sawPhase = 0.0f;

    rng.seed (seed);
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

    zr.fill (0.0f);
    zi.fill (0.0f);
    gain.fill (1.0f - 0.9f * p.bloom);

    updateControl (p, bend);
    controlCounter = kControlInterval;

    if (! isContinuous (p.exciter))
        excite (vel);
}

void Voice::retrigger (float vel, const Params& p, float bend)
{
    // Hitting an object that is still ringing: keep the ringing, add energy.
    velocity = vel;
    keyDown = true;
    sustained = false;
    sampleCount = 0;
    updateControl (p, bend);
    controlCounter = kControlInterval;

    if (! isContinuous (p.exciter))
        excite (vel);
}

void Voice::excite (float vel)
{
    for (int j = 0; j < numLive; ++j)
        zr[(size_t) j] += resAmp[(size_t) j] * vel * kImpulseGain;
}

void Voice::fadeOut()
{
    fading = true;
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

void Voice::updateControl (const Params& p, float bend)
{
    const float t = (float) sampleCount * invSr;
    const float evolveTime = std::max (0.02f, p.evolve);
    const float evo = 1.0f - std::exp (-t / evolveTime);

    const float material = std::clamp (p.material + p.transmute * evo, 0.0f, 1.0f);
    const MaterialProps mp = materialAt (material);

    const float f0 = 440.0f * std::exp2 (((float) note - 69.0f + bend + 12.0f * (float) p.octave) / 12.0f);
    sawInc = std::min (0.5f, f0 * invSr);

    const float sizeDecay = std::exp2 ((p.size - 0.5f) * 4.5f);
    const float noteDecay = std::clamp (std::pow (220.0f / f0, 0.35f), 0.35f, 3.0f);
    const float baseT60 = mp.t60 * p.decay * sizeDecay * noteDecay * (1.0f + 2.0f * p.bloom);
    const bool released = ! keyDown && ! sustained;

    // Brightness of the excitation (mallet hardness, pluck brightness, bow/breath tone).
    float cutoff, positionAmount;
    switch (p.exciter)
    {
        case kStrike: cutoff = 150.0f * std::exp2 (p.hardness * 6.5f + velocity * 1.5f); positionAmount = 0.8f; break;
        case kPluck:  cutoff = 600.0f * std::exp2 (p.hardness * 5.0f + velocity * 1.5f); positionAmount = 0.97f; break;
        default:      cutoff = 250.0f * std::exp2 (p.hardness * 6.0f); positionAmount = 0.5f; break;
    }

    const float tilt = mp.beta + (p.size - 0.5f) * 1.0f; // bigger objects: more low end
    const float irregularity = std::max (0.0f, (p.size - 0.5f) * 2.0f) * 0.006f;
    const float split = p.fracture * (0.25f + 0.75f * evo);
    const float nyquistLimit = 0.46f * (float) sr;
    const float bloomTarget = 1.0f + 3.0f * p.bloom;

    float ampSum = 0.0f;
    int lastLive = 0;
    std::array<float, kNumRes> resR {};

    for (int k = 0; k < kNumModes; ++k)
    {
        float ratio = shapeRatio (p.shape, k);
        ratio *= std::sqrt (1.0f + mp.stiff * (float) ((k + 1) * (k + 1)));

        if (k > 0)
        {
            ratio *= 1.0f + irregularity * rndIrreg[(size_t) k];
            ratio *= 1.0f + p.fracture * 0.06f * rndFrac[(size_t) k];
        }

        // Drift: upper partials bend more than the fundamental (up to ±1 octave).
        const float weight = 0.15f + 0.85f * std::clamp (std::log2 (std::max (ratio, 1.0f)) / 5.0f, 0.0f, 1.0f);
        ratio *= std::exp2 (p.drift * weight * evo);

        const float freq = f0 * ratio;

        float t60 = baseT60 / std::pow (std::max (ratio, 0.5f), mp.alpha);
        t60 = std::clamp (t60, 0.004f, 120.0f);
        if (released)
            t60 = std::min (t60, std::max (0.01f, p.release));
        const float r = std::exp (-kLn1000 / (t60 * (float) sr));

        float a = std::pow (std::max (ratio, 0.25f), -tilt);
        const float x = freq / cutoff;
        a /= std::sqrt (1.0f + x * x * x * x);
        const float hitShape = std::fabs (std::sin (kPi * (float) (k + 1) * p.position));
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
    }

    // Bloom gains move at control rate.
    for (int jj = 0; jj < numLive; ++jj)
    {
        const auto j = (size_t) jj;
        gain[j] += (gainTarget[j] - gain[j]) * gainCoef[j];
    }
}

void Voice::render (float* left, float* right, int numSamples, const Params& p, float bend)
{
    if (! active)
        return;

    const bool continuous = isContinuous (p.exciter);
    const float h = p.hardness;

    for (int i = 0; i < numSamples; ++i)
    {
        if (--controlCounter <= 0)
        {
            updateControl (p, bend);
            controlCounter = kControlInterval;

            float energy = 0.0f;
            for (int j = 0; j < numLive; ++j)
            {
                const auto jj = (size_t) j;
                energy += (zr[jj] * zr[jj] + zi[jj] * zi[jj]) * gain[jj] * gain[jj];
            }
            level = energy;

            const bool exciting = continuous && (keyDown || sustained || contEnv > 1.0e-4f);
            if (! exciting && sampleCount > (int64_t) (0.02 * sr) && energy < 1.0e-10f)
            {
                kill();
                return;
            }
        }

        float eT = 0.0f, eN = 0.0f;
        if (continuous)
        {
            const float target = ((keyDown || sustained) && ! fading) ? 1.0f : 0.0f;
            contEnv += (target - contEnv) * (target > contEnv ? attackCoef : releaseCoef);
            const float noise = rng.bipolar();

            if (p.exciter == kBow)
            {
                sawPhase += sawInc;
                if (sawPhase >= 1.0f)
                    sawPhase -= 1.0f;
                const float saw = 2.0f * sawPhase - 1.0f - polyBlep (sawPhase, sawInc);
                const float amount = contEnv * velocity;
                eT = amount * saw * (0.3f + 0.7f * h);
                // Bow noise pulses with the stick-slip cycle.
                eN = amount * noise * (1.0f - 0.6f * h) * (0.4f + 1.2f * sawPhase);
            }
            else
            {
                eN = contEnv * velocity * noise;
            }
        }

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

        float g = kVoiceGain;
        if (fading)
        {
            fadeGain -= fadeStep;
            if (fadeGain <= 0.0f)
            {
                kill();
                return;
            }
            g *= fadeGain;
        }

        left[i] += l * g;
        right[i] += r * g;
        ++sampleCount;
    }
}

//==============================================================================
void Engine::prepare (double sampleRate)
{
    for (auto& v : voices)
        v.prepare (sampleRate);
    rng.seed (0x1234567u);
}

void Engine::noteOn (int note, float velocity)
{
    velocity = std::clamp (velocity, 0.0f, 1.0f);

    // Same note still sounding: hit it again rather than starting fresh.
    for (auto& v : voices)
    {
        if (v.active && ! v.fading && v.note == note)
        {
            v.retrigger (velocity, params, bend);
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

    slot->start (note, velocity, params, bend, rng.next());
    slot->age = ++ageCounter;
}

void Engine::noteOff (int note)
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

void Engine::setSustain (bool down)
{
    sustainDown = down;
    if (! down)
        for (auto& v : voices)
            v.sustained = false;
}

void Engine::allNotesOff()
{
    for (auto& v : voices)
    {
        v.keyDown = false;
        v.sustained = false;
    }
    sustainDown = false;
}

void Engine::allSoundOff()
{
    for (auto& v : voices)
        v.kill();
    sustainDown = false;
}

void Engine::render (float* left, float* right, int numSamples)
{
    std::fill (left, left + numSamples, 0.0f);
    std::fill (right, right + numSamples, 0.0f);

    for (auto& v : voices)
        v.render (left, right, numSamples, params, bend);
}

int Engine::activeVoiceCount() const
{
    int n = 0;
    for (auto& v : voices)
        if (v.active)
            ++n;
    return n;
}

} // namespace impossible
