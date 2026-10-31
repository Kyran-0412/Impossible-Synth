#pragma once

#include "dsp/Engine.h"
#include "dsp/Presets.h"

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>

class ImpossibleSynthProcessor : public juce::AudioProcessor
{
public:
    ImpossibleSynthProcessor();
    ~ImpossibleSynthProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 10.0; }

    // Factory presets show up as programs, so FL Studio's preset menu lists them too.
    int getNumPrograms() override { return (int) factory.size(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Parameter ID for a layer setting, e.g. layerId (0, "shape") == "L1_shape".
    static juce::String layerId (int layer, const juce::String& id);

    // Presets (call from the message thread).
    void applyPatch (const impossible::Patch& p, const juce::String& presetName);
    void randomize();
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::File& file);
    juce::Array<juce::File> getUserPresets() const;
    static juce::File userPresetFolder();
    juce::String getPresetName() const { return presetName; }

    // Wavetables (call from the message thread).
    bool importWavetable (int layer, const juce::File& file);
    const impossible::Wavetable* getLayerWavetable (int layer) const; // what the layer's table choice points at
    juce::String getImportedName (int layer) const;
    static juce::StringArray wavetableChoices();

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState; // on-screen keyboard

private:
    void handleMidi (const juce::MidiMessage& m);
    impossible::Patch readPatch() const;
    void loadStateTree (juce::ValueTree tree);
    bool loadImported (int layer, const juce::File& file);

    impossible::Engine engine;
    juce::Reverb reverb;
    juce::SmoothedValue<float> outputGain;
    juce::AudioFormatManager formats;

    // Parameters in visitPatch() order, for fast reading/writing.
    std::vector<std::atomic<float>*> rawValues;
    std::vector<juce::RangedAudioParameter*> parameters;

    std::vector<impossible::Preset> factory;
    int currentProgram = 0;
    juce::String presetName { "Init" };

    // Imported wavetables. Old tables are kept alive (the audio thread may
    // still be reading one) until the plugin closes.
    std::array<std::shared_ptr<impossible::Wavetable>, impossible::kNumLayers> importedTables;
    std::array<juce::String, impossible::kNumLayers> importedPaths;
    std::vector<std::shared_ptr<impossible::Wavetable>> retiredTables;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImpossibleSynthProcessor)
};
