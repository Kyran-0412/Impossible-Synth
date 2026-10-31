# Impossible Synth

A physical-modelling synth for objects that can't exist. You hit, pluck, bow or blow into a virtual object, choose what it's shaped like and what it's made of, and then bend the physics: sounds that swell after they're struck, overtones that bend as they ring, glass that melts into rubber mid-note, objects that crack apart.

VST3 for Windows (works in FL Studio), plus a standalone app.

## Install (Windows)

1. Open the repo's **Actions** tab on GitHub and click the latest successful **Build** run.
2. Under **Artifacts**, download **Impossible-Synth-Windows** and unzip it.
3. Copy the **`Impossible Synth.vst3`** folder from `VST3/` into `C:\Program Files\Common Files\VST3`.
4. In FL Studio: **Options → Manage plugins → Find more plugins** (or **Start scan**), then add **Impossible Synth** to a channel.

The `Standalone/Impossible Synth.exe` runs on its own without a DAW. Set your audio device under **Options**.

The **Demo-Sounds** artifact has a WAV render of every factory preset, made by the automated build (dry, without reverb).

## Layers

Every sound has **4 layers**. Each layer is its own synth, with its own engine, filter, drive and modulation:

- **Physical**: hit, pluck, bow or blow into a virtual object (the original Impossible engine).
- **Wavetable**: scan through wavetables, with warps, unison and a sub oscillator.
- **Cells**: a Game of Life grid that plays itself. Living cells become overtones, and the sound changes as the patterns grow and die.
- **Swarm**: a flock of particles, each one a tiny oscillator, pulled toward "home" notes and knocked around by chaos.

Use the tabs at the top: **MIX** shows all four layers side by side, and **LAYER 1-4** open each layer's page. Click a tab's dot to switch that layer on or off. The **Engine** box (top right) changes the selected layer's engine.

**Mix page**, per layer: on/off, engine, **Volume**, **Pan**, **Fine** tune, **Octave**, **Key Low/High** (keyboard splits) and **Vel Low/High** (velocity switching), plus **Edit** to jump to its page.

**How layers link**
- **Macros**: the 4 Macro knobs at the bottom are sources in every layer's Matrix, so one knob can move several layers at once. For example, Macro 1 can fade one layer in while turning another layer's material from glass to wood.
- **Other engines into the Object**: set a Physical layer's **Exciter** to **Wavetable**, **Cells** or **Swarm**. That engine is then played *through* the object. Click **Edit Wavetable / Cells / Swarm** to set it up.
- **Splits and switches**: key and velocity ranges per layer.
- **Stacks**: switch on several layers and detune them with Fine or spread them with Pan.

Sounds and songs saved with earlier versions load into layer 1, unchanged.

## Wavetables

16 built-in tables: Basic Shapes, PWM, Saw Sync, Vowels, Harmonic Sweep, Organ, FM Growl, FM Bell, Wavefold, Steps, Chaos, Spectral Noise, Comb Saw, Bitwise, and two made from the Physical engine's own resonances: **Object Morph** (string → bell) and **Glass Spectrum** (rubber → glass).

**Import WAV** loads your own wavetables: Serum/Vital-style files with 2048 samples per frame (up to 256 frames), or any short single-cycle waveform. The file's location is saved with the sound, so keep the WAV where it is.

Wavetable page controls: **Table**, **Position** (scan through frames), **Warp** mode (Bend, Sync, Mirror, Squeeze, Fold, Crush, FM) and **Warp Amt**, **Unison** (up to 7), **Detune**, **Spread**, **Sub**, and an **Amp** envelope (Attack, Decay, Sustain, Release).

## Cells (Game of Life)

Each note starts its own grid, 32 columns by 16 rows, which keeps evolving while the note is held:

- **Columns are harmonics.** The more cells alive in column 1, the louder the fundamental; column 2 is the 2nd harmonic, and so on up to 32. A glider crawling across the grid becomes an overtone sweeping up the spectrum.
- **Rows are stereo.** Cells near the top lean left, cells near the bottom lean right.

Controls: **Rule** (Life, HighLife, Seeds, Day & Night, Maze, Anneal: each grows differently), **Seed** (the starting pattern: Random, Gliders, R-Pentomino, Acorn, Symmetric, Spaceships), **Density**, **Speed** (generations per second) or **Sync** (step in time with the song), **Glide** (smooth blend between generations, or hard steps at 0), **Tilt** (darker or brighter), **Mutate** (randomly flips cells so life never settles), **Sub**, and the **Amp** envelope. If every cell dies, the grid reseeds itself so the note keeps sounding. The display shows the newest note's grid live.

## Swarm (particles)

Up to 32 particles move around while the note is held. Each one is a small oscillator: **left/right in the display is pitch**, and **up/down is stereo position**.

Controls: **Particles** (4 to 32), **Home** (the notes they're drawn to: Unison, Harmonics, Octaves, Fifths, Chord, Cluster), **Pull** (gravity toward home: high gives a clean tone, low a free-floating cloud), **Chaos** (random kicks), **Flock** (right makes them clump together, left scatters them), **Spread** (squeeze the home notes together or stretch them apart; the middle is exact), **Speed**, **Wave** (Sine, Triangle, Saw), **Sub**, and the **Amp** envelope. Try a Mod Env or Macro on **Pull** to make a sound start as chaos and settle into a chord, or the reverse.

## Presets

The bar at the top has 51 factory presets: keys, plucks, pads, basses, leads, textures, wavetable, Cells and Swarm sounds, and layered stacks. Use **<** and **>** to step through them. FL Studio's own preset menu lists them too.

- **Save** stores the current sound in `Documents\Impossible Synth\Presets`. Saved sounds appear under **User** in the preset list.
- **Randomize** rolls a new random sound, often with two or three layers. It's tuned to give usable results rather than pure noise.

## Physical layer controls

**Excite**: how you play the object
- **Exciter**: Strike (mallet), Pluck, Bow, Breath, **Osc** (a normal synth oscillator fed into the object), or **Wavetable**, **Cells** or **Swarm** (that engine fed into the object). All except Strike and Pluck keep sounding while you hold the key.
- **Attack**: fade-in time for Bow, Breath and Osc.
- **Hardness**: mallet hardness / brightness. Key velocity adds to it.
- **Position**: where you hit the object. This changes which overtones ring.
- **Wave / Unison / Detune**: the Osc exciter's waveform (Sine → Triangle → Saw → Square), number of stacked oscillators (up to 7, for supersaws), and how far apart they're tuned.

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

**Filter & Drive** (all layer types): applied to each note
- **Filter**: Off, Low Pass, Band Pass, High Pass or Notch, with **Cutoff** and **Resonance**.
- **Env Amt**: how much the Mod Env sweeps the cutoff.
- **Drive**: distortion before the filter.
- **Body**: how much of the sound goes through the object. Turn it down to hear the raw oscillator, bow or breath. Useful for basses and leads.

**Modulation** (all layer types): make things move
- **LFO 1 / LFO 2**: Sine, Triangle, Saw Up/Down, Square, S&H (random steps) or Drift (smooth random). **Sync** locks the LFO to the song tempo. **Mode** is Free (all notes share one LFO) or Retrig (restarts with each note).
- **Mod Env**: an ADSR envelope that starts with each note.
- **Matrix**: six slots. Each one routes a **source** (LFO 1, LFO 2, Mod Env, Velocity, Mod Wheel, Note, Random, Macro 1-4) to a **target** (Pitch, Volume, Pan, any Excite/Object/Impossible knob, WT Position, WT Warp, Cell Speed/Tilt/Mutate, Swarm Pull/Chaos/Flock/Spread, Cutoff, Resonance, Drive, Body) with an **amount**. For example, LFO 1 → Material makes the object wobble between wood and glass.

**Global** (bottom strip)
- **Macro 1-4**: see *How layers link*.
- **Space**: reverb amount. **Room**: reverb size. **Output**: volume.

Double-click any knob to reset it. Hover over a control for a short explanation. Greyed-out controls don't affect the current settings. Sustain pedal, pitch bend (±2 semitones) and mod wheel work. Up to 12 notes at once per layer. Drag the bottom-right corner to resize the window.

## How it works

Every note is a bank of 32 resonators, one per mode (a natural vibration frequency of the object). Each mode is a pair of resonators so it can split apart for Fracture. Shape sets the modes' frequency ratios. Material sets how bright they start and how fast each one dies away. The exciter feeds energy in: an instant hit for Strike and Pluck, or a continuous signal for Bow, Breath and Osc. The Impossible controls and the modulation matrix change the mode frequencies, decay rates and loudness while the note is still ringing. Each note then has its own drive and filter.

Wavetable layers read band-limited copies of each frame (fewer harmonics for higher notes), so high notes don't alias.

The sound engine (`Source/dsp/`) is plain C++ with no JUCE dependency. The plugin wrapper and UI use [JUCE](https://juce.com) 8, which is downloaded automatically when you build.

## Building it yourself

You need CMake 3.22+ and a C++17 compiler (on Windows, Visual Studio 2022 with "Desktop development with C++").

```bash
cmake -B build
cmake --build build --config Release
```

The plugin ends up in `build/ImpossibleSynth_artefacts/Release/VST3/`.

`render_test` is an engine test that needs no DAW. It renders every preset to a WAV file, checks levels, and stress-tests 300 random patches:

```bash
./build/render_test demo-sounds        # Windows: build\Release\render_test.exe demo-sounds
```

With `-DIMPOSSIBLE_UI_SNAPSHOT=ON` two more developer tools are built: `state_test` (checks saving and loading, including old saves) and `ui_snapshot` (renders every UI tab to PNG files).

## License

Personal project. JUCE is used under the AGPLv3.
