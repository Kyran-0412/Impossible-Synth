// Offline test for the Impossible Synth engine (no JUCE, no DAW needed).
// Renders every factory preset to a WAV file, checks levels, and stress-tests
// hundreds of random patches for silence, blow-ups and NaN/Inf.
//
// Build:  g++ -std=c++17 -O2 -ISource tools/render_test.cpp Source/dsp/Engine.cpp -o render_test
// Run:    ./render_test [output-folder]

#include "dsp/Presets.h"

#if defined(__SSE__) || defined(_M_X64)
 #include <xmmintrin.h>
 #define HAS_SSE_FTZ 1
#endif

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace impossible;

namespace
{
constexpr double kSampleRate = 48000.0;

struct Note
{
    double on, off;
    int pitch;
    float velocity;
};

struct Result
{
    double peak = 0.0, rms = 0.0;
    bool finite = true;
};

void writeWav (const std::string& path, const std::vector<float>& l, const std::vector<float>& r)
{
    FILE* f = std::fopen (path.c_str(), "wb");
    if (f == nullptr)
        return;

    const auto frames = (uint32_t) l.size();
    const uint32_t dataBytes = frames * 2 * 2;
    auto u32 = [f] (uint32_t v) { std::fwrite (&v, 4, 1, f); };
    auto u16 = [f] (uint16_t v) { std::fwrite (&v, 2, 1, f); };

    std::fwrite ("RIFF", 1, 4, f);
    u32 (36 + dataBytes);
    std::fwrite ("WAVEfmt ", 1, 8, f);
    u32 (16);
    u16 (1);
    u16 (2);
    u32 ((uint32_t) kSampleRate);
    u32 ((uint32_t) kSampleRate * 4);
    u16 (4);
    u16 (16);
    std::fwrite ("data", 1, 4, f);
    u32 (dataBytes);

    for (uint32_t i = 0; i < frames; ++i)
    {
        for (float s : { l[i], r[i] })
        {
            const auto v = (int16_t) std::lrint (std::tanh (s) * 32767.0f); // same safety clip as the plugin
            std::fwrite (&v, 2, 1, f);
        }
    }
    std::fclose (f);
}

// Renders notes through the engine. Output gain is applied, reverb is not.
Result render (const Patch& params, const std::vector<Note>& notes, double seconds,
               std::vector<float>* outL = nullptr, std::vector<float>* outR = nullptr)
{
    Engine engine;
    engine.prepare (kSampleRate);
    engine.setPatch (params);

    const int total = (int) (seconds * kSampleRate);
    std::vector<float> L ((size_t) total), R ((size_t) total);

    struct Ev { int pos; bool on; int pitch; float vel; };
    std::vector<Ev> events;
    for (auto& n : notes)
    {
        events.push_back ({ (int) (n.on * kSampleRate), true, n.pitch, n.velocity });
        events.push_back ({ (int) (n.off * kSampleRate), false, n.pitch, 0.0f });
    }
    std::stable_sort (events.begin(), events.end(), [] (const Ev& a, const Ev& b) { return a.pos < b.pos; });

    const int block = 256;
    size_t e = 0;
    for (int pos = 0; pos < total; pos += block)
    {
        const int end = std::min (total, pos + block);
        engine.setTransport (120.0, true, pos / kSampleRate * 2.0);
        int cur = pos;
        while (e < events.size() && events[e].pos < end)
        {
            const int at = std::max (cur, events[e].pos);
            if (at > cur)
                engine.render (&L[(size_t) cur], &R[(size_t) cur], at - cur);
            cur = at;
            if (events[e].on)
                engine.noteOn (events[e].pitch, events[e].vel);
            else
                engine.noteOff (events[e].pitch);
            ++e;
        }
        if (end > cur)
            engine.render (&L[(size_t) cur], &R[(size_t) cur], end - cur);
    }

    const float gain = std::pow (10.0f, params.output / 20.0f);
    Result res;
    double sumSq = 0.0;
    for (int i = 0; i < total; ++i)
    {
        for (float* s : { &L[(size_t) i], &R[(size_t) i] })
        {
            *s *= gain;
            if (! std::isfinite (*s))
                res.finite = false;
            res.peak = std::max (res.peak, (double) std::fabs (*s));
            sumSq += (double) *s * *s;
        }
    }
    res.rms = std::sqrt (sumSq / (2.0 * total));

    if (outL != nullptr) *outL = std::move (L);
    if (outR != nullptr) *outR = std::move (R);
    return res;
}

// A short phrase: a few single notes, then a held chord.
std::vector<Note> testPhrase (bool bassRange)
{
    const int root = bassRange ? 36 : 60;
    std::vector<Note> n;
    const int melody[] = { 0, 4, 7, 12 };
    double t = 0.0;
    for (int m : melody)
    {
        n.push_back ({ t, t + 0.35, root + m, 0.8f });
        t += 0.4;
    }
    for (int m : { 0, 7, 16 })
        n.push_back ({ 1.8, 4.0, root + m, 0.75f });
    return n;
}
} // namespace

int main (int argc, char** argv)
{
#ifdef HAS_SSE_FTZ
    // Flush denormals to zero, like the plugin does (juce::ScopedNoDenormals).
    _mm_setcsr (_mm_getcsr() | 0x8040);
#endif

    const std::string outDir = argc > 1 ? argv[1] : ".";
    bool ok = true;
    const auto t0 = std::chrono::steady_clock::now();
    double audioSeconds = 0.0;

    // 1. Every factory preset.
    std::printf ("%-24s %8s %8s  %s\n", "preset", "peak", "rms dB", "status");
    int index = 1;
    for (auto& preset : factoryPresets())
    {
        const bool bass = preset.name.find ("Bass") != std::string::npos || preset.name.find ("Sub") != std::string::npos
                          || preset.name.find ("Growl") != std::string::npos
                          || preset.name.find ("Split") != std::string::npos;
        std::vector<float> L, R;
        const Result res = render (preset.patch, testPhrase (bass), 7.0, &L, &R);
        audioSeconds += 7.0;

        const double rmsDb = 20.0 * std::log10 (std::max (1.0e-9, res.rms));
        const char* status = ! res.finite ? "FAIL (NaN/Inf)"
                           : res.peak < 0.02 ? "FAIL (too quiet)"
                           : res.peak > 3.0 ? "FAIL (too loud)"
                           : rmsDb < -40.0 ? "warn (quiet)" : "ok";
        if (status[0] == 'F')
            ok = false;
        std::printf ("%-24s %8.3f %8.1f  %s\n", preset.name.c_str(), res.peak, rmsDb, status);

        char file[256];
        std::snprintf (file, sizeof (file), "%s/%02d %s.wav", outDir.c_str(), index++, preset.name.c_str());
        writeWav (file, L, R);
    }

    // 2. Random patches (what the Randomize button makes).
    Rng rng;
    rng.seed (2024);
    int failures = 0, quiet = 0;
    double worstPeak = 0.0;
    const int numRandom = 300;
    for (int i = 0; i < numRandom; ++i)
    {
        const Patch p = randomPatch (rng);
        const Result res = render (p, testPhrase (false), 5.0);
        audioSeconds += 5.0;
        worstPeak = std::max (worstPeak, res.peak);
        if (! res.finite || res.peak > 4.0)
        {
            ++failures;
            std::printf ("random patch %d: %s (peak %.2f)\n", i, res.finite ? "too loud" : "NaN/Inf", res.peak);
        }
        if (res.peak < 0.01)
            ++quiet;
    }
    std::printf ("\nRandom patches: %d tested, %d failed, %d near-silent, loudest peak %.2f\n",
                 numRandom, failures, quiet, worstPeak);
    if (failures > 0 || quiet > numRandom / 20)
        ok = false;

    const double cpu = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
    std::printf ("Speed: %.1fx real time\n", audioSeconds / cpu);
    std::printf ("%s\n", ok ? "ALL PASSED" : "SOME FAILED");
    return ok ? 0 : 1;
}
