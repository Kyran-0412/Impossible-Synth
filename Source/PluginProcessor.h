#pragma once

#include "dsp/Engine.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>

namespace ParamID
{
inline constexpr const char* exciter = "exciter";
inline constexpr const char* hardness = "hardness";
inline constexpr const char* position = "position";
inline constexpr const char* shape = "shape";
inline constexpr const char* material = "material";
inline constexpr const char* size = "size";
inline constexpr const char* decay = "decay";
inline constexpr const char* release = "release";
inline constexpr const char* octave = "octave";
inline constexpr const char* bloom = "bloom";
inline constexpr const char* drift = "drift";
inline constexpr const char* transmute = "transmute";
inline constexpr const char* fracture = "fracture";
inline constexpr const char* evolve = "evolve";
inline constexpr const char* space = "space";
inline constexpr const char* room = "room";
inline constexpr const char* output = "output";
} // namespace ParamID

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

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState; // on-screen keyboard

private:
    void handleMidi (const juce::MidiMessage& m);
    impossible::Params readParams() const;

    impossible::Engine engine;
    juce::Reverb reverb;
    juce::SmoothedValue<float> outputGain;

    struct Raw
    {
        std::atomic<float>* exciter = nullptr;
        std::atomic<float>* hardness = nullptr;
        std::atomic<float>* position = nullptr;
        std::atomic<float>* shape = nullptr;
        std::atomic<float>* material = nullptr;
        std::atomic<float>* size = nullptr;
        std::atomic<float>* decay = nullptr;
        std::atomic<float>* release = nullptr;
        std::atomic<float>* octave = nullptr;
        std::atomic<float>* bloom = nullptr;
        std::atomic<float>* drift = nullptr;
        std::atomic<float>* transmute = nullptr;
        std::atomic<float>* fracture = nullptr;
        std::atomic<float>* evolve = nullptr;
        std::atomic<float>* space = nullptr;
        std::atomic<float>* room = nullptr;
        std::atomic<float>* output = nullptr;
    } raw;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImpossibleSynthProcessor)
};
