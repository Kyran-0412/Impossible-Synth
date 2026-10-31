#include "PluginProcessor.h"
#include "PluginEditor.h"

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

juce::NormalisableRange<float> skewedRange (float lo, float hi, float centre)
{
    juce::NormalisableRange<float> r (lo, hi);
    r.setSkewForCentre (centre);
    return r;
}
} // namespace

ImpossibleSynthProcessor::ImpossibleSynthProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "ImpossibleSynth", createLayout())
{
    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id); };
    raw.exciter = get (ParamID::exciter);
    raw.hardness = get (ParamID::hardness);
    raw.position = get (ParamID::position);
    raw.shape = get (ParamID::shape);
    raw.material = get (ParamID::material);
    raw.size = get (ParamID::size);
    raw.decay = get (ParamID::decay);
    raw.release = get (ParamID::release);
    raw.octave = get (ParamID::octave);
    raw.bloom = get (ParamID::bloom);
    raw.drift = get (ParamID::drift);
    raw.transmute = get (ParamID::transmute);
    raw.fracture = get (ParamID::fracture);
    raw.evolve = get (ParamID::evolve);
    raw.space = get (ParamID::space);
    raw.room = get (ParamID::room);
    raw.output = get (ParamID::output);
}

juce::AudioProcessorValueTreeState::ParameterLayout ImpossibleSynthProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto percent = [] (float v, int) { return String (roundToInt (v * 100.0f)) + "%"; };
    auto bipolarPercent = [] (float v, int)
    {
        const int p = roundToInt (v * 100.0f);
        return (p > 0 ? "+" : "") + String (p) + "%";
    };
    auto seconds = [] (float v, int)
    {
        return v < 0.995f ? String (roundToInt (v * 1000.0f)) + " ms" : String (v, 2) + " s";
    };

    auto addFloat = [&] (const char* id, const char* name, NormalisableRange<float> range, float def,
                         std::function<String (float, int)> toText)
    {
        layout.add (std::make_unique<AudioParameterFloat> (
            ParameterID { id, 1 }, name, range, def,
            AudioParameterFloatAttributes().withStringFromValueFunction (std::move (toText))));
    };

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamID::exciter, 1 }, "Exciter",
                                                        StringArray { "Strike", "Pluck", "Bow", "Breath" }, 0));
    addFloat (ParamID::hardness, "Hardness", { 0.0f, 1.0f }, 0.5f, percent);
    addFloat (ParamID::position, "Position", { 0.02f, 0.5f }, 0.2f, [] (float v, int)
              { return String (roundToInt (v * 100.0f)) + "%"; });

    addFloat (ParamID::shape, "Shape", { 0.0f, 1.0f }, 0.5f, [] (float v, int)
              { return morphText (v, impossible::kShapeNames, impossible::kNumShapes); });
    addFloat (ParamID::material, "Material", { 0.0f, 1.0f }, 0.6f, [] (float v, int)
              { return morphText (v, impossible::kMaterialNames, impossible::kNumMaterials); });
    addFloat (ParamID::size, "Size", { 0.0f, 1.0f }, 0.5f, [] (float v, int)
              {
                  if (v < 0.15f) return String ("Tiny");
                  if (v < 0.4f) return String ("Small");
                  if (v < 0.6f) return String ("Normal");
                  if (v < 0.85f) return String ("Big");
                  return String ("Huge");
              });
    addFloat (ParamID::decay, "Decay", skewedRange (0.1f, 10.0f, 1.0f), 1.0f, [] (float v, int)
              { return String (v, 2) + "x"; });
    addFloat (ParamID::release, "Release", skewedRange (0.02f, 10.0f, 1.0f), 1.0f, seconds);
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamID::octave, 1 }, "Octave",
                                                        StringArray { "-2", "-1", "0", "+1", "+2" }, 2));

    addFloat (ParamID::bloom, "Bloom", { 0.0f, 1.0f }, 0.0f, percent);
    addFloat (ParamID::drift, "Drift", { -1.0f, 1.0f }, 0.0f, bipolarPercent);
    addFloat (ParamID::transmute, "Transmute", { -1.0f, 1.0f }, 0.0f, bipolarPercent);
    addFloat (ParamID::fracture, "Fracture", { 0.0f, 1.0f }, 0.0f, percent);
    addFloat (ParamID::evolve, "Evolve", skewedRange (0.05f, 10.0f, 1.5f), 1.5f, seconds);

    addFloat (ParamID::space, "Space", { 0.0f, 1.0f }, 0.25f, percent);
    addFloat (ParamID::room, "Room", { 0.0f, 1.0f }, 0.6f, percent);
    addFloat (ParamID::output, "Output", { -36.0f, 6.0f }, 0.0f, [] (float v, int)
              { return String (v, 1) + " dB"; });

    return layout;
}

void ImpossibleSynthProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
    reverb.setSampleRate (sampleRate);
    reverb.reset();
    outputGain.reset (sampleRate, 0.05);
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (raw.output->load()));
    keyboardState.reset();
}

bool ImpossibleSynthProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

impossible::Params ImpossibleSynthProcessor::readParams() const
{
    impossible::Params p;
    p.exciter = juce::roundToInt (raw.exciter->load());
    p.hardness = raw.hardness->load();
    p.position = raw.position->load();
    p.shape = raw.shape->load();
    p.material = raw.material->load();
    p.size = raw.size->load();
    p.decay = raw.decay->load();
    p.release = raw.release->load();
    p.octave = juce::roundToInt (raw.octave->load()) - 2;
    p.bloom = raw.bloom->load();
    p.drift = raw.drift->load();
    p.transmute = raw.transmute->load();
    p.fracture = raw.fracture->load();
    p.evolve = raw.evolve->load();
    return p;
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

    engine.setParams (readParams());

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
    const float space = raw.space->load();
    juce::Reverb::Parameters rp;
    rp.roomSize = 0.3f + 0.68f * raw.room->load();
    rp.damping = 0.35f;
    rp.wetLevel = space * 0.5f;
    rp.dryLevel = 1.0f - space * 0.4f;
    rp.width = 1.0f;
    rp.freezeMode = 0.0f;
    reverb.setParameters (rp); // cheap: it only sets smoothed targets
    reverb.processStereo (left, right, numSamples);

    // Output level with a gentle safety clip.
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (raw.output->load()));
    for (int i = 0; i < numSamples; ++i)
    {
        const float g = outputGain.getNextValue();
        left[i] = std::tanh (left[i] * g);
        right[i] = std::tanh (right[i] * g);
    }
}

juce::AudioProcessorEditor* ImpossibleSynthProcessor::createEditor()
{
    return new ImpossibleSynthEditor (*this);
}

void ImpossibleSynthProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void ImpossibleSynthProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ImpossibleSynthProcessor();
}
