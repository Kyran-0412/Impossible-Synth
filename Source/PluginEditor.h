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
    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown, int buttonX, int buttonY,
                       int buttonW, int buttonH, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool isMouseOver, bool isButtonDown) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    juce::Font getPopupMenuFont() override;
};

class ImpossibleSynthEditor : public juce::AudioProcessorEditor,
                              private juce::Timer
{
public:
    explicit ImpossibleSynthEditor (ImpossibleSynthProcessor&);
    ~ImpossibleSynthEditor() override;

    void resized() override;

    static constexpr int kBaseWidth = 1200;
    static constexpr int kBaseHeight = 850;

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

    struct ModRow
    {
        juce::ComboBox source, target;
        juce::Slider amount;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sourceAttachment, targetAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> amountAttachment;
    };

    struct Section
    {
        juce::String title;
        juce::Colour colour;
        juce::Rectangle<int> bounds;
    };

    // Everything is laid out at a fixed base size inside this component,
    // which is then scaled to the window size.
    class Content : public juce::Component
    {
    public:
        explicit Content (ImpossibleSynthEditor& e) : editor (e) {}
        void paint (juce::Graphics& g) override { editor.paintContent (g); }

    private:
        ImpossibleSynthEditor& editor;
    };

    Knob& addKnob (const char* paramId, const juce::String& name, juce::Colour colour,
                   const juce::String& tooltip, bool bipolar = false);
    Choice& addChoice (const char* paramId, const juce::String& name, const juce::String& tooltip = {});
    void setupModRow (ModRow& row, int index);
    void layoutContent();
    void paintContent (juce::Graphics& g);
    void timerCallback() override;

    void refreshPresetList();
    void presetChosen();
    void stepPreset (int delta);
    void savePresetDialog();

    ImpossibleSynthProcessor& processor;
    ImpossibleLookAndFeel lnf;
    Content content { *this };

    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<Choice>> choices;
    std::array<ModRow, impossible::kNumModSlots> modRows;
    std::vector<Section> sections;
    std::vector<std::pair<juce::Rectangle<int>, juce::String>> subTitles;

    juce::TextButton prevButton { "<" }, nextButton { ">" }, saveButton { "Save" }, randomButton { "Randomize" };
    juce::ComboBox presetBox;
    juce::Array<juce::File> userPresetFiles;
    juce::String shownPresetName;
    std::unique_ptr<juce::AlertWindow> saveDialog;

    juce::MidiKeyboardComponent keyboard;
    juce::TooltipWindow tooltips { this, 600 };

    // Quick access for layout and enabling.
    Choice *exciter = nullptr, *unison = nullptr, *octave = nullptr, *filterMode = nullptr;
    Knob *attack = nullptr, *hardness = nullptr, *position = nullptr, *oscWave = nullptr, *detune = nullptr;
    Knob *shape = nullptr, *material = nullptr, *size = nullptr, *decay = nullptr, *release = nullptr;
    Knob *bloom = nullptr, *drift = nullptr, *transmute = nullptr, *fracture = nullptr, *evolve = nullptr;
    Knob *cutoff = nullptr, *resonance = nullptr, *filterEnv = nullptr, *drive = nullptr, *body = nullptr;
    std::array<Choice*, impossible::kNumLfos> lfoShape {}, lfoSync {}, lfoRetrig {};
    std::array<Knob*, impossible::kNumLfos> lfoRate {};
    Knob *envA = nullptr, *envD = nullptr, *envS = nullptr, *envR = nullptr;
    Knob *space = nullptr, *room = nullptr, *output = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImpossibleSynthEditor)
};
