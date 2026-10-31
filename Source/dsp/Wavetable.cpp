#include "Wavetable.h"
#include "ModeTables.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <functional>

namespace impossible
{

namespace
{
constexpr double kPi = 3.14159265358979323846;
using Complex = std::complex<double>;

// In-place radix-2 FFT. inverse = true uses e^{+i...} and does not scale.
void fft (std::vector<Complex>& a, bool inverse)
{
    const size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i)
    {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j)
            std::swap (a[i], a[j]);
    }

    for (size_t len = 2; len <= n; len <<= 1)
    {
        const double angle = 2.0 * kPi / (double) len * (inverse ? 1.0 : -1.0);
        const Complex step (std::cos (angle), std::sin (angle));
        for (size_t i = 0; i < n; i += len)
        {
            Complex w (1.0, 0.0);
            for (size_t j = 0; j < len / 2; ++j)
            {
                const Complex u = a[i + j];
                const Complex v = a[i + j + len / 2] * w;
                a[i + j] = u + v;
                a[i + j + len / 2] = u - v;
                w *= step;
            }
        }
    }
}
} // namespace

//==============================================================================
int Wavetable::levelFor (float maxHarmonics)
{
    int level = 0;
    while (level < kNumLevels - 1 && (float) (512 >> level) > maxHarmonics)
        ++level;
    return level;
}

std::shared_ptr<Wavetable> Wavetable::fromFrames (const std::vector<std::vector<float>>& input,
                                                  std::string name, int pitchDivisor)
{
    if (input.empty())
        return nullptr;

    auto wt = std::make_shared<Wavetable>();
    wt->frames = (int) std::min (input.size(), (size_t) kMaxFrames);
    wt->divisor = std::max (1, pitchDivisor);
    wt->label = std::move (name);

    int offset = 0;
    for (int l = 0; l < kNumLevels; ++l)
    {
        wt->sizes[(size_t) l] = std::max (64, 4 * (512 >> l));
        wt->offsets[(size_t) l] = offset;
        offset += wt->sizes[(size_t) l] + 1; // +1 guard sample (copy of sample 0) for interpolation
    }
    wt->stride = offset;
    wt->data.assign ((size_t) (wt->stride * wt->frames), 0.0f);

    std::vector<Complex> spectrum ((size_t) kFrameSize);
    for (int f = 0; f < wt->frames; ++f)
    {
        const auto& src = input[(size_t) f];
        for (int n = 0; n < kFrameSize; ++n)
            spectrum[(size_t) n] = Complex (n < (int) src.size() ? (double) src[(size_t) n] : 0.0, 0.0);
        fft (spectrum, false);

        for (int l = 0; l < kNumLevels; ++l)
        {
            const int size = wt->sizes[(size_t) l];
            const int harmonics = std::min (512 >> l, size / 2 - 1);
            std::vector<Complex> level ((size_t) size);
            for (int k = 1; k <= harmonics; ++k)
            {
                level[(size_t) k] = spectrum[(size_t) k] / (double) kFrameSize;
                level[(size_t) (size - k)] = std::conj (level[(size_t) k]);
            }
            fft (level, true);

            float* dst = wt->data.data() + f * wt->stride + wt->offsets[(size_t) l];
            for (int n = 0; n < size; ++n)
                dst[n] = (float) level[(size_t) n].real();
            dst[size] = dst[0];
        }
    }

    // Normalise to a peak of 1, then lift thin-sounding tables (low RMS) by up
    // to 2x so tables have similar loudness.
    float peak = 0.0f;
    double sumSq = 0.0;
    for (int f = 0; f < wt->frames; ++f)
    {
        const float* d = wt->frameData (f, 0);
        for (int n = 0; n < wt->sizes[0]; ++n)
        {
            peak = std::max (peak, std::fabs (d[n]));
            sumSq += (double) d[n] * d[n];
        }
    }
    if (peak > 1.0e-9f)
    {
        const double rms = std::sqrt (sumSq / (wt->frames * wt->sizes[0])) / peak;
        const float gain = (float) (std::clamp (0.45 / std::max (rms, 1.0e-6), 1.0, 2.0) / peak);
        for (auto& s : wt->data)
            s *= gain;
    }

    return wt;
}

std::shared_ptr<Wavetable> Wavetable::fromAudio (const float* samples, int numSamples, std::string name)
{
    if (samples == nullptr || numSamples < 8)
        return nullptr;

    std::vector<std::vector<float>> frames;

    if (numSamples < kFrameSize || (numSamples % kFrameSize != 0 && numSamples < 2 * kFrameSize))
    {
        // A short file: treat it as one cycle and stretch it to a full frame.
        std::vector<float> frame ((size_t) kFrameSize);
        for (int n = 0; n < kFrameSize; ++n)
        {
            const double pos = (double) n * numSamples / kFrameSize;
            const int i = (int) pos;
            const double t = pos - i;
            const float a = samples[i % numSamples];
            const float b = samples[(i + 1) % numSamples];
            frame[(size_t) n] = (float) (a + (b - a) * t);
        }
        frames.push_back (std::move (frame));
    }
    else
    {
        const int count = std::min (numSamples / kFrameSize, kMaxFrames);
        for (int f = 0; f < count; ++f)
            frames.emplace_back (samples + f * kFrameSize, samples + (f + 1) * kFrameSize);
    }

    return fromFrames (frames, std::move (name));
}

const float* Wavetable::frameData (int frame, int level) const
{
    frame = std::clamp (frame, 0, frames - 1);
    return data.data() + frame * stride + offsets[(size_t) level];
}

float Wavetable::read (float framePos, int level, float phase) const
{
    const float maxFrame = (float) (frames - 1);
    framePos = framePos < 0.0f ? 0.0f : (framePos > maxFrame ? maxFrame : framePos);
    const int f0 = (int) framePos;
    const int f1 = std::min (f0 + 1, frames - 1);
    const float ft = framePos - (float) f0;

    const int size = sizes[(size_t) level];
    const float pos = phase * (float) size;
    int i = (int) pos;
    if (i >= size) i = size - 1;
    if (i < 0) i = 0;
    const float t = pos - (float) i;

    const float* a = data.data() + f0 * stride + offsets[(size_t) level];
    const float sa = a[i] + (a[i + 1] - a[i]) * t;
    if (f1 == f0 || ft < 1.0e-4f)
        return sa;

    const float* b = data.data() + f1 * stride + offsets[(size_t) level];
    const float sb = b[i] + (b[i + 1] - b[i]) * t;
    return sa + (sb - sa) * ft;
}

//==============================================================================
namespace
{
constexpr int kBuiltInFrames = 32;
using Shape = std::function<double (double t, double x)>; // t = frame position 0..1, x = phase 0..1

std::shared_ptr<Wavetable> fromFunction (const char* name, const Shape& fn, int divisor = 1)
{
    std::vector<std::vector<float>> frames;
    for (int f = 0; f < kBuiltInFrames; ++f)
    {
        const double t = (double) f / (kBuiltInFrames - 1);
        std::vector<float> frame ((size_t) Wavetable::kFrameSize);
        for (int n = 0; n < Wavetable::kFrameSize; ++n)
            frame[(size_t) n] = (float) fn (t, (double) n / Wavetable::kFrameSize);
        frames.push_back (std::move (frame));
    }
    return Wavetable::fromFrames (frames, name, divisor);
}

// Additive: amplitude of harmonic k (1-based) at frame position t, all sine phase.
using Partials = std::function<double (double t, int k)>;

std::shared_ptr<Wavetable> fromPartials (const char* name, int maxHarmonic, const Partials& amp, int divisor = 1)
{
    std::vector<std::vector<float>> frames;
    std::vector<Complex> spectrum ((size_t) Wavetable::kFrameSize);
    for (int f = 0; f < kBuiltInFrames; ++f)
    {
        const double t = (double) f / (kBuiltInFrames - 1);
        std::fill (spectrum.begin(), spectrum.end(), Complex());
        for (int k = 1; k <= std::min (maxHarmonic, 1000); ++k)
        {
            const double a = amp (t, k);
            spectrum[(size_t) k] = Complex (0.0, -0.5 * a);
            spectrum[(size_t) (Wavetable::kFrameSize - k)] = Complex (0.0, 0.5 * a);
        }
        fft (spectrum, true);
        std::vector<float> frame ((size_t) Wavetable::kFrameSize);
        for (int n = 0; n < Wavetable::kFrameSize; ++n)
            frame[(size_t) n] = (float) spectrum[(size_t) n].real();
        frames.push_back (std::move (frame));
    }
    return Wavetable::fromFrames (frames, name, divisor);
}

double frac (double x) { return x - std::floor (x); }

double basicShape (int which, double x)
{
    switch (which)
    {
        case 0: return std::sin (2.0 * kPi * x);
        case 1: return 1.0 - 4.0 * std::fabs (frac (x + 0.25) - 0.5);
        case 2: return 2.0 * frac (x + 0.5) - 1.0;
        default: return x < 0.5 ? 1.0 : -1.0;
    }
}

// Smooth random value for integer i (deterministic).
double hashUnit (uint32_t i)
{
    i ^= i >> 16; i *= 0x7feb352du; i ^= i >> 15; i *= 0x846ca68bu; i ^= i >> 16;
    return (double) (i >> 8) / 16777216.0;
}

double formantAmp (double freq, const double (&f)[3])
{
    const double gains[3] = { 1.0, 0.5, 0.3 };
    const double widths[3] = { 90.0, 110.0, 150.0 };
    double a = 0.0;
    for (int i = 0; i < 3; ++i)
    {
        const double d = (freq - f[i]) / widths[i];
        a += gains[i] / (1.0 + d * d);
    }
    return a;
}

std::vector<std::shared_ptr<Wavetable>> makeBuiltIns()
{
    std::vector<std::shared_ptr<Wavetable>> t;

    t.push_back (fromFunction ("Basic Shapes", [] (double tt, double x)
    {
        const double pos = tt * 3.0;
        const int i = std::min ((int) pos, 2);
        const double f = pos - i;
        return basicShape (i, x) * (1.0 - f) + basicShape (i + 1, x) * f;
    }));

    t.push_back (fromFunction ("PWM", [] (double tt, double x)
    {
        const double width = 0.5 - 0.47 * tt;
        return x < width ? 1.0 : -1.0;
    }));

    t.push_back (fromFunction ("Saw Sync", [] (double tt, double x)
    {
        const double ratio = 1.0 + 7.0 * tt;
        return (2.0 * frac (x * ratio) - 1.0) * (1.0 - x * 0.3);
    }));

    t.push_back (fromPartials ("Vowels", 120, [] (double tt, int k)
    {
        static const double vowels[5][3] = { { 800, 1150, 2900 }, { 350, 2000, 2800 }, { 270, 2140, 2950 },
                                             { 450, 800, 2830 }, { 325, 700, 2700 } };
        const double pos = tt * 4.0;
        const int i = std::min ((int) pos, 3);
        const double f = pos - i;
        const double freq = 110.0 * k;
        const double a = formantAmp (freq, vowels[i]) * (1.0 - f) + formantAmp (freq, vowels[i + 1]) * f;
        return a / std::sqrt ((double) k);
    }));

    t.push_back (fromPartials ("Harmonic Sweep", 64, [] (double tt, int k)
    {
        const double centre = 1.0 + 47.0 * tt;
        const double d = (k - centre) / 2.0;
        return std::exp (-d * d) + 0.25 / k;
    }));

    t.push_back (fromPartials ("Organ", 16, [] (double tt, int k)
    {
        static const double regs[4][8] = { { 1, 1, 0.8, 0, 0, 0, 0, 0 },          // flute
                                           { 1, 0.8, 0.6, 0.6, 0.4, 0.3, 0.2, 0.2 }, // full
                                           { 0.6, 0, 1, 0, 0.8, 0, 0.5, 0 },        // hollow
                                           { 1, 0.2, 0.2, 0.9, 0.1, 0.6, 0.1, 0.7 } }; // bright
        static const int harmonicFor[8] = { 1, 2, 3, 4, 5, 6, 8, 16 };
        const double pos = tt * 3.0;
        const int i = std::min ((int) pos, 2);
        const double f = pos - i;
        for (int d = 0; d < 8; ++d)
            if (harmonicFor[d] == k)
                return regs[i][d] * (1.0 - f) + regs[i + 1][d] * f;
        return 0.0;
    }));

    t.push_back (fromFunction ("FM Growl", [] (double tt, double x)
    {
        const double index = 8.0 * tt;
        return std::sin (2.0 * kPi * x + index * std::sin (2.0 * kPi * x));
    }));

    t.push_back (fromFunction ("FM Bell", [] (double tt, double x)
    {
        const double index = 0.2 + 4.0 * tt;
        return std::sin (2.0 * kPi * x + index * std::sin (2.0 * kPi * 7.0 * x) + 0.5 * index * std::sin (2.0 * kPi * 3.0 * x));
    }));

    t.push_back (fromFunction ("Wavefold", [] (double tt, double x)
    {
        const double gain = 1.0 + 11.0 * tt;
        return std::sin (gain * std::sin (2.0 * kPi * x));
    }));

    t.push_back (fromFunction ("Steps", [] (double tt, double x)
    {
        const double levels = std::round (32.0 * std::pow (2.0 / 32.0, tt));
        const double s = std::sin (2.0 * kPi * x);
        return std::round (s * levels) / levels;
    }));

    t.push_back (fromFunction ("Chaos", [] (double tt, double x)
    {
        // A wave traced by the logistic map, which turns chaotic as r rises.
        const double r = 3.5 + 0.5 * tt;
        double v = 0.4;
        double pts[16];
        for (double& p : pts)
        {
            v = r * v * (1.0 - v);
            p = v * 2.0 - 1.0;
        }
        const double pos = x * 16.0;
        const int i = (int) pos % 16;
        const double f = pos - std::floor (pos);
        const double s = 0.5 - 0.5 * std::cos (kPi * f);
        return pts[i] * (1.0 - s) + pts[(i + 1) % 16] * s;
    }));

    t.push_back (fromPartials ("Spectral Noise", 200, [] (double tt, int k)
    {
        const double pos = tt * 3.0;
        const int i = std::min ((int) pos, 2);
        const double f = pos - i;
        const double a = hashUnit ((uint32_t) (k * 4 + i)), b = hashUnit ((uint32_t) (k * 4 + i + 1));
        return (a * (1.0 - f) + b * f) / std::pow ((double) k, 0.8);
    }));

    // "Impossible" tables built from the Physical engine's own mode spectra.
    // The table runs 4x below the note so inharmonic modes land within an eighth
    // of a semitone-ish grid.
    t.push_back (fromPartials ("Object Morph", 400, [] (double tt, int k)
    {
        double a = 0.0;
        for (int m = 0; m < kNumModes; ++m)
        {
            const double ratio = shapeRatio ((float) tt, m);
            if ((int) std::lround (ratio * 4.0) == k)
                a += std::pow (ratio, -0.6);
        }
        return a;
    }, 4));

    t.push_back (fromPartials ("Glass Spectrum", 400, [] (double tt, int k)
    {
        const MaterialProps mp = materialAt ((float) tt);
        double a = 0.0;
        for (int m = 0; m < kNumModes; ++m)
        {
            const double ratio = shapeRatio (1.0f, m) * std::sqrt (1.0 + mp.stiff * (m + 1) * (m + 1));
            if ((int) std::lround (ratio * 4.0) == k)
                a += std::pow (ratio, -(double) mp.beta);
        }
        return a;
    }, 4));

    t.push_back (fromPartials ("Comb Saw", 300, [] (double tt, int k)
    {
        const double d = 0.02 + 0.3 * tt;
        return std::fabs (std::cos (kPi * k * d)) / k;
    }));

    t.push_back (fromFunction ("Bitwise", [] (double tt, double x)
    {
        const int a = (int) (x * 256.0);
        const int b = (int) (x * 256.0 * (1.0 + 7.0 * tt));
        return (double) ((a ^ b) & 255) / 127.5 - 1.0;
    }));

    return t;
}
} // namespace

const std::vector<std::shared_ptr<Wavetable>>& builtInWavetables()
{
    static const std::vector<std::shared_ptr<Wavetable>> tables = makeBuiltIns();
    return tables;
}

} // namespace impossible
