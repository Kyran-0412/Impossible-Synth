// Offline test for the Impossible Synth engine (no JUCE, no DAW needed).
// Renders a set of patches to WAV files and prints peak/RMS levels, checks
// for NaN/Inf, and measures CPU speed.
//
// Build:  g++ -std=c++17 -O2 -ISource tools/render_test.cpp Source/dsp/Engine.cpp -o render_test
// Run:    ./render_test [output-folder]

#include "dsp/Engine.h"

#if defined(__SSE__) || defined(_M_X64)
 #include <xmmintrin.h>
 #include <pmmintrin.h>
 #define HAS_SSE_FTZ 1
#endif

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
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

struct Patch
{
    const char* name;
    Params params;
    std::vector<Note> notes;
    double seconds;
};

void writeWav (const std::string& path, const std::vector<float>& l, const std::vector<float>& r)
{
    FILE* f = std::fopen (path.c_str(), "wb");
    if (f == nullptr)
        return;

    const uint32_t frames = (uint32_t) l.size();
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
            const float c = std::tanh (s); // same safety clip as the plugin
            const int16_t v = (int16_t) std::lrint (c * 32767.0f);
            std::fwrite (&v, 2, 1, f);
        }
    }
    std::fclose (f);
}

Params base()
{
    return Params {};
}
} // namespace

int main (int argc, char** argv)
{
#ifdef HAS_SSE_FTZ
    // Flush denormals to zero, like the plugin does (juce::ScopedNoDenormals).
    _mm_setcsr (_mm_getcsr() | 0x8040);
#endif

    const std::string outDir = argc > 1 ? argv[1] : ".";
    std::vector<Patch> patches;

    auto chord = [] (double on, double off, std::initializer_list<int> pitches, float vel)
    {
        std::vector<Note> n;
        for (int p : pitches)
            n.push_back ({ on, off, p, vel });
        return n;
    };
    auto melody = [] (std::initializer_list<int> pitches, double step, float vel)
    {
        std::vector<Note> n;
        double t = 0.0;
        for (int p : pitches)
        {
            n.push_back ({ t, t + step * 0.8, p, vel });
            t += step;
        }
        return n;
    };

    {
        Params p = base();
        p.shape = 0.5f; // bar
        p.material = 0.33f; // wood
        patches.push_back ({ "01_wood_bar_marimba", p, melody ({ 60, 64, 67, 72, 67, 64, 60 }, 0.3, 0.8f), 4.0 });
    }
    {
        Params p = base();
        p.shape = 1.0f; // bell
        p.material = 0.66f; // metal
        patches.push_back ({ "02_metal_bell", p, chord (0.0, 1.0, { 60 }, 0.9f), 8.0 });
    }
    {
        Params p = base();
        p.shape = 0.0f;
        p.material = 0.5f;
        p.exciter = kPluck;
        patches.push_back ({ "03_plucked_string", p, melody ({ 48, 55, 60, 64, 67 }, 0.4, 0.8f), 5.0 });
    }
    {
        Params p = base();
        p.shape = 0.0f;
        p.material = 0.4f;
        p.exciter = kBow;
        p.hardness = 0.6f;
        p.release = 0.4f;
        patches.push_back ({ "04_bowed_string", p, chord (0.0, 3.0, { 48, 55, 64 }, 0.8f), 5.0 });
    }
    {
        Params p = base();
        p.shape = 0.25f; // tube
        p.material = 0.9f;
        p.exciter = kBreath;
        p.hardness = 0.4f;
        patches.push_back ({ "05_breath_tube", p, chord (0.0, 3.0, { 57, 64 }, 0.8f), 5.0 });
    }
    {
        Params p = base();
        p.shape = 0.75f; // plate
        p.material = 1.0f; // glass
        p.bloom = 1.0f;
        p.evolve = 1.5f;
        patches.push_back ({ "06_bloom_glass_plate", p, chord (0.0, 4.0, { 60, 67 }, 0.8f), 8.0 });
    }
    {
        Params p = base();
        p.shape = 1.0f;
        p.material = 0.7f;
        p.drift = -1.0f;
        p.evolve = 2.0f;
        patches.push_back ({ "07_drift_down_bell", p, chord (0.0, 4.0, { 64 }, 0.9f), 8.0 });
    }
    {
        Params p = base();
        p.shape = 0.5f;
        p.material = 1.0f; // glass...
        p.transmute = -1.0f; // ...melting into rubber
        p.evolve = 1.0f;
        patches.push_back ({ "08_transmute_glass_to_rubber", p, chord (0.0, 4.0, { 60 }, 0.9f), 6.0 });
    }
    {
        Params p = base();
        p.shape = 0.9f;
        p.material = 0.7f;
        p.fracture = 1.0f;
        p.evolve = 1.0f;
        patches.push_back ({ "09_fracture_metal", p, chord (0.0, 4.0, { 55 }, 0.9f), 8.0 });
    }
    {
        Params p = base();
        p.shape = 0.0f;
        p.material = 0.8f;
        p.size = 1.0f; // huge
        p.decay = 3.0f;
        patches.push_back ({ "10_huge_string", p, chord (0.0, 6.0, { 36 }, 1.0f), 10.0 });
    }
    {
        // Stress test: hammer 24 notes quickly to exercise voice stealing.
        Params p = base();
        p.shape = 0.66f;
        p.material = 0.8f;
        std::vector<Note> n;
        for (int i = 0; i < 24; ++i)
            n.push_back ({ i * 0.08, i * 0.08 + 2.0, 40 + (i * 7) % 48, 1.0f });
        patches.push_back ({ "11_voice_stealing_stress", p, n, 6.0 });
    }
    {
        // Extremes: highest notes, max everything.
        Params p = base();
        p.shape = 1.0f;
        p.material = 1.0f;
        p.hardness = 1.0f;
        p.bloom = 1.0f;
        p.drift = 1.0f;
        p.fracture = 1.0f;
        p.size = 0.0f;
        p.octave = 2;
        patches.push_back ({ "12_extremes", p, chord (0.0, 2.0, { 96, 108, 120, 127 }, 1.0f), 4.0 });
    }

    bool ok = true;
    double totalAudio = 0.0, totalCpu = 0.0;

    std::printf ("%-32s %8s %8s %8s %s\n", "patch", "peak", "rms", "x-real", "status");
    for (auto& patch : patches)
    {
        Engine engine;
        engine.prepare (kSampleRate);
        engine.setParams (patch.params);

        const int total = (int) (patch.seconds * kSampleRate);
        std::vector<float> L ((size_t) total), R ((size_t) total);

        // Build a simple event list at sample positions.
        struct Ev { int pos; bool on; int pitch; float vel; };
        std::vector<Ev> events;
        for (auto& n : patch.notes)
        {
            events.push_back ({ (int) (n.on * kSampleRate), true, n.pitch, n.velocity });
            events.push_back ({ (int) (n.off * kSampleRate), false, n.pitch, 0.0f });
        }
        std::sort (events.begin(), events.end(), [] (const Ev& a, const Ev& b) { return a.pos < b.pos; });

        const auto t0 = std::chrono::steady_clock::now();
        const int block = 256;
        size_t e = 0;
        for (int pos = 0; pos < total; pos += block)
        {
            const int end = std::min (total, pos + block);
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
        const double cpu = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        totalCpu += cpu;
        totalAudio += patch.seconds;

        double peak = 0.0, sumSq = 0.0;
        bool finite = true;
        for (int i = 0; i < total; ++i)
        {
            for (float s : { L[(size_t) i], R[(size_t) i] })
            {
                if (! std::isfinite (s))
                    finite = false;
                peak = std::max (peak, (double) std::fabs (s));
                sumSq += (double) s * s;
            }
        }
        const double rms = std::sqrt (sumSq / (2.0 * total));
        const bool silent = peak < 1.0e-3;
        const bool tooLoud = peak > 4.0;
        const bool pass = finite && ! silent && ! tooLoud;
        ok = ok && pass;

        std::printf ("%-32s %8.3f %8.4f %8.1f %s\n", patch.name, peak, rms, patch.seconds / cpu,
                     ! finite ? "FAIL (NaN/Inf)" : silent ? "FAIL (silent)" : tooLoud ? "FAIL (too loud)" : "ok");

        writeWav (outDir + "/" + patch.name + ".wav", L, R);
    }

    std::printf ("\nOverall speed: %.1fx real time\n", totalAudio / totalCpu);
    std::printf ("%s\n", ok ? "ALL PASSED" : "SOME FAILED");
    return ok ? 0 : 1;
}
