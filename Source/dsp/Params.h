#pragma once

// Every synth setting, in one place. Plain C++ (no JUCE) so the engine,
// presets and offline tests all share it. The plugin builds its parameter
// list from the IDs, ranges and defaults defined here.
//
// A Patch has 4 layers. Each layer is a Physical (modal), Wavetable, Cells
// (cellular automaton) or Swarm (particle) engine, with its own filter, drive and modulation. Plugin
// parameter IDs for layer settings are prefixed: "L1_shape", "L2_wtPos", ...

#include <array>
#include <cmath>

namespace impossible
{

constexpr int kNumLayers = 4;
constexpr int kNumLfos = 2;
constexpr int kNumModSlots = 6;
constexpr int kNumMacros = 4;
constexpr int kMaxUnison = 7;

enum LayerType { kLayerPhysical, kLayerWavetable, kLayerCells, kLayerSwarm, kNumLayerTypes };
enum Exciter { kStrike, kPluck, kBow, kBreath, kOsc, kExcWavetable, kExcCells, kExcSwarm, kNumExciters };
enum FilterMode { kFilterOff, kFilterLP, kFilterBP, kFilterHP, kFilterNotch, kNumFilterModes };
enum LfoShape { kLfoSine, kLfoTriangle, kLfoSawUp, kLfoSawDown, kLfoSquare, kLfoSampleHold, kLfoDrift, kNumLfoShapes };
enum WarpMode { kWarpOff, kWarpBend, kWarpSync, kWarpMirror, kWarpSqueeze, kWarpFold, kWarpCrush, kWarpFM, kNumWarpModes };
enum CellRule { kRuleLife, kRuleHighLife, kRuleSeeds, kRuleDayNight, kRuleMaze, kRuleAnneal, kNumCellRules };
enum CellSeed { kSeedRandom, kSeedGliders, kSeedRPentomino, kSeedAcorn, kSeedSymmetric, kSeedSpaceships, kNumCellSeeds };
enum SwarmHome { kHomeUnison, kHomeHarmonics, kHomeOctaves, kHomeFifths, kHomeChord, kHomeCluster, kNumSwarmHomes };

constexpr int kCellRows = 16;    // grid height: rows set stereo position
constexpr int kCellCols = 32;    // grid width: column x drives harmonic x + 1
constexpr int kMaxParticles = 32;

// New entries are only ever appended, so older presets keep their meaning.
enum ModSource
{
    kSrcOff, kSrcLfo1, kSrcLfo2, kSrcEnv, kSrcVelocity, kSrcModWheel, kSrcNote, kSrcRandom,
    kSrcMacro1, kSrcMacro2, kSrcMacro3, kSrcMacro4,
    kNumSources
};

enum ModTarget
{
    kDstOff, kDstPitch, kDstVolume,
    kDstHardness, kDstPosition, kDstShape, kDstMaterial, kDstSize, kDstDecay,
    kDstBloom, kDstDrift, kDstTransmute, kDstFracture,
    kDstOscWave, kDstDetune, kDstBody, kDstCutoff, kDstResonance, kDstDrive,
    kDstWtPos, kDstWtWarp, kDstPan,
    kDstCellSpeed, kDstCellTilt, kDstCellMutate,
    kDstSwarmPull, kDstSwarmChaos, kDstSwarmFlock, kDstSwarmSpread,
    kNumTargets
};

inline const char* const kLayerTypeNames[kNumLayerTypes] = { "Physical", "Wavetable", "Cells", "Swarm" };
inline const char* const kOnOffNames[2] = { "Off", "On" };
inline const char* const kExciterNames[kNumExciters] = { "Strike", "Pluck", "Bow", "Breath", "Osc", "Wavetable", "Cells", "Swarm" };
inline const char* const kFilterModeNames[kNumFilterModes] = { "Off", "Low Pass", "Band Pass", "High Pass", "Notch" };
inline const char* const kLfoShapeNames[kNumLfoShapes] = { "Sine", "Triangle", "Saw Up", "Saw Down", "Square", "S&H", "Drift" };
inline const char* const kWarpNames[kNumWarpModes] = { "Off", "Bend", "Sync", "Mirror", "Squeeze", "Fold", "Crush", "FM" };
inline const char* const kCellRuleNames[kNumCellRules] = { "Life", "HighLife", "Seeds", "Day & Night", "Maze", "Anneal" };
inline const char* const kCellSeedNames[kNumCellSeeds] = { "Random", "Gliders", "R-Pentomino", "Acorn", "Symmetric", "Spaceships" };
inline const char* const kSwarmHomeNames[kNumSwarmHomes] = { "Unison", "Harmonics", "Octaves", "Fifths", "Chord", "Cluster" };
inline const char* const kParticleCountNames[4] = { "4", "8", "16", "32" };
inline const char* const kSourceNames[kNumSources] = {
    "Off", "LFO 1", "LFO 2", "Mod Env", "Velocity", "Mod Wheel", "Note", "Random",
    "Macro 1", "Macro 2", "Macro 3", "Macro 4"
};
inline const char* const kTargetNames[kNumTargets] = {
    "Off", "Pitch", "Volume", "Hardness", "Position", "Shape", "Material", "Size", "Decay",
    "Bloom", "Drift", "Transmute", "Fracture", "Osc Wave", "Detune", "Body", "Cutoff", "Resonance", "Drive",
    "WT Position", "WT Warp", "Pan",
    "Cell Speed", "Cell Tilt", "Cell Mutate",
    "Swarm Pull", "Swarm Chaos", "Swarm Flock", "Swarm Spread"
};
inline const char* const kOctaveNames[5] = { "-2", "-1", "0", "+1", "+2" };
inline const char* const kUnisonNames[kMaxUnison] = { "1", "2", "3", "4", "5", "6", "7" };
inline const char* const kRetrigNames[2] = { "Free", "Retrig" };

// LFO tempo sync: "Free" uses the Rate knob, the rest lock to the host tempo.
constexpr int kNumSyncOptions = 13;
inline const char* const kSyncNames[kNumSyncOptions] = {
    "Free", "4 bars", "2 bars", "1 bar", "1/2", "1/4", "1/8", "1/16", "1/32", "1/4 T", "1/8 T", "1/16 T", "1/8 D"
};
inline const double kSyncBeats[kNumSyncOptions] = {
    0.0, 16.0, 8.0, 4.0, 2.0, 1.0, 0.5, 0.25, 0.125, 2.0 / 3.0, 1.0 / 3.0, 1.0 / 6.0, 0.75
};

// A value range. With centre > min, the knob is skewed so the centre value sits
// in the middle (same maths as JUCE's NormalisableRange::setSkewForCentre).
struct Range
{
    float min, max, centre = -1.0f;

    bool skewed() const { return centre > min; }
    float skew() const { return std::log (0.5f) / std::log ((centre - min) / (max - min)); }

    float toNorm (float v) const
    {
        const float n = (v - min) / (max - min);
        if (n <= 0.0f) return 0.0f;
        if (n >= 1.0f) return 1.0f;
        return skewed() ? std::exp (std::log (n) * skew()) : n;
    }

    float fromNorm (float n) const
    {
        n = n < 0.0f ? 0.0f : (n > 1.0f ? 1.0f : n);
        if (skewed() && n > 0.0f)
            n = std::exp (std::log (n) / skew());
        return min + (max - min) * n;
    }
};

namespace ranges
{
inline constexpr Range hardness { 0.0f, 1.0f };
inline constexpr Range position { 0.02f, 0.5f };
inline constexpr Range attack { 0.001f, 10.0f, 0.3f };
inline constexpr Range oscWave { 0.0f, 1.0f };
inline constexpr Range detune { 0.0f, 1.0f };
inline constexpr Range shape { 0.0f, 1.0f };
inline constexpr Range material { 0.0f, 1.0f };
inline constexpr Range size { 0.0f, 1.0f };
inline constexpr Range decay { 0.1f, 10.0f, 1.0f };
inline constexpr Range release { 0.02f, 10.0f, 1.0f };
inline constexpr Range bloom { 0.0f, 1.0f };
inline constexpr Range drift { -1.0f, 1.0f };
inline constexpr Range transmute { -1.0f, 1.0f };
inline constexpr Range fracture { 0.0f, 1.0f };
inline constexpr Range evolve { 0.05f, 10.0f, 1.5f };
inline constexpr Range cutoff { 20.0f, 20000.0f, 1000.0f };
inline constexpr Range resonance { 0.0f, 1.0f };
inline constexpr Range filterEnv { -1.0f, 1.0f };
inline constexpr Range drive { 0.0f, 1.0f };
inline constexpr Range body { 0.0f, 1.0f };
inline constexpr Range lfoRate { 0.01f, 40.0f, 2.0f };
inline constexpr Range envAttack { 0.001f, 10.0f, 0.3f };
inline constexpr Range envDecay { 0.005f, 10.0f, 0.5f };
inline constexpr Range envSustain { 0.0f, 1.0f };
inline constexpr Range envRelease { 0.005f, 10.0f, 0.5f };
inline constexpr Range modAmount { -1.0f, 1.0f };
inline constexpr Range wtPos { 0.0f, 1.0f };
inline constexpr Range wtWarp { 0.0f, 1.0f };
inline constexpr Range wtDetune { 0.0f, 1.0f };
inline constexpr Range wtSpread { 0.0f, 1.0f };
inline constexpr Range wtSub { 0.0f, 1.0f };
inline constexpr Range ampDecay { 0.005f, 10.0f, 0.5f };
inline constexpr Range ampSustain { 0.0f, 1.0f };
inline constexpr Range volume { -36.0f, 6.0f };
inline constexpr Range pan { -1.0f, 1.0f };
inline constexpr Range fine { -100.0f, 100.0f };
inline constexpr Range velocity { 0.0f, 1.0f };
inline constexpr Range macro { 0.0f, 1.0f };
inline constexpr Range cellDensity { 0.0f, 1.0f };
inline constexpr Range cellSpeed { 0.25f, 60.0f, 6.0f };
inline constexpr Range cellGlide { 0.0f, 1.0f };
inline constexpr Range cellTilt { 0.0f, 2.0f };
inline constexpr Range cellMutate { 0.0f, 1.0f };
inline constexpr Range swarmPull { 0.0f, 1.0f };
inline constexpr Range swarmChaos { 0.0f, 1.0f };
inline constexpr Range swarmFlock { -1.0f, 1.0f };
inline constexpr Range swarmSpread { 0.0f, 1.0f };
inline constexpr Range swarmSpeed { 0.05f, 10.0f, 1.0f };
inline constexpr Range swarmWave { 0.0f, 1.0f };
inline constexpr Range space { 0.0f, 1.0f };
inline constexpr Range room { 0.0f, 1.0f };
inline constexpr Range output { -36.0f, 6.0f };
} // namespace ranges

// One layer's settings. Choice settings hold the index of the selected option.
struct LayerParams
{
    // Excite (Physical)
    int exciter = kStrike;
    float attack = 0.04f;   // fade-in for Bow / Breath / Osc / Wavetable, and the Wavetable amp attack
    float hardness = 0.5f;
    float position = 0.2f;
    float oscWave = 0.66f;  // Sine → Triangle → Saw → Square
    int unison = 0;         // index: 0 = 1 voice ... 6 = 7 voices
    float detune = 0.25f;

    // Object (Physical)
    float shape = 0.5f;     // String → Tube → Bar → Plate → Bell
    float material = 0.6f;  // Rubber → Wood → Metal → Glass
    float size = 0.5f;
    float decay = 1.0f;
    float release = 1.0f;   // also the Wavetable amp release
    int octave = 2;         // index: 0 = -2 ... 4 = +2

    // Impossible (Physical)
    float bloom = 0.0f;
    float drift = 0.0f;
    float transmute = 0.0f;
    float fracture = 0.0f;
    float evolve = 1.5f;

    // Filter & drive
    int filterMode = kFilterLP;
    float cutoff = 20000.0f;
    float resonance = 0.0f;
    float filterEnv = 0.0f;
    float drive = 0.0f;
    float body = 1.0f;      // 1 = all through the object, 0 = raw exciter only

    // Modulation
    std::array<int, kNumLfos> lfoShape { kLfoSine, kLfoTriangle };
    std::array<float, kNumLfos> lfoRate { 1.0f, 0.25f };
    std::array<int, kNumLfos> lfoSync { 0, 0 };
    std::array<int, kNumLfos> lfoRetrig { 0, 0 };
    float envAttack = 0.01f;
    float envDecay = 0.5f;
    float envSustain = 0.0f;
    float envRelease = 0.5f;
    std::array<int, kNumModSlots> modSource {};
    std::array<int, kNumModSlots> modTarget {};
    std::array<float, kNumModSlots> modAmount {};

    // Layer
    int enabled = 0;
    int type = kLayerPhysical;
    float volume = 0.0f;    // dB
    float pan = 0.0f;
    float fine = 0.0f;      // cents
    int keyLow = 0;
    int keyHigh = 127;
    float velLow = 0.0f;
    float velHigh = 1.0f;

    // Wavetable
    int wtTable = 0;
    float wtPos = 0.0f;
    int wtWarpMode = kWarpOff;
    float wtWarp = 0.0f;
    int wtUnison = 0;
    float wtDetune = 0.2f;
    float wtSpread = 0.7f;
    float wtSub = 0.0f;
    float ampDecay = 1.0f;
    float ampSustain = 1.0f;

    // Cells (cellular automaton)
    int cellRule = kRuleLife;
    int cellSeed = kSeedRandom;
    float cellDensity = 0.35f;
    float cellSpeed = 6.0f;   // generations per second (when Sync is Free)
    int cellSync = 0;
    float cellGlide = 0.6f;   // crossfade between generations (0 = hard steps)
    float cellTilt = 0.6f;    // higher = darker (upper harmonics quieter)
    float cellMutate = 0.05f; // random cells flipped each generation

    // Swarm (particles)
    int swarmCount = 2;       // index: 4, 8, 16, 32
    int swarmHome = kHomeHarmonics;
    float swarmPull = 0.6f;
    float swarmChaos = 0.3f;
    float swarmFlock = 0.0f;
    float swarmSpread = 0.5f;  // 0.5 = exact home notes; lower squeezes them, higher stretches
    float swarmSpeed = 1.0f;
    float swarmWave = 0.0f;   // Sine → Triangle → Saw

    // Convenience: set one modulation slot.
    void mod (int slot, int source, int target, float amount)
    {
        modSource[(size_t) slot] = source;
        modTarget[(size_t) slot] = target;
        modAmount[(size_t) slot] = amount;
    }
};

// A whole sound: 4 layers plus global settings.
struct Patch
{
    std::array<LayerParams, kNumLayers> layers;
    std::array<float, kNumMacros> macros {};
    float space = 0.25f;
    float room = 0.6f;
    float output = 0.0f;

    Patch()
    {
        // Layer 1 starts on; the others start off, one of each engine type,
        // so switching one on gives you a different engine.
        for (int i = 0; i < kNumLayers; ++i)
            layers[(size_t) i].type = i;
        layers[0].enabled = 1;
    }
};

// Calls f(id, field) for every setting of one layer (IDs without the layer prefix).
template <typename L, typename F>
void visitLayer (L& p, F&& f)
{
    static const char* const lfoShapeIds[kNumLfos] = { "lfo1Shape", "lfo2Shape" };
    static const char* const lfoRateIds[kNumLfos] = { "lfo1Rate", "lfo2Rate" };
    static const char* const lfoSyncIds[kNumLfos] = { "lfo1Sync", "lfo2Sync" };
    static const char* const lfoRetrigIds[kNumLfos] = { "lfo1Retrig", "lfo2Retrig" };
    static const char* const modSrcIds[kNumModSlots] = { "mod1Src", "mod2Src", "mod3Src", "mod4Src", "mod5Src", "mod6Src" };
    static const char* const modDstIds[kNumModSlots] = { "mod1Dst", "mod2Dst", "mod3Dst", "mod4Dst", "mod5Dst", "mod6Dst" };
    static const char* const modAmtIds[kNumModSlots] = { "mod1Amt", "mod2Amt", "mod3Amt", "mod4Amt", "mod5Amt", "mod6Amt" };

    f ("exciter", p.exciter);
    f ("attack", p.attack);
    f ("hardness", p.hardness);
    f ("position", p.position);
    f ("oscWave", p.oscWave);
    f ("unison", p.unison);
    f ("detune", p.detune);

    f ("shape", p.shape);
    f ("material", p.material);
    f ("size", p.size);
    f ("decay", p.decay);
    f ("release", p.release);
    f ("octave", p.octave);

    f ("bloom", p.bloom);
    f ("drift", p.drift);
    f ("transmute", p.transmute);
    f ("fracture", p.fracture);
    f ("evolve", p.evolve);

    f ("filterMode", p.filterMode);
    f ("cutoff", p.cutoff);
    f ("resonance", p.resonance);
    f ("filterEnv", p.filterEnv);
    f ("drive", p.drive);
    f ("body", p.body);

    for (int i = 0; i < kNumLfos; ++i)
    {
        f (lfoShapeIds[i], p.lfoShape[(size_t) i]);
        f (lfoRateIds[i], p.lfoRate[(size_t) i]);
        f (lfoSyncIds[i], p.lfoSync[(size_t) i]);
        f (lfoRetrigIds[i], p.lfoRetrig[(size_t) i]);
    }

    f ("envAttack", p.envAttack);
    f ("envDecay", p.envDecay);
    f ("envSustain", p.envSustain);
    f ("envRelease", p.envRelease);

    for (int i = 0; i < kNumModSlots; ++i)
    {
        f (modSrcIds[i], p.modSource[(size_t) i]);
        f (modDstIds[i], p.modTarget[(size_t) i]);
        f (modAmtIds[i], p.modAmount[(size_t) i]);
    }

    f ("on", p.enabled);
    f ("type", p.type);
    f ("volume", p.volume);
    f ("pan", p.pan);
    f ("fine", p.fine);
    f ("keyLow", p.keyLow);
    f ("keyHigh", p.keyHigh);
    f ("velLow", p.velLow);
    f ("velHigh", p.velHigh);

    f ("wtTable", p.wtTable);
    f ("wtPos", p.wtPos);
    f ("wtWarpMode", p.wtWarpMode);
    f ("wtWarp", p.wtWarp);
    f ("wtUnison", p.wtUnison);
    f ("wtDetune", p.wtDetune);
    f ("wtSpread", p.wtSpread);
    f ("wtSub", p.wtSub);
    f ("ampDecay", p.ampDecay);
    f ("ampSustain", p.ampSustain);

    f ("cellRule", p.cellRule);
    f ("cellSeed", p.cellSeed);
    f ("cellDensity", p.cellDensity);
    f ("cellSpeed", p.cellSpeed);
    f ("cellSync", p.cellSync);
    f ("cellGlide", p.cellGlide);
    f ("cellTilt", p.cellTilt);
    f ("cellMutate", p.cellMutate);

    f ("swarmCount", p.swarmCount);
    f ("swarmHome", p.swarmHome);
    f ("swarmPull", p.swarmPull);
    f ("swarmChaos", p.swarmChaos);
    f ("swarmFlock", p.swarmFlock);
    f ("swarmSpread", p.swarmSpread);
    f ("swarmSpeed", p.swarmSpeed);
    f ("swarmWave", p.swarmWave);
}

// Calls f(layer, id, field) for every setting in the patch. layer is 0..3 for
// layer settings and -1 for global ones.
template <typename P, typename F>
void visitPatch (P& p, F&& f)
{
    for (int i = 0; i < kNumLayers; ++i)
        visitLayer (p.layers[(size_t) i], [&] (const char* id, auto& field) { f (i, id, field); });

    static const char* const macroIds[kNumMacros] = { "macro1", "macro2", "macro3", "macro4" };
    for (int i = 0; i < kNumMacros; ++i)
        f (-1, macroIds[i], p.macros[(size_t) i]);

    f (-1, "space", p.space);
    f (-1, "room", p.room);
    f (-1, "output", p.output);
}

} // namespace impossible
