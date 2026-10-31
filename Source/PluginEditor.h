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
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool isMouseOver, bool isButtonDown) override;
    juce::Font getPopupMenuFont() override;
};

// Draws a wavetable as a stack of frames, highlighting the current position.
class WavetableDisplay : public juce::Component
{
public:
    std::function<const impossible::Wavetable*()> getTable;
    std::function<float()> getPosition;
    juce::Colour colour;

    void paint (juce::Graphics& g) override;
    void refresh();

private:
    const impossible::Wavetable* lastTable = nullptr;
    float lastPosition = -1.0f;
};

// Live view of the newest note's Game of Life grid.
class CellsDisplay : public juce::Component,
                     private juce::Timer
{
public:
    CellsDisplay() { startTimerHz (30); }
    ~CellsDisplay() override { stopTimer(); }

    std::function<const impossible::LayerDisplay*()> getDisplay;
    juce::Colour colour;

    void paint (juce::Graphics& g) override;

private:
    void timerCallback() override
    {
        if (isShowing())
            repaint();
    }
};

// Live view of the newest note's particles (pitch across, stereo up/down).
class SwarmDisplay : public juce::Component,
                     private juce::Timer
{
public:
    SwarmDisplay() { startTimerHz (30); }
    ~SwarmDisplay() override { stopTimer(); }

    std::function<const impossible::LayerDisplay*()> getDisplay;
    juce::Colour colour;

    void paint (juce::Graphics& g) override;

private:
    void timerCallback() override
    {
        if (isShowing())
            repaint();
    }
};

// One tab in the layer bar: click to select, click the dot to switch the layer on/off.
class LayerTab : public juce::Component,
                 public juce::SettableTooltipClient
{
public:
    int layer = -1; // -1 = Mix
    bool selected = false;
    bool enabled = false;
    juce::String engineName;
    juce::Colour colour;
    std::function<void()> onSelect, onTogglePower;

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
};

class ImpossibleSynthEditor : public juce::AudioProcessorEditor,
                              private juce::Timer
{
public:
    explicit ImpossibleSynthEditor (ImpossibleSynthProcessor&);
    ~ImpossibleSynthEditor() override;

    void resized() override;

    static constexpr int kBaseWidth = 1240;
    static constexpr int kBaseHeight = 1000;

private:
    // A control bound to a parameter. Layer controls (layer == kCurrentLayer)
    // follow whichever layer is being edited; others are fixed.
    static constexpr int kCurrentLayer = -2;
    static constexpr int kGlobal = -1;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        juce::String baseId;
        int layer = kCurrentLayer;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    struct Choice
    {
        juce::ComboBox box;
        juce::Label label;
        juce::String baseId;
        int layer = kCurrentLayer;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
    };

    struct ModRow
    {
        juce::ComboBox source, target;
        juce::Slider amount;
        int index = 0;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sourceAttachment, targetAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> amountAttachment;
    };

    struct MixColumn
    {
        juce::ToggleButton power { "On" };
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerAttachment;
        Choice* type = nullptr;
        Knob *volume = nullptr, *pan = nullptr, *fine = nullptr, *velLow = nullptr, *velHigh = nullptr;
        Choice *octave = nullptr, *keyLow = nullptr, *keyHigh = nullptr;
        juce::TextButton edit { "Edit" };
    };

    struct Section
    {
        juce::String title;
        juce::Colour colour;
        juce::Rectangle<int> bounds;
    };

    class Content : public juce::Component
    {
    public:
        explicit Content (ImpossibleSynthEditor& e) : editor (e) {}
        void paint (juce::Graphics& g) override { editor.paintContent (g); }

    private:
        ImpossibleSynthEditor& editor;
    };

    Knob& addKnob (const juce::String& baseId, const juce::String& name, juce::Colour colour,
                   const juce::String& tooltip, bool bipolar = false, int layer = kCurrentLayer);
    Choice& addChoice (const juce::String& baseId, const juce::String& name, const juce::String& tooltip = {},
                       int layer = kCurrentLayer);
    juce::String paramIdFor (const juce::String& baseId, int layer) const;
    void bindToLayer (int layer);
    void showView (int view, int sourcePage = -1);
    void layoutContent();
    void layoutPhysicalPage (juce::Rectangle<int> page);
    void layoutWavetablePage (juce::Rectangle<int> page);
    void layoutCellsPage (juce::Rectangle<int> page);
    void layoutSwarmPage (juce::Rectangle<int> page);
    void layoutSourceRowB (juce::Rectangle<int> rowB, juce::Rectangle<int> rowC);
    void layoutMixPage (juce::Rectangle<int> page);
    void layoutSharedSections (juce::Rectangle<int> filterArea, juce::Rectangle<int> modArea);
    void paintContent (juce::Graphics& g);
    void timerCallback() override;
    int currentLayerType() const;
    float rawValue (const juce::String& id) const;

    void refreshPresetList();
    void presetChosen();
    void stepPreset (int delta);
    void savePresetDialog();
    void importWavetable();

    ImpossibleSynthProcessor& processor;
    ImpossibleLookAndFeel lnf;
    Content content { *this };

    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<Choice>> choices;
    std::array<ModRow, impossible::kNumModSlots> modRows;
    std::array<MixColumn, impossible::kNumLayers> mixColumns;
    std::vector<Section> sections;
    std::vector<std::pair<juce::Rectangle<int>, juce::String>> subTitles;
    juce::Rectangle<int> linksArea;

    // View: -1 = Mix page, 0..3 = editing that layer.
    int view = 0;
    int boundLayer = -1;
    int sourceOverride = -1; // showing the Wavetable/Cells/Swarm page of a Physical layer's exciter
    int shownType = -1;

    std::array<LayerTab, impossible::kNumLayers + 1> tabs;
    juce::Label engineLabel;
    Choice* engineType = nullptr;

    juce::TextButton prevButton { "<" }, nextButton { ">" }, saveButton { "Save" }, randomButton { "Randomize" };
    juce::ComboBox presetBox;
    juce::Array<juce::File> userPresetFiles;
    juce::String shownPresetName;
    std::unique_ptr<juce::AlertWindow> saveDialog;
    std::unique_ptr<juce::FileChooser> fileChooser;

    juce::TextButton editSourceButton;
    juce::TextButton backToObjectButton { juce::String::fromUTF8 ("\xe2\x80\xb9 Back to Object") };
    juce::TextButton importButton { "Import WAV" };
    WavetableDisplay wavetableDisplay;
    CellsDisplay cellsDisplay;
    SwarmDisplay swarmDisplay;

    juce::MidiKeyboardComponent keyboard;
    juce::TooltipWindow tooltips { this, 600 };

    // Quick access.
    Choice *exciter = nullptr, *unison = nullptr, *octave = nullptr, *filterMode = nullptr;
    Knob *attack = nullptr, *hardness = nullptr, *position = nullptr, *oscWave = nullptr, *detune = nullptr;
    Knob *shape = nullptr, *material = nullptr, *size = nullptr, *decay = nullptr, *release = nullptr;
    Knob *bloom = nullptr, *drift = nullptr, *transmute = nullptr, *fracture = nullptr, *evolve = nullptr;
    Knob *cutoff = nullptr, *resonance = nullptr, *filterEnv = nullptr, *drive = nullptr, *body = nullptr;
    Choice *wtTable = nullptr, *wtWarpMode = nullptr, *wtUnison = nullptr;
    Knob *wtPos = nullptr, *wtWarp = nullptr, *wtDetune = nullptr, *wtSpread = nullptr, *wtSub = nullptr;
    Knob *ampDecay = nullptr, *ampSustain = nullptr;
    Choice *cellRule = nullptr, *cellSeed = nullptr, *cellSync = nullptr, *swarmCount = nullptr, *swarmHome = nullptr;
    Knob *cellDensity = nullptr, *cellSpeed = nullptr, *cellGlide = nullptr, *cellTilt = nullptr, *cellMutate = nullptr;
    Knob *swarmPull = nullptr, *swarmChaos = nullptr, *swarmFlock = nullptr, *swarmSpread = nullptr, *swarmSpeed = nullptr, *swarmWave = nullptr;
    std::array<Choice*, impossible::kNumLfos> lfoShape {}, lfoSync {}, lfoRetrig {};
    std::array<Knob*, impossible::kNumLfos> lfoRate {};
    Knob *envA = nullptr, *envD = nullptr, *envS = nullptr, *envR = nullptr;
    std::array<Knob*, impossible::kNumMacros> macros {};
    Knob *space = nullptr, *room = nullptr, *output = nullptr;

    std::vector<juce::Component*> physicalComponents, wavetableComponents, cellsComponents, swarmComponents,
        sharedComponents, mixComponents;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImpossibleSynthEditor)
};
