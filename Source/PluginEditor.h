#pragma once

#include "PluginProcessor.h"

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

class ImpossibleLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ImpossibleLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown, int buttonX, int buttonY,
                       int buttonW, int buttonH, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
};

class ImpossibleSynthEditor : public juce::AudioProcessorEditor
{
public:
    explicit ImpossibleSynthEditor (ImpossibleSynthProcessor&);
    ~ImpossibleSynthEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    struct Choice
    {
        juce::ComboBox box;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
    };

    struct Section
    {
        juce::String title;
        juce::Colour colour;
        juce::Rectangle<int> bounds;
    };

    Knob& addKnob (const char* paramId, const juce::String& name, juce::Colour colour, bool bipolar = false);
    Choice& addChoice (const char* paramId, const juce::String& name);
    void layoutRow (juce::Rectangle<int> area, std::initializer_list<juce::Component*> items);

    ImpossibleSynthProcessor& processor;
    ImpossibleLookAndFeel lnf;

    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<Choice>> choices;
    std::array<Section, 4> sections;
    juce::MidiKeyboardComponent keyboard;
    juce::TooltipWindow tooltips { this, 600 };

    // Quick access for layout.
    Choice* exciter = nullptr;
    Choice* octave = nullptr;
    Knob *hardness = nullptr, *position = nullptr;
    Knob *shape = nullptr, *material = nullptr, *size = nullptr, *decay = nullptr, *release = nullptr;
    Knob *bloom = nullptr, *drift = nullptr, *transmute = nullptr, *fracture = nullptr, *evolve = nullptr;
    Knob *space = nullptr, *room = nullptr, *output = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImpossibleSynthEditor)
};
