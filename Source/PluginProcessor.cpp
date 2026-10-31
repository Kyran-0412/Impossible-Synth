#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <map>
#include <type_traits>

using namespace impossible;

namespace
{
// "String", "String→Tube 40%", ... for morphing knobs.
juce::String morphText (float value, const char* const* names, int count)
{
    const float pos = juce::jlimit (0.0f, 1.0f, value) * (float) (count - 1);
    const int i = juce::jmin ((int) pos, count - 2);
    const float frac = pos - (float) i;

    if (frac < 0.04f)
        return names[i];
    if (frac > 0.96f)
        return names[i + 1];
    return juce::String (names[i]) + juce::String::fromUTF8 ("\xe2\x86\x92") + names[i + 1] + " "
           + juce::String (juce::roundToInt (frac * 100.0f)) + "%";
}

juce::StringArray toStringArray (const char* const* names, int count)
{
    juce::StringArray a;
    for (int i = 0; i < count; ++i)
        a.add (names[i]);
    return a;
}

// Note names numbered like FL Studio (middle C = C5).
juce::StringArray noteNames()
{
    static const char* const names[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    juce::StringArray a;
    for (int n = 0; n < 128; ++n)
        a.add (juce::String (names[n % 12]) + juce::String (n / 12));
    return a;
}

juce::NormalisableRange<float> toJuce (const Range& r)
{
    juce::NormalisableRange<float> nr (r.min, r.max);
    if (r.skewed())
        nr.setSkewForCentre (r.centre);
    return nr;
}

struct Spec
{
    juce::String name;
    Range range { 0.0f, 1.0f };
    std::function<juce::String (float, int)> text;
    juce::StringArray choices;
};

// Display name, range and value text for every parameter ID (layer IDs without prefix).
const std::map<juce::String, Spec>& specs()
{
    static const std::map<juce::String, Spec> table = []
    {
        using juce::String;
        auto percent = [] (float v, int) { return String (juce::roundToInt (v * 100.0f)) + "%"; };
        auto bipolar = [] (float v, int)
        {
            const int p = juce::roundToInt (v * 100.0f);
            return (p > 0 ? "+" : "") + String (p) + "%";
        };
        auto seconds = [] (float v, int)
        {
            return v < 0.995f ? String (juce::roundToInt (v * 1000.0f)) + " ms" : String (v, 2) + " s";
        };
        auto hertz = [] (float v, int)
        {
            return v < 1000.0f ? String (v, v < 10.0f ? 2 : 0) + " Hz" : String (v / 1000.0f, 1) + " kHz";
        };
        auto decibels = [] (float v, int) { return String (v, 1) + " dB"; };

        std::map<String, Spec> s;
        s["exciter"] = { "Exciter", {}, {}, toStringArray (kExciterNames, kNumExciters) };
        s["attack"] = { "Attack", ranges::attack, seconds, {} };
        s["hardness"] = { "Hardness", ranges::hardness, percent, {} };
        s["position"] = { "Position", ranges::position, percent, {} };
        s["oscWave"] = { "Osc Wave", ranges::oscWave, [] (float v, int)
                         {
                             static const char* const waves[] = { "Sine", "Triangle", "Saw", "Square" };
                             return morphText (v, waves, 4);
                         }, {} };
        s["unison"] = { "Unison", {}, {}, toStringArray (kUnisonNames, kMaxUnison) };
        s["detune"] = { "Detune", ranges::detune, percent, {} };

        s["shape"] = { "Shape", ranges::shape, [] (float v, int) { return morphText (v, kShapeNames, kNumShapes); }, {} };
        s["material"] = { "Material", ranges::material, [] (float v, int) { return morphText (v, kMaterialNames, kNumMaterials); }, {} };
        s["size"] = { "Size", ranges::size, [] (float v, int)
                      {
                          if (v < 0.15f) return String ("Tiny");
                          if (v < 0.4f) return String ("Small");
                          if (v < 0.6f) return String ("Normal");
                          if (v < 0.85f) return String ("Big");
                          return String ("Huge");
                      }, {} };
        s["decay"] = { "Decay", ranges::decay, [] (float v, int) { return String (v, 2) + "x"; }, {} };
        s["release"] = { "Release", ranges::release, seconds, {} };
        s["octave"] = { "Octave", {}, {}, toStringArray (kOctaveNames, 5) };

        s["bloom"] = { "Bloom", ranges::bloom, percent, {} };
        s["drift"] = { "Drift", ranges::drift, bipolar, {} };
        s["transmute"] = { "Transmute", ranges::transmute, bipolar, {} };
        s["fracture"] = { "Fracture", ranges::fracture, percent, {} };
        s["evolve"] = { "Evolve", ranges::evolve, seconds, {} };

        s["filterMode"] = { "Filter Mode", {}, {}, toStringArray (kFilterModeNames, kNumFilterModes) };
        s["cutoff"] = { "Cutoff", ranges::cutoff, hertz, {} };
        s["resonance"] = { "Resonance", ranges::resonance, percent, {} };
        s["filterEnv"] = { "Filter Env", ranges::filterEnv, bipolar, {} };
        s["drive"] = { "Drive", ranges::drive, percent, {} };
        s["body"] = { "Body", ranges::body, percent, {} };

        for (int i = 1; i <= kNumLfos; ++i)
        {
            const String n = "lfo" + String (i);
            const String label = "LFO " + String (i) + " ";
            s[n + "Shape"] = { label + "Shape", {}, {}, toStringArray (kLfoShapeNames, kNumLfoShapes) };
            s[n + "Rate"] = { label + "Rate", ranges::lfoRate, hertz, {} };
            s[n + "Sync"] = { label + "Sync", {}, {}, toStringArray (kSyncNames, kNumSyncOptions) };
            s[n + "Retrig"] = { label + "Mode", {}, {}, toStringArray (kRetrigNames, 2) };
        }

        s["envAttack"] = { "Env Attack", ranges::envAttack, seconds, {} };
        s["envDecay"] = { "Env Decay", ranges::envDecay, seconds, {} };
        s["envSustain"] = { "Env Sustain", ranges::envSustain, percent, {} };
        s["envRelease"] = { "Env Release", ranges::envRelease, seconds, {} };

        for (int i = 1; i <= kNumModSlots; ++i)
        {
            const String n = "mod" + String (i);
            const String label = "Mod " + String (i) + " ";
            s[n + "Src"] = { label + "Source", {}, {}, toStringArray (kSourceNames, kNumSources) };
            s[n + "Dst"] = { label + "Target", {}, {}, toStringArray (kTargetNames, kNumTargets) };
            s[n + "Amt"] = { label + "Amount", ranges::modAmount, bipolar, {} };
        }

        s["on"] = { "On", {}, {}, toStringArray (kOnOffNames, 2) };
        s["type"] = { "Engine", {}, {}, toStringArray (kLayerTypeNames, kNumLayerTypes) };
        s["volume"] = { "Volume", ranges::volume, decibels, {} };
        s["pan"] = { "Pan", ranges::pan, [] (float v, int)
                     {
                         const int p = juce::roundToInt (v * 100.0f);
                         return p == 0 ? String ("C") : (p < 0 ? "L" + String (-p) : "R" + String (p));
                     }, {} };
        s["fine"] = { "Fine Tune", ranges::fine, [] (float v, int) { return String (juce::roundToInt (v)) + " ct"; }, {} };
        s["keyLow"] = { "Key Low", {}, {}, noteNames() };
        s["keyHigh"] = { "Key High", {}, {}, noteNames() };
        s["velLow"] = { "Velocity Low", ranges::velocity, [] (float v, int) { return String (juce::roundToInt (v * 127.0f)); }, {} };
        s["velHigh"] = { "Velocity High", ranges::velocity, [] (float v, int) { return String (juce::roundToInt (v * 127.0f)); }, {} };

        s["wtTable"] = { "Wavetable", {}, {}, ImpossibleSynthProcessor::wavetableChoices() };
        s["wtPos"] = { "WT Position", ranges::wtPos, percent, {} };
        s["wtWarpMode"] = { "Warp Mode", {}, {}, toStringArray (kWarpNames, kNumWarpModes) };
        s["wtWarp"] = { "Warp", ranges::wtWarp, percent, {} };
        s["wtUnison"] = { "WT Unison", {}, {}, toStringArray (kUnisonNames, kMaxUnison) };
        s["wtDetune"] = { "WT Detune", ranges::wtDetune, percent, {} };
        s["wtSpread"] = { "WT Spread", ranges::wtSpread, percent, {} };
        s["wtSub"] = { "Sub", ranges::wtSub, percent, {} };
        s["ampDecay"] = { "Amp Decay", ranges::ampDecay, seconds, {} };
        s["ampSustain"] = { "Amp Sustain", ranges::ampSustain, percent, {} };

        for (int i = 1; i <= kNumMacros; ++i)
            s["macro" + String (i)] = { "Macro " + String (i), ranges::macro, percent, {} };

        s["space"] = { "Space", ranges::space, percent, {} };
        s["room"] = { "Room", ranges::room, percent, {} };
        s["output"] = { "Output", ranges::output, decibels, {} };
        return s;
    }();
    return table;
}

juce::String fullId (int layer, const char* id)
{
    return layer < 0 ? juce::String (id) : ImpossibleSynthProcessor::layerId (layer, id);
}
} // namespace

//==============================================================================
juce::String ImpossibleSynthProcessor::layerId (int layer, const juce::String& id)
{
    return "L" + juce::String (layer + 1) + "_" + id;
}

juce::StringArray ImpossibleSynthProcessor::wavetableChoices()
{
    juce::StringArray names;
    for (auto& t : builtInWavetables())
        names.add (t->name());
    names.add ("Imported");
    return names;
}

ImpossibleSynthProcessor::ImpossibleSynthProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "ImpossibleSynth", createLayout()),
      factory (factoryPresets())
{
    formats.registerBasicFormats();

    Patch defaults;
    visitPatch (defaults, [this] (int layer, const char* id, auto&)
    {
        auto* param = apvts.getParameter (fullId (layer, id));
        jassert (param != nullptr);
        parameters.push_back (param);
        rawValues.push_back (apvts.getRawParameterValue (fullId (layer, id)));
    });
}

juce::AudioProcessorValueTreeState::ParameterLayout ImpossibleSynthProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const Patch defaults;

    visitPatch (defaults, [&] (int layer, const char* id, const auto& value)
    {
        const auto it = specs().find (id);
        jassert (it != specs().end()); // every parameter needs a spec
        const Spec& spec = it->second;

        const auto pid = juce::ParameterID { fullId (layer, id), 1 };
        const auto name = layer < 0 ? spec.name : "L" + juce::String (layer + 1) + " " + spec.name;

        if constexpr (std::is_same_v<std::decay_t<decltype (value)>, int>)
        {
            // Choice parameters snap to whole options, so they always save and restore exactly
            // (on/off switches are Off/On choices for the same reason).
            layout.add (std::make_unique<juce::AudioParameterChoice> (pid, name, spec.choices, value));
        }
        else
        {
            layout.add (std::make_unique<juce::AudioParameterFloat> (
                pid, name, toJuce (spec.range), value,
                juce::AudioParameterFloatAttributes().withStringFromValueFunction (spec.text)));
        }
    });

    return layout;
}

Patch ImpossibleSynthProcessor::readPatch() const
{
    Patch p;
    size_t i = 0;
    visitPatch (p, [&] (int, const char*, auto& field)
    {
        const float v = rawValues[i++]->load();
        if constexpr (std::is_same_v<std::decay_t<decltype (field)>, int>)
            field = juce::roundToInt (v);
        else
            field = v;
    });
    return p;
}

void ImpossibleSynthProcessor::applyPatch (const Patch& source, const juce::String& name)
{
    Patch p = source;
    size_t i = 0;
    visitPatch (p, [&] (int, const char*, auto& field)
    {
        auto* param = parameters[i++];
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 ((float) field));
        param->endChangeGesture();
    });
    presetName = name;
}

//==============================================================================
void ImpossibleSynthProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) factory.size()))
        return;
    currentProgram = index;
    applyPatch (factory[(size_t) index].patch, factory[(size_t) index].name);
}

const juce::String ImpossibleSynthProcessor::getProgramName (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) factory.size()))
        return {};
    return factory[(size_t) index].name;
}

void ImpossibleSynthProcessor::randomize()
{
    Rng rng;
    rng.seed ((uint32_t) juce::Random::getSystemRandom().nextInt());
    applyPatch (randomPatch (rng), "Random");
}

juce::File ImpossibleSynthProcessor::userPresetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
        .getChildFile ("Impossible Synth")
        .getChildFile ("Presets");
}

juce::Array<juce::File> ImpossibleSynthProcessor::getUserPresets() const
{
    auto files = userPresetFolder().findChildFiles (juce::File::findFiles, false, "*.impreset");
    files.sort();
    return files;
}

bool ImpossibleSynthProcessor::saveUserPreset (const juce::String& name)
{
    const auto clean = juce::File::createLegalFileName (name.trim());
    if (clean.isEmpty())
        return false;

    auto folder = userPresetFolder();
    if (! folder.createDirectory())
        return false;

    juce::MemoryBlock block;
    getStateInformation (block);
    auto xml = getXmlFromBinary (block.getData(), (int) block.getSize());
    if (xml == nullptr)
        return false;
    xml->setAttribute ("presetName", name.trim());
    if (! xml->writeTo (folder.getChildFile (clean + ".impreset")))
        return false;

    presetName = name.trim();
    return true;
}

bool ImpossibleSynthProcessor::loadUserPreset (const juce::File& file)
{
    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return false;

    auto tree = juce::ValueTree::fromXml (*xml);
    if (! tree.hasProperty ("presetName"))
        tree.setProperty ("presetName", file.getFileNameWithoutExtension(), nullptr);
    loadStateTree (tree);
    return true;
}

//==============================================================================
bool ImpossibleSynthProcessor::loadImported (int layer, const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
    if (reader == nullptr || reader->lengthInSamples <= 0)
        return false;

    const int maxSamples = Wavetable::kFrameSize * Wavetable::kMaxFrames;
    const int length = (int) juce::jmin<juce::int64> (reader->lengthInSamples, maxSamples);
    juce::AudioBuffer<float> buffer ((int) reader->numChannels, length);
    if (! reader->read (&buffer, 0, length, 0, true, true))
        return false;

    // Mix to mono.
    std::vector<float> mono ((size_t) length, 0.0f);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        for (int n = 0; n < length; ++n)
            mono[(size_t) n] += buffer.getSample (ch, n) / (float) buffer.getNumChannels();

    auto table = Wavetable::fromAudio (mono.data(), length, file.getFileNameWithoutExtension().toStdString());
    if (table == nullptr)
        return false;

    if (importedTables[(size_t) layer] != nullptr)
        retiredTables.push_back (importedTables[(size_t) layer]);
    importedTables[(size_t) layer] = table;
    importedPaths[(size_t) layer] = file.getFullPathName();
    engine.setImportedTable (layer, table.get());
    return true;
}

bool ImpossibleSynthProcessor::importWavetable (int layer, const juce::File& file)
{
    if (! juce::isPositiveAndBelow (layer, kNumLayers) || ! loadImported (layer, file))
        return false;

    // Switch the layer's table choice to "Imported".
    if (auto* param = apvts.getParameter (layerId (layer, "wtTable")))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 ((float) kNumBuiltInWavetables));
        param->endChangeGesture();
    }
    return true;
}

const Wavetable* ImpossibleSynthProcessor::getLayerWavetable (int layer) const
{
    const auto& builtIns = builtInWavetables();
    const int index = juce::roundToInt (apvts.getRawParameterValue (layerId (layer, "wtTable"))->load());
    if (juce::isPositiveAndBelow (index, (int) builtIns.size()))
        return builtIns[(size_t) index].get();
    if (importedTables[(size_t) layer] != nullptr)
        return importedTables[(size_t) layer].get();
    return builtIns.empty() ? nullptr : builtIns[0].get();
}

juce::String ImpossibleSynthProcessor::getImportedName (int layer) const
{
    if (auto& t = importedTables[(size_t) layer])
        return t->name();
    return {};
}

//==============================================================================
void ImpossibleSynthProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
    for (int i = 0; i < kNumLayers; ++i)
        engine.setImportedTable (i, importedTables[(size_t) i].get());
    reverb.setSampleRate (sampleRate);
    reverb.reset();
    outputGain.reset (sampleRate, 0.05);
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (readPatch().output));
    keyboardState.reset();
}

bool ImpossibleSynthProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void ImpossibleSynthProcessor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn())
        engine.noteOn (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())
        engine.noteOff (m.getNoteNumber());
    else if (m.isSustainPedalOn())
        engine.setSustain (true);
    else if (m.isSustainPedalOff())
        engine.setSustain (false);
    else if (m.isPitchWheel())
        engine.setPitchBend ((float) (m.getPitchWheelValue() - 8192) / 8192.0f * 2.0f);
    else if (m.isControllerOfType (1))
        engine.setModWheel ((float) m.getControllerValue() / 127.0f);
    else if (m.isAllSoundOff())
        engine.allSoundOff();
    else if (m.isAllNotesOff())
        engine.allNotesOff();
}

void ImpossibleSynthProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    if (buffer.getNumChannels() < 2 || numSamples == 0)
        return;

    // Merge in notes played on the on-screen keyboard.
    keyboardState.processNextMidiBuffer (midi, 0, numSamples, true);

    const Patch patch = readPatch();
    engine.setPatch (patch);

    // Tempo and song position, for tempo-synced LFOs.
    double bpm = 120.0, ppq = 0.0;
    bool playing = false;
    if (auto* ph = getPlayHead())
    {
        if (auto position = ph->getPosition())
        {
            bpm = position->getBpm().orFallback (120.0);
            ppq = position->getPpqPosition().orFallback (0.0);
            playing = position->getIsPlaying();
        }
    }
    engine.setTransport (bpm, playing, ppq);

    float* left = buffer.getWritePointer (0);
    float* right = buffer.getWritePointer (1);

    // Sample-accurate MIDI: render up to each event, then apply it.
    int pos = 0;
    for (const auto meta : midi)
    {
        const int at = juce::jlimit (0, numSamples, meta.samplePosition);
        if (at > pos)
        {
            engine.render (left + pos, right + pos, at - pos);
            pos = at;
        }
        handleMidi (meta.getMessage());
    }
    if (pos < numSamples)
        engine.render (left + pos, right + pos, numSamples - pos);

    // Space (reverb).
    juce::Reverb::Parameters rp;
    rp.roomSize = 0.3f + 0.68f * patch.room;
    rp.damping = 0.35f;
    rp.wetLevel = patch.space * 0.5f;
    rp.dryLevel = 1.0f - patch.space * 0.4f;
    rp.width = 1.0f;
    rp.freezeMode = 0.0f;
    reverb.setParameters (rp); // cheap: it only sets smoothed targets
    reverb.processStereo (left, right, numSamples);

    // Output level with a gentle safety clip.
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (patch.output));
    for (int i = 0; i < numSamples; ++i)
    {
        const float g = outputGain.getNextValue();
        left[i] = std::tanh (left[i] * g);
        right[i] = std::tanh (right[i] * g);
    }
}

//==============================================================================
juce::AudioProcessorEditor* ImpossibleSynthProcessor::createEditor()
{
    return new ImpossibleSynthEditor (*this);
}

void ImpossibleSynthProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("presetName", presetName, nullptr);
    state.setProperty ("version", 3, nullptr);
    for (int i = 0; i < kNumLayers; ++i)
        state.setProperty ("importedTable" + juce::String (i + 1), importedPaths[(size_t) i], nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void ImpossibleSynthProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            loadStateTree (juce::ValueTree::fromXml (*xml));
}

void ImpossibleSynthProcessor::loadStateTree (juce::ValueTree tree)
{
    // v0.1 / v0.2 had a single layer with unprefixed IDs: move them to layer 1.
    if (! tree.hasProperty ("version"))
    {
        static const juce::StringArray globals { "space", "room", "output" };
        for (auto child : tree)
        {
            const auto id = child.getProperty ("id").toString();
            if (child.hasType ("PARAM") && id.isNotEmpty() && ! globals.contains (id) && ! id.startsWith ("L"))
                child.setProperty ("id", layerId (0, id), nullptr);
        }
    }

    // Anything missing from the saved state goes back to its default, so
    // layers left over from the previous sound don't keep playing.
    for (auto* param : parameters)
    {
        const auto id = param->getParameterID();
        if (! tree.getChildWithProperty ("id", id).isValid())
        {
            juce::ValueTree child ("PARAM");
            child.setProperty ("id", id, nullptr);
            child.setProperty ("value", param->convertFrom0to1 (param->getDefaultValue()), nullptr);
            tree.appendChild (child, nullptr);
        }
    }

    presetName = tree.getProperty ("presetName", "Init").toString();

    for (int i = 0; i < kNumLayers; ++i)
    {
        const auto path = tree.getProperty ("importedTable" + juce::String (i + 1)).toString();
        if (path.isNotEmpty() && path != importedPaths[(size_t) i])
            loadImported (i, juce::File (path));
    }

    apvts.replaceState (tree);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ImpossibleSynthProcessor();
}
