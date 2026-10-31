#pragma once

#include "dsp/Engine.h"
#include "dsp/Presets.h"

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

    // Presets (call from the message thread).
    void applyParams (const impossible::Params& p, const juce::String& presetName);
    void randomize();
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::File& file);
    juce::Array<juce::File> getUserPresets() const;
    static juce::File userPresetFolder();
    juce::String getPresetName() const { return presetName; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState; // on-screen keyboard

private:
    void handleMidi (const juce::MidiMessage& m);
    impossible::Params readParams() const;

    impossible::Engine engine;
    juce::Reverb reverb;
    juce::SmoothedValue<float> outputGain;

    // Parameters in visitParams() order, for fast reading/writing.
    std::vector<std::atomic<float>*> rawValues;
    std::vector<juce::RangedAudioParameter*> parameters;

    std::vector<impossible::Preset> factory;
    int currentProgram = 0;
    juce::String presetName { "Init" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImpossibleSynthProcessor)
};
