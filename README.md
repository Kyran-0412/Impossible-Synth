# Impossible Synth

A physical-modelling synth for objects that can't exist. You hit, pluck, bow or blow into a virtual object, choose what it's shaped like and what it's made of, and then bend the physics: sounds that swell after they're struck, overtones that bend as they ring, glass that melts into rubber mid-note, objects that crack apart.

VST3 for Windows (works in FL Studio), plus a standalone app.

## Install (Windows)

1. Open the repo's **Actions** tab on GitHub and click the latest successful **Build** run.
2. Under **Artifacts**, download **Impossible-Synth-Windows** and unzip it.
3. Copy the **`Impossible Synth.vst3`** folder from `VST3/` into `C:\Program Files\Common Files\VST3`.
4. In FL Studio: **Options → Manage plugins → Find more plugins** (or **Start scan**), then add **Impossible Synth** to a channel.

The `Standalone/Impossible Synth.exe` runs on its own without a DAW. Set your audio device under **Options**.

The **Demo-Sounds** artifact has WAV renders of a few test patches, made by the automated build.

## Controls

**Excite**: how you play the object
- **Exciter**: Strike (mallet), Pluck, Bow (sustains while held) or Breath (sustains while held).
- **Hardness**: mallet hardness / brightness. Key velocity adds to it.
- **Position**: where you hit the object. This changes which overtones ring.

**Object**: what you're playing
- **Shape**: morphs String → Tube → Bar → Plate → Bell.
- **Material**: morphs Rubber → Wood → Metal → Glass.
- **Size**: tiny to huge. Bigger objects ring longer, deeper and less perfectly.
- **Decay**: multiplies the ring time.
- **Release**: how fast the sound is damped after you let go of the key.
- **Octave**: transposes by -2 to +2 octaves.

**Impossible**: physics that shouldn't work
- **Bloom**: the sound swells louder after the hit instead of only fading.
- **Drift**: overtones bend down (left) or up (right) as the note rings.
- **Transmute**: the material changes during the note. Left heads toward rubber, right toward glass.
- **Fracture**: overtones split apart and beat against each other, like the object cracking.
- **Evolve**: how fast Bloom, Drift, Transmute and Fracture happen.

**Space**
- **Space**: reverb amount. **Room**: reverb size. **Output**: volume.

Double-click any knob to reset it. Sustain pedal and pitch bend (±2 semitones) work. Up to 12 notes at once.

## How it works

Every note is a bank of 32 resonators, one per mode (a natural vibration frequency of the object). Each mode is a pair of resonators so it can split apart for Fracture. Shape sets the modes' frequency ratios. Material sets how bright they start and how fast each one dies away. The exciter feeds energy in: an instant hit for Strike and Pluck, or a continuous signal for Bow and Breath. The Impossible controls change the mode frequencies, decay rates and loudness while the note is still ringing.

The sound engine (`Source/dsp/`) is plain C++ with no JUCE dependency. The plugin wrapper and UI use [JUCE](https://juce.com) 8, which is downloaded automatically when you build.

## Building it yourself

You need CMake 3.22+ and a C++17 compiler (on Windows, Visual Studio 2022 with "Desktop development with C++").

```bash
cmake -B build
cmake --build build --config Release
```

The plugin ends up in `build/ImpossibleSynth_artefacts/Release/VST3/`.

`render_test` is an engine test that needs no DAW. It renders test patches to WAV files and checks their levels:

```bash
./build/render_test demo-sounds        # Windows: build\Release\render_test.exe demo-sounds
```

## License

Personal project. JUCE is used under the AGPLv3.
