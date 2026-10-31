#include "PluginEditor.h"

using namespace impossible;

namespace Colours
{
const juce::Colour background { 0xff0f1117 };
const juce::Colour panel { 0xff171a23 };
const juce::Colour panelEdge { 0xff272c3a };
const juce::Colour knobBody { 0xff222633 };
const juce::Colour track { 0xff2c3142 };
const juce::Colour text { 0xffe6e8ef };
const juce::Colour dim { 0xff8a90a2 };

const juce::Colour excite { 0xffffb347 };
const juce::Colour object { 0xff4fd1c5 };
const juce::Colour impossible { 0xffc77dff };
const juce::Colour filter { 0xffff6b8b };
const juce::Colour mod { 0xff9be15d };
const juce::Colour space { 0xff5b9cff };
} // namespace Colours

namespace
{
juce::Font uiFont (float height, bool bold = false)
{
    auto options = juce::FontOptions (height);
    return juce::Font (bold ? options.withStyle ("Bold") : options);
}

constexpr int kFactoryIdOffset = 1;  // combo IDs 1..N are factory presets
constexpr int kUserIdOffset = 1001;  // combo IDs 1001.. are user presets
} // namespace

//==============================================================================
ImpossibleLookAndFeel::ImpossibleLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, Colours::background);
    setColour (juce::Label::textColourId, Colours::text);
    setColour (juce::Slider::textBoxTextColourId, Colours::dim);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, Colours::impossible.withAlpha (0.4f));
    setColour (juce::ComboBox::textColourId, Colours::text);
    setColour (juce::ComboBox::backgroundColourId, Colours::knobBody);
    setColour (juce::ComboBox::outlineColourId, Colours::panelEdge);
    setColour (juce::ComboBox::arrowColourId, Colours::dim);
    setColour (juce::PopupMenu::backgroundColourId, Colours::panel);
    setColour (juce::PopupMenu::textColourId, Colours::text);
    setColour (juce::PopupMenu::headerTextColourId, Colours::dim);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Colours::track);
    setColour (juce::PopupMenu::highlightedTextColourId, Colours::text);
    setColour (juce::TextButton::buttonColourId, Colours::knobBody);
    setColour (juce::TextButton::textColourOffId, Colours::text);
    setColour (juce::TextButton::textColourOnId, Colours::text);
    setColour (juce::TextEditor::backgroundColourId, Colours::knobBody);
    setColour (juce::TextEditor::textColourId, Colours::text);
    setColour (juce::TextEditor::outlineColourId, Colours::panelEdge);
    setColour (juce::TextEditor::highlightColourId, Colours::impossible.withAlpha (0.4f));
    setColour (juce::CaretComponent::caretColourId, Colours::text);
    setColour (juce::AlertWindow::backgroundColourId, Colours::panel);
    setColour (juce::AlertWindow::textColourId, Colours::text);
    setColour (juce::AlertWindow::outlineColourId, Colours::panelEdge);
    setColour (juce::TooltipWindow::backgroundColourId, Colours::panel);
    setColour (juce::TooltipWindow::textColourId, Colours::text);
    setColour (juce::TooltipWindow::outlineColourId, Colours::panelEdge);
}

void ImpossibleLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                              float sliderPos, float startAngle, float endAngle,
                                              juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (4.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const float cx = bounds.getCentreX();
    const float cy = bounds.getCentreY();
    const float lineW = juce::jmax (3.0f, radius * 0.11f);
    const float arcR = radius - lineW * 0.5f;
    const bool enabled = slider.isEnabled();
    const auto colour = enabled ? slider.findColour (juce::Slider::rotarySliderFillColourId) : Colours::dim.darker (0.6f);
    const bool bipolar = slider.getProperties().getWithDefault ("bipolar", false);
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    const float from = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;

    juce::Path trackPath;
    trackPath.addCentredArc (cx, cy, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (Colours::track);
    g.strokePath (trackPath, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (std::abs (angle - from) > 0.001f)
    {
        juce::Path valuePath;
        valuePath.addCentredArc (cx, cy, arcR, arcR, 0.0f, juce::jmin (from, angle), juce::jmax (from, angle), true);
        g.setColour (colour);
        g.strokePath (valuePath, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    const float bodyR = radius * 0.64f;
    g.setColour (Colours::knobBody.brighter (slider.isMouseOverOrDragging() && enabled ? 0.15f : 0.0f));
    g.fillEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f);
    g.setColour (Colours::panelEdge);
    g.drawEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);

    const juce::Point<float> dir (std::sin (angle), -std::cos (angle));
    g.setColour (enabled ? Colours::text : Colours::dim);
    g.drawLine (cx + dir.x * bodyR * 0.25f, cy + dir.y * bodyR * 0.25f,
                cx + dir.x * bodyR * 0.85f, cy + dir.y * bodyR * 0.85f, juce::jmax (2.0f, lineW * 0.55f));
}

void ImpossibleLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                              float sliderPos, float, float, juce::Slider::SliderStyle,
                                              juce::Slider& slider)
{
    // Bipolar bar (used for modulation amounts): fills from the centre.
    const auto r = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height)
                       .withSizeKeepingCentre ((float) width, 8.0f);
    g.setColour (Colours::track);
    g.fillRoundedRectangle (r, 4.0f);

    const float centre = r.getCentreX();
    const float lo = juce::jmin (centre, sliderPos);
    const float hi = juce::jmax (centre, sliderPos);
    g.setColour (slider.findColour (juce::Slider::trackColourId));
    g.fillRoundedRectangle (lo, r.getY(), juce::jmax (2.0f, hi - lo), r.getHeight(), 4.0f);

    g.setColour (Colours::dim);
    g.fillRect (centre - 0.5f, r.getY() - 2.0f, 1.0f, r.getHeight() + 4.0f);
    g.setColour (Colours::text);
    g.fillEllipse (sliderPos - 5.0f, r.getCentreY() - 5.0f, 10.0f, 10.0f);
}

void ImpossibleLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int,
                                          juce::ComboBox& box)
{
    const auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId).brighter (box.isMouseOver() ? 0.1f : 0.0f));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (r, 6.0f, 1.0f);

    juce::Path arrow;
    const float ax = (float) width - 12.0f;
    const float ay = (float) height * 0.5f;
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (box.findColour (juce::ComboBox::arrowColourId));
    g.fillPath (arrow);
}

juce::Font ImpossibleLookAndFeel::getComboBoxFont (juce::ComboBox& box)
{
    return uiFont (juce::jmin (15.0f, (float) box.getHeight() * 0.55f), true);
}

void ImpossibleLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (4, 1, box.getWidth() - 20, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

juce::Label* ImpossibleLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (slider);
    l->setFont (uiFont (12.5f));
    l->setColour (juce::Label::textColourId, Colours::dim);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    return l;
}

void ImpossibleLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& colour,
                                                  bool isMouseOver, bool isButtonDown)
{
    const auto r = button.getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (colour.brighter (isButtonDown ? 0.25f : isMouseOver ? 0.12f : 0.0f));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (Colours::panelEdge);
    g.drawRoundedRectangle (r, 6.0f, 1.0f);
}

juce::Font ImpossibleLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return uiFont (juce::jmin (15.0f, (float) buttonHeight * 0.5f), true);
}

juce::Font ImpossibleLookAndFeel::getPopupMenuFont()
{
    return uiFont (15.0f);
}

//==============================================================================
ImpossibleSynthEditor::ImpossibleSynthEditor (ImpossibleSynthProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    addAndMakeVisible (content);

    sections = { { "EXCITE", Colours::excite, {} },
                 { "OBJECT", Colours::object, {} },
                 { "IMPOSSIBLE", Colours::impossible, {} },
                 { "FILTER & DRIVE", Colours::filter, {} },
                 { "MODULATION", Colours::mod, {} },
                 { "SPACE", Colours::space, {} } };

    // Excite
    exciter = &addChoice ("exciter", "Exciter", "How the object is played. Bow, Breath and Osc keep sounding while you hold the key.");
    attack = &addKnob ("attack", "Attack", Colours::excite, "Fade-in time for Bow, Breath and Osc.");
    hardness = &addKnob ("hardness", "Hardness", Colours::excite, "How hard the mallet is, or how bright the bow/breath/osc is. Velocity adds to it.");
    position = &addKnob ("position", "Position", Colours::excite, "Where on the object you hit it. Changes which overtones ring.");
    oscWave = &addKnob ("oscWave", "Wave", Colours::excite, "Osc exciter waveform: Sine, Triangle, Saw, Square.");
    unison = &addChoice ("unison", "Unison", "Osc exciter: number of stacked, detuned oscillators.");
    detune = &addKnob ("detune", "Detune", Colours::excite, "Osc exciter: how far apart the unison oscillators are tuned.");

    // Object
    shape = &addKnob ("shape", "Shape", Colours::object, "Morphs the object: String, Tube, Bar, Plate, Bell.");
    material = &addKnob ("material", "Material", Colours::object, "Morphs what it's made of: Rubber, Wood, Metal, Glass.");
    size = &addKnob ("size", "Size", Colours::object, "Tiny to huge. Bigger rings longer, deeper and less perfectly.");
    decay = &addKnob ("decay", "Decay", Colours::object, "Multiplies how long the object rings.");
    release = &addKnob ("release", "Release", Colours::object, "How quickly the sound is damped after you let go of the key.");
    octave = &addChoice ("octave", "Octave", "Transpose by octaves.");

    // Impossible
    bloom = &addKnob ("bloom", "Bloom", Colours::impossible, "The sound swells louder after the hit instead of only fading.");
    drift = &addKnob ("drift", "Drift", Colours::impossible, "Overtones bend down (left) or up (right) as the note rings.", true);
    transmute = &addKnob ("transmute", "Transmute", Colours::impossible, "The material changes during the note: left heads to rubber, right to glass.", true);
    fracture = &addKnob ("fracture", "Fracture", Colours::impossible, "Overtones split apart and beat against each other, like the object cracking.");
    evolve = &addKnob ("evolve", "Evolve", Colours::impossible, "How fast Bloom, Drift, Transmute and Fracture happen.");

    // Filter & drive
    filterMode = &addChoice ("filterMode", "Filter", "Filter type, applied to each note.");
    cutoff = &addKnob ("cutoff", "Cutoff", Colours::filter, "Filter cutoff frequency.");
    resonance = &addKnob ("resonance", "Resonance", Colours::filter, "Filter resonance: emphasis around the cutoff.");
    filterEnv = &addKnob ("filterEnv", "Env Amt", Colours::filter, "How much the Mod Env moves the cutoff (up to 6 octaves either way).", true);
    drive = &addKnob ("drive", "Drive", Colours::filter, "Distortion before the filter.");
    body = &addKnob ("body", "Body", Colours::filter, "How much of the sound goes through the object. Lower it to hear the raw Bow/Breath/Osc.");

    // Modulation
    for (int l = 0; l < kNumLfos; ++l)
    {
        const juce::String n = "lfo" + juce::String (l + 1);
        lfoShape[(size_t) l] = &addChoice ((n + "Shape").toRawUTF8(), "Shape", "LFO waveform. S&H jumps randomly, Drift wanders smoothly.");
        lfoSync[(size_t) l] = &addChoice ((n + "Sync").toRawUTF8(), "Sync", "Free uses the Rate knob. Note values lock the LFO to the song tempo.");
        lfoRetrig[(size_t) l] = &addChoice ((n + "Retrig").toRawUTF8(), "Mode", "Free: all notes share one LFO. Retrig: restarts on every note.");
        lfoRate[(size_t) l] = &addKnob ((n + "Rate").toRawUTF8(), "Rate", Colours::mod, "LFO speed (when Sync is Free).");
    }
    envA = &addKnob ("envAttack", "A", Colours::mod, "Mod Env attack.");
    envD = &addKnob ("envDecay", "D", Colours::mod, "Mod Env decay.");
    envS = &addKnob ("envSustain", "S", Colours::mod, "Mod Env sustain level.");
    envR = &addKnob ("envRelease", "R", Colours::mod, "Mod Env release.");
    for (int i = 0; i < kNumModSlots; ++i)
        setupModRow (modRows[(size_t) i], i);

    // Space
    space = &addKnob ("space", "Space", Colours::space, "Amount of reverb.");
    room = &addKnob ("room", "Room", Colours::space, "Size of the reverb room.");
    output = &addKnob ("output", "Output", Colours::space, "Output volume.");

    // Preset bar
    for (auto* b : { &prevButton, &nextButton, &saveButton, &randomButton })
        content.addAndMakeVisible (*b);
    content.addAndMakeVisible (presetBox);
    presetBox.setJustificationType (juce::Justification::centred);
    presetBox.setTooltip ("Presets. Your saved presets appear under User.");
    prevButton.onClick = [this] { stepPreset (-1); };
    nextButton.onClick = [this] { stepPreset (1); };
    saveButton.onClick = [this] { savePresetDialog(); };
    saveButton.setTooltip ("Save the current sound as a preset (in Documents/Impossible Synth/Presets).");
    randomButton.onClick = [this] { processor.randomize(); };
    randomButton.setTooltip ("Roll a random sound.");
    randomButton.setColour (juce::TextButton::buttonColourId, Colours::impossible.withAlpha (0.35f));
    presetBox.onChange = [this] { presetChosen(); };
    refreshPresetList();

    keyboard.setAvailableRange (36, 96);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, Colours::impossible.withAlpha (0.6f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, Colours::impossible.withAlpha (0.25f));
    keyboard.setWantsKeyboardFocus (false);
    keyboard.setOctaveForMiddleC (5); // same note names as FL Studio
    content.addAndMakeVisible (keyboard);

    layoutContent();

    // Set last so every child picks up the custom look.
    setLookAndFeel (&lnf);

    // Resizable with a fixed aspect ratio; start smaller on small screens.
    setResizable (true, true);
    setResizeLimits (kBaseWidth / 2, kBaseHeight / 2, kBaseWidth * 2, kBaseHeight * 2);
    if (auto* aspect = getConstrainer())
        aspect->setFixedAspectRatio ((double) kBaseWidth / (double) kBaseHeight);

    double scale = 1.0;
    if (auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
        scale = juce::jlimit (0.5, 1.0, (display->userArea.getHeight() - 80) / (double) kBaseHeight);
    setSize (juce::roundToInt (kBaseWidth * scale), juce::roundToInt (kBaseHeight * scale));

    timerCallback();
    startTimerHz (10);
}

ImpossibleSynthEditor::~ImpossibleSynthEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

ImpossibleSynthEditor::Knob& ImpossibleSynthEditor::addKnob (const char* paramId, const juce::String& name,
                                                             juce::Colour colour, const juce::String& tooltip,
                                                             bool bipolar)
{
    auto k = std::make_unique<Knob>();
    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 100, 18);
    k->slider.setColour (juce::Slider::rotarySliderFillColourId, colour);
    k->slider.getProperties().set ("bipolar", bipolar);
    k->slider.setTooltip (tooltip);
    content.addAndMakeVisible (k->slider);

    k->label.setText (name, juce::dontSendNotification);
    k->label.setJustificationType (juce::Justification::centred);
    k->label.setFont (uiFont (14.0f, true));
    content.addAndMakeVisible (k->label);

    k->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, paramId, k->slider);
    // Double-click resets to default (set after the attachment so it uses the real range).
    if (auto* param = processor.apvts.getParameter (paramId))
        k->slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));

    knobs.push_back (std::move (k));
    return *knobs.back();
}

ImpossibleSynthEditor::Choice& ImpossibleSynthEditor::addChoice (const char* paramId, const juce::String& name,
                                                                 const juce::String& tooltip)
{
    auto c = std::make_unique<Choice>();
    if (auto* param = dynamic_cast<juce::AudioParameterChoice*> (processor.apvts.getParameter (paramId)))
        c->box.addItemList (param->choices, 1);
    c->box.setJustificationType (juce::Justification::centred);
    c->box.setTooltip (tooltip);
    content.addAndMakeVisible (c->box);

    c->label.setText (name, juce::dontSendNotification);
    c->label.setJustificationType (juce::Justification::centred);
    c->label.setFont (uiFont (14.0f, true));
    content.addAndMakeVisible (c->label);

    c->attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, paramId, c->box);
    choices.push_back (std::move (c));
    return *choices.back();
}

void ImpossibleSynthEditor::setupModRow (ModRow& row, int index)
{
    const juce::String n = "mod" + juce::String (index + 1);
    auto fill = [this] (juce::ComboBox& box, const juce::String& id)
    {
        if (auto* param = dynamic_cast<juce::AudioParameterChoice*> (processor.apvts.getParameter (id)))
            box.addItemList (param->choices, 1);
        box.setJustificationType (juce::Justification::centredLeft);
        content.addAndMakeVisible (box);
    };
    fill (row.source, n + "Src");
    fill (row.target, n + "Dst");
    row.source.setTooltip ("Modulation source.");
    row.target.setTooltip ("What the source moves.");

    row.amount.setSliderStyle (juce::Slider::LinearHorizontal);
    row.amount.setTextBoxStyle (juce::Slider::TextBoxRight, false, 46, 20);
    row.amount.setColour (juce::Slider::trackColourId, Colours::mod);
    row.amount.setTooltip ("Modulation amount. Double-click to reset.");
    row.amount.setDoubleClickReturnValue (true, 0.0);
    content.addAndMakeVisible (row.amount);

    row.sourceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, n + "Src", row.source);
    row.targetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, n + "Dst", row.target);
    row.amountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, n + "Amt", row.amount);
}

//==============================================================================
void ImpossibleSynthEditor::resized()
{
    const float scale = (float) getWidth() / (float) kBaseWidth;
    content.setBounds (0, 0, kBaseWidth, kBaseHeight);
    content.setTransform (juce::AffineTransform::scale (scale));
}

void ImpossibleSynthEditor::layoutContent()
{
    auto area = juce::Rectangle<int> (0, 0, kBaseWidth, kBaseHeight).reduced (16, 0);

    // Header: preset bar on the right.
    {
        auto header = area.removeFromTop (60).withSizeKeepingCentre (area.getWidth(), 32);
        randomButton.setBounds (header.removeFromRight (110));
        header.removeFromRight (8);
        saveButton.setBounds (header.removeFromRight (64));
        header.removeFromRight (8);
        nextButton.setBounds (header.removeFromRight (32));
        header.removeFromRight (4);
        presetBox.setBounds (header.removeFromRight (250));
        header.removeFromRight (4);
        prevButton.setBounds (header.removeFromRight (32));
    }

    auto kb = area.removeFromBottom (90);
    kb.removeFromBottom (12);
    keyboard.setBounds (kb);
    keyboard.setKeyWidth ((float) kb.getWidth() / 36.0f); // 36 white keys in C2..C7

    const int gap = 12;
    area.removeFromBottom (gap);
    auto rowA = area.removeFromTop (196);
    area.removeFromTop (gap);
    auto rowB = area.removeFromTop (196);
    area.removeFromTop (gap);
    auto rowC = area;

    sections[0].bounds = rowA.removeFromLeft (560);
    rowA.removeFromLeft (gap);
    sections[1].bounds = rowA;
    sections[2].bounds = rowB.removeFromLeft (560);
    rowB.removeFromLeft (gap);
    sections[3].bounds = rowB;
    sections[5].bounds = rowC.removeFromRight (200);
    rowC.removeFromRight (gap);
    sections[4].bounds = rowC;

    auto placeKnob = [] (Knob* k, juce::Rectangle<int> cell)
    {
        k->label.setBounds (cell.removeFromTop (20));
        k->slider.setBounds (cell.reduced (2, 0));
    };
    auto placeChoice = [] (Choice* c, juce::Rectangle<int> cell)
    {
        c->label.setBounds (cell.removeFromTop (20));
        c->box.setBounds (cell.withSizeKeepingCentre (juce::jmin (cell.getWidth() - 8, 110), 30).translated (0, -10));
    };
    auto inner = [] (const Section& s) { return s.bounds.reduced (10).withTrimmedTop (30); };
    auto row = [] (juce::Rectangle<int> r, int count)
    {
        std::vector<juce::Rectangle<int>> cells;
        const int w = r.getWidth() / count;
        for (int i = 0; i < count; ++i)
            cells.push_back (i == count - 1 ? r : r.removeFromLeft (w));
        return cells;
    };

    {
        auto c = row (inner (sections[0]), 7);
        placeChoice (exciter, c[0]);
        placeKnob (attack, c[1]);
        placeKnob (hardness, c[2]);
        placeKnob (position, c[3]);
        placeKnob (oscWave, c[4]);
        placeChoice (unison, c[5]);
        placeKnob (detune, c[6]);
    }
    {
        auto c = row (inner (sections[1]), 6);
        placeKnob (shape, c[0]);
        placeKnob (material, c[1]);
        placeKnob (size, c[2]);
        placeKnob (decay, c[3]);
        placeKnob (release, c[4]);
        placeChoice (octave, c[5]);
    }
    {
        auto c = row (inner (sections[2]), 5);
        placeKnob (bloom, c[0]);
        placeKnob (drift, c[1]);
        placeKnob (transmute, c[2]);
        placeKnob (fracture, c[3]);
        placeKnob (evolve, c[4]);
    }
    {
        auto c = row (inner (sections[3]), 6);
        placeChoice (filterMode, c[0]);
        placeKnob (cutoff, c[1]);
        placeKnob (resonance, c[2]);
        placeKnob (filterEnv, c[3]);
        placeKnob (drive, c[4]);
        placeKnob (body, c[5]);
    }

    // Modulation: LFO 1 | LFO 2 | Mod Env | Matrix
    subTitles.clear();
    {
        auto r = inner (sections[4]);
        for (int l = 0; l < kNumLfos; ++l)
        {
            auto block = r.removeFromLeft (168);
            r.removeFromLeft (14);
            subTitles.push_back ({ block.removeFromTop (20), "LFO " + juce::String (l + 1) });
            block.removeFromTop (4);
            auto combos = block.removeFromLeft (90);
            for (auto* c : { lfoShape[(size_t) l], lfoSync[(size_t) l], lfoRetrig[(size_t) l] })
            {
                c->label.setBounds (combos.removeFromTop (18));
                c->box.setBounds (combos.removeFromTop (28));
                combos.removeFromTop (8);
            }
            placeKnob (lfoRate[(size_t) l], block.withTrimmedTop (30).withTrimmedBottom (30));
        }

        auto envBlock = r.removeFromLeft (200);
        r.removeFromLeft (14);
        subTitles.push_back ({ envBlock.removeFromTop (20), "MOD ENV" });
        envBlock.removeFromTop (30);
        envBlock.removeFromBottom (30);
        auto c = row (envBlock, 4);
        placeKnob (envA, c[0]);
        placeKnob (envD, c[1]);
        placeKnob (envS, c[2]);
        placeKnob (envR, c[3]);

        subTitles.push_back ({ r.removeFromTop (20), "MATRIX" });
        r.removeFromTop (4);
        for (auto& m : modRows)
        {
            auto line = r.removeFromTop (28);
            r.removeFromTop (5);
            m.source.setBounds (line.removeFromLeft (92));
            line.removeFromLeft (4);
            m.target.setBounds (line.removeFromLeft (98));
            line.removeFromLeft (6);
            m.amount.setBounds (line);
        }
    }

    {
        auto r = inner (sections[5]);
        auto top = r.removeFromTop (r.getHeight() / 2);
        placeKnob (space, top.removeFromLeft (top.getWidth() / 2));
        placeKnob (room, top);
        placeKnob (output, r.withSizeKeepingCentre (r.getWidth() / 2, r.getHeight()));
    }
}

void ImpossibleSynthEditor::paintContent (juce::Graphics& g)
{
    g.fillAll (Colours::background);

    // Header.
    auto header = juce::Rectangle<int> (0, 0, kBaseWidth, 60).reduced (24, 0);
    g.setColour (Colours::text);
    g.setFont (uiFont (26.0f, true));
    g.drawText ("IMPOSSIBLE SYNTH", header.removeFromLeft (300), juce::Justification::centredLeft);
    g.setColour (Colours::dim);
    g.setFont (uiFont (13.0f));
    g.drawText ("v0.2", header.removeFromLeft (40), juce::Justification::centredLeft);

    for (auto& s : sections)
    {
        const auto r = s.bounds.toFloat();
        g.setColour (Colours::panel);
        g.fillRoundedRectangle (r, 10.0f);
        g.setColour (Colours::panelEdge);
        g.drawRoundedRectangle (r.reduced (0.5f), 10.0f, 1.0f);

        g.setColour (s.colour);
        g.fillRoundedRectangle (r.getX() + 16.0f, r.getY() + 14.0f, 4.0f, 14.0f, 2.0f);
        g.setFont (uiFont (14.0f, true));
        g.drawText (s.title, (int) r.getX() + 26, (int) r.getY() + 10, 200, 22, juce::Justification::centredLeft);
    }

    g.setFont (uiFont (12.0f, true));
    for (auto& [r, text] : subTitles)
    {
        g.setColour (Colours::dim);
        g.drawText (text, r, juce::Justification::centredLeft);
        g.setColour (Colours::panelEdge);
        g.drawHorizontalLine (r.getBottom(), (float) r.getX(), (float) r.getRight());
    }
}

void ImpossibleSynthEditor::timerCallback()
{
    // Grey out controls that currently do nothing.
    auto value = [this] (const char* id) { return juce::roundToInt (processor.apvts.getRawParameterValue (id)->load()); };
    const int ex = value ("exciter");
    const bool continuous = ex == kBow || ex == kBreath || ex == kOsc;
    const bool osc = ex == kOsc;
    const bool filterOn = value ("filterMode") != kFilterOff;

    attack->slider.setEnabled (continuous);
    oscWave->slider.setEnabled (osc);
    unison->box.setEnabled (osc);
    detune->slider.setEnabled (osc);
    for (auto* k : { cutoff, resonance, filterEnv })
        k->slider.setEnabled (filterOn);
    lfoRate[0]->slider.setEnabled (value ("lfo1Sync") == 0);
    lfoRate[1]->slider.setEnabled (value ("lfo2Sync") == 0);

    if (processor.getPresetName() != shownPresetName)
        refreshPresetList();
}

//==============================================================================
void ImpossibleSynthEditor::refreshPresetList()
{
    presetBox.clear (juce::dontSendNotification);
    presetBox.addSectionHeading ("Factory");
    for (int i = 0; i < processor.getNumPrograms(); ++i)
        presetBox.addItem (processor.getProgramName (i), kFactoryIdOffset + i);

    userPresetFiles = processor.getUserPresets();
    if (! userPresetFiles.isEmpty())
    {
        presetBox.addSectionHeading ("User");
        for (int i = 0; i < userPresetFiles.size(); ++i)
            presetBox.addItem (userPresetFiles[i].getFileNameWithoutExtension(), kUserIdOffset + i);
    }

    shownPresetName = processor.getPresetName();

    // Select the matching entry, or just show the name (e.g. "Random").
    int match = 0;
    for (int i = 0; i < presetBox.getNumItems(); ++i)
        if (presetBox.getItemText (i) == shownPresetName)
            match = presetBox.getItemId (i);

    if (match != 0)
        presetBox.setSelectedId (match, juce::dontSendNotification);
    else
        presetBox.setText (shownPresetName, juce::dontSendNotification);
}

void ImpossibleSynthEditor::presetChosen()
{
    const int id = presetBox.getSelectedId();
    if (id >= kUserIdOffset)
    {
        const int index = id - kUserIdOffset;
        if (juce::isPositiveAndBelow (index, userPresetFiles.size()))
            processor.loadUserPreset (userPresetFiles[index]);
    }
    else if (id >= kFactoryIdOffset)
    {
        processor.setCurrentProgram (id - kFactoryIdOffset);
    }
    refreshPresetList();
}

void ImpossibleSynthEditor::stepPreset (int delta)
{
    const int count = presetBox.getNumItems();
    if (count == 0)
        return;

    int index = 0;
    for (int i = 0; i < count; ++i)
        if (presetBox.getItemId (i) == presetBox.getSelectedId())
            index = i;

    index = (index + delta + count) % count;
    presetBox.setSelectedId (presetBox.getItemId (index), juce::sendNotificationSync);
}

void ImpossibleSynthEditor::savePresetDialog()
{
    saveDialog = std::make_unique<juce::AlertWindow> ("Save preset", "Name your preset:",
                                                      juce::MessageBoxIconType::NoIcon, this);
    saveDialog->setLookAndFeel (&lnf);
    saveDialog->addTextEditor ("name", processor.getPresetName() == "Random" ? juce::String() : processor.getPresetName());
    saveDialog->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    saveDialog->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    juce::Component::SafePointer<ImpossibleSynthEditor> safeThis (this);
    saveDialog->enterModalState (true, juce::ModalCallbackFunction::create ([safeThis] (int result)
    {
        if (safeThis == nullptr || safeThis->saveDialog == nullptr)
            return;

        const auto name = safeThis->saveDialog->getTextEditorContents ("name").trim();
        if (result == 1 && name.isNotEmpty())
        {
            if (safeThis->processor.saveUserPreset (name))
                safeThis->refreshPresetList();
            else
                juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Save preset",
                                                        "Couldn't save the preset to:\n"
                                                            + ImpossibleSynthProcessor::userPresetFolder().getFullPathName());
        }

        // Delete the dialog after its callback has finished.
        juce::MessageManager::callAsync ([safeThis]
        {
            if (safeThis != nullptr)
                safeThis->saveDialog.reset();
        });
    }), false);
}
