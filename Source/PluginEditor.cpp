#include "PluginEditor.h"

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
const juce::Colour space { 0xff5b9cff };
} // namespace Colours

namespace
{
juce::Font uiFont (float height, bool bold = false)
{
    auto options = juce::FontOptions (height);
    return juce::Font (bold ? options.withStyle ("Bold") : options);
}

constexpr int kWidth = 960;
constexpr int kHeight = 610;
} // namespace

//==============================================================================
ImpossibleLookAndFeel::ImpossibleLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, Colours::background);
    setColour (juce::Label::textColourId, Colours::text);
    setColour (juce::Slider::textBoxTextColourId, Colours::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, Colours::impossible.withAlpha (0.4f));
    setColour (juce::ComboBox::textColourId, Colours::text);
    setColour (juce::ComboBox::backgroundColourId, Colours::knobBody);
    setColour (juce::ComboBox::outlineColourId, Colours::panelEdge);
    setColour (juce::ComboBox::arrowColourId, Colours::dim);
    setColour (juce::PopupMenu::backgroundColourId, Colours::panel);
    setColour (juce::PopupMenu::textColourId, Colours::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Colours::track);
    setColour (juce::PopupMenu::highlightedTextColourId, Colours::text);
    setColour (juce::TextEditor::backgroundColourId, Colours::knobBody);
    setColour (juce::TextEditor::textColourId, Colours::text);
    setColour (juce::TextEditor::highlightColourId, Colours::impossible.withAlpha (0.4f));
    setColour (juce::CaretComponent::caretColourId, Colours::text);
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
    const auto colour = slider.findColour (juce::Slider::rotarySliderFillColourId);
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
        g.setColour (slider.isEnabled() ? colour : Colours::dim);
        g.strokePath (valuePath, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    const float bodyR = radius * 0.64f;
    g.setColour (Colours::knobBody.brighter (slider.isMouseOverOrDragging() ? 0.15f : 0.0f));
    g.fillEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f);
    g.setColour (Colours::panelEdge);
    g.drawEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);

    const juce::Point<float> dir (std::sin (angle), -std::cos (angle));
    g.setColour (Colours::text);
    g.drawLine (cx + dir.x * bodyR * 0.25f, cy + dir.y * bodyR * 0.25f,
                cx + dir.x * bodyR * 0.85f, cy + dir.y * bodyR * 0.85f, juce::jmax (2.0f, lineW * 0.55f));
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
    const float ax = (float) width - 18.0f;
    const float ay = (float) height * 0.5f;
    arrow.addTriangle (ax - 5.0f, ay - 2.5f, ax + 5.0f, ay - 2.5f, ax, ay + 3.5f);
    g.setColour (box.findColour (juce::ComboBox::arrowColourId));
    g.fillPath (arrow);
}

juce::Font ImpossibleLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return uiFont (15.0f, true);
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

//==============================================================================
ImpossibleSynthEditor::ImpossibleSynthEditor (ImpossibleSynthProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{

    sections = { Section { "EXCITE", Colours::excite, {} },
                 Section { "OBJECT", Colours::object, {} },
                 Section { "IMPOSSIBLE", Colours::impossible, {} },
                 Section { "SPACE", Colours::space, {} } };

    exciter = &addChoice (ParamID::exciter, "Exciter");
    hardness = &addKnob (ParamID::hardness, "Hardness", Colours::excite);
    position = &addKnob (ParamID::position, "Position", Colours::excite);

    shape = &addKnob (ParamID::shape, "Shape", Colours::object);
    material = &addKnob (ParamID::material, "Material", Colours::object);
    size = &addKnob (ParamID::size, "Size", Colours::object);
    decay = &addKnob (ParamID::decay, "Decay", Colours::object);
    release = &addKnob (ParamID::release, "Release", Colours::object);
    octave = &addChoice (ParamID::octave, "Octave");

    bloom = &addKnob (ParamID::bloom, "Bloom", Colours::impossible);
    drift = &addKnob (ParamID::drift, "Drift", Colours::impossible, true);
    transmute = &addKnob (ParamID::transmute, "Transmute", Colours::impossible, true);
    fracture = &addKnob (ParamID::fracture, "Fracture", Colours::impossible);
    evolve = &addKnob (ParamID::evolve, "Evolve", Colours::impossible);

    space = &addKnob (ParamID::space, "Space", Colours::space);
    room = &addKnob (ParamID::room, "Room", Colours::space);
    output = &addKnob (ParamID::output, "Output", Colours::space);

    hardness->slider.setTooltip ("How hard the mallet is, or how bright the bow/breath is. Velocity adds to it.");
    position->slider.setTooltip ("Where on the object you hit it. Changes which overtones ring.");
    shape->slider.setTooltip ("Morphs the object: String, Tube, Bar, Plate, Bell.");
    material->slider.setTooltip ("Morphs what it's made of: Rubber, Wood, Metal, Glass.");
    size->slider.setTooltip ("Tiny to huge. Bigger rings longer, deeper and less perfectly.");
    decay->slider.setTooltip ("Multiplies how long the object rings.");
    release->slider.setTooltip ("How quickly the sound is damped after you let go of the key.");
    bloom->slider.setTooltip ("The sound swells louder after the hit instead of only fading.");
    drift->slider.setTooltip ("Overtones bend down (left) or up (right) as the note rings.");
    transmute->slider.setTooltip ("The material changes during the note, e.g. glass melting into rubber (left).");
    fracture->slider.setTooltip ("Overtones split apart and beat against each other, like the object cracking.");
    evolve->slider.setTooltip ("How fast Bloom, Drift, Transmute and Fracture happen.");
    space->slider.setTooltip ("Amount of reverb.");
    room->slider.setTooltip ("Size of the reverb room.");

    keyboard.setAvailableRange (36, 96);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, Colours::impossible.withAlpha (0.6f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, Colours::impossible.withAlpha (0.25f));
    keyboard.setWantsKeyboardFocus (false);
    keyboard.setOctaveForMiddleC (5); // same note names as FL Studio
    addAndMakeVisible (keyboard);

    // Set last so every child picks up the custom look.
    setLookAndFeel (&lnf);
    setSize (kWidth, kHeight);
}

ImpossibleSynthEditor::~ImpossibleSynthEditor()
{
    setLookAndFeel (nullptr);
}

ImpossibleSynthEditor::Knob& ImpossibleSynthEditor::addKnob (const char* paramId, const juce::String& name,
                                                             juce::Colour colour, bool bipolar)
{
    auto k = std::make_unique<Knob>();
    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 110, 18);
    k->slider.setColour (juce::Slider::rotarySliderFillColourId, colour);
    k->slider.getProperties().set ("bipolar", bipolar);
    k->slider.setPopupDisplayEnabled (false, false, nullptr);
    addAndMakeVisible (k->slider);

    k->label.setText (name, juce::dontSendNotification);
    k->label.setJustificationType (juce::Justification::centred);
    k->label.setFont (uiFont (14.0f, true));
    addAndMakeVisible (k->label);

    k->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, paramId, k->slider);
    // Double-click resets to default (set after the attachment so it uses the real range).
    if (auto* param = processor.apvts.getParameter (paramId))
        k->slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));

    knobs.push_back (std::move (k));
    return *knobs.back();
}

ImpossibleSynthEditor::Choice& ImpossibleSynthEditor::addChoice (const char* paramId, const juce::String& name)
{
    auto c = std::make_unique<Choice>();
    if (auto* param = dynamic_cast<juce::AudioParameterChoice*> (processor.apvts.getParameter (paramId)))
        c->box.addItemList (param->choices, 1);
    c->box.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (c->box);

    c->label.setText (name, juce::dontSendNotification);
    c->label.setJustificationType (juce::Justification::centred);
    c->label.setFont (uiFont (14.0f, true));
    addAndMakeVisible (c->label);

    c->attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, paramId, c->box);
    choices.push_back (std::move (c));
    return *choices.back();
}

void ImpossibleSynthEditor::paint (juce::Graphics& g)
{
    g.fillAll (Colours::background);

    // Header.
    auto header = getLocalBounds().removeFromTop (64).reduced (24, 0);
    g.setColour (Colours::text);
    g.setFont (uiFont (26.0f, true));
    g.drawText ("IMPOSSIBLE SYNTH", header.removeFromLeft (320), juce::Justification::centredLeft);
    g.setColour (Colours::dim);
    g.setFont (uiFont (14.0f));
    g.drawText ("physical modelling for objects that can't exist", header, juce::Justification::centredRight);

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
}

void ImpossibleSynthEditor::resized()
{
    auto area = getLocalBounds().reduced (16, 0);
    area.removeFromTop (64);

    auto kb = area.removeFromBottom (84);
    kb.removeFromBottom (12);
    keyboard.setBounds (kb);
    keyboard.setKeyWidth ((float) kb.getWidth() / 36.0f); // 36 white keys in C2..C7

    area.removeFromBottom (12);
    const int gap = 12;
    auto row1 = area.removeFromTop ((area.getHeight() - gap) / 2);
    area.removeFromTop (gap);
    auto row2 = area;

    sections[0].bounds = row1.removeFromLeft (300);
    row1.removeFromLeft (gap);
    sections[1].bounds = row1;
    sections[2].bounds = row2.removeFromLeft (560);
    row2.removeFromLeft (gap);
    sections[3].bounds = row2;

    auto placeKnob = [] (Knob* k, juce::Rectangle<int> cell)
    {
        k->label.setBounds (cell.removeFromTop (20));
        k->slider.setBounds (cell.reduced (4, 0));
    };
    auto placeChoice = [] (Choice* c, juce::Rectangle<int> cell)
    {
        c->label.setBounds (cell.removeFromTop (20));
        c->box.setBounds (cell.withSizeKeepingCentre (juce::jmin (cell.getWidth() - 8, 110), 30)
                              .translated (0, -10));
    };

    auto inner = [] (const Section& s) { return s.bounds.reduced (10).withTrimmedTop (30); };

    {
        auto r = inner (sections[0]);
        const int w = r.getWidth() / 3;
        placeChoice (exciter, r.removeFromLeft (w));
        placeKnob (hardness, r.removeFromLeft (w));
        placeKnob (position, r);
    }
    {
        auto r = inner (sections[1]);
        const int w = r.getWidth() / 6;
        placeKnob (shape, r.removeFromLeft (w));
        placeKnob (material, r.removeFromLeft (w));
        placeKnob (size, r.removeFromLeft (w));
        placeKnob (decay, r.removeFromLeft (w));
        placeKnob (release, r.removeFromLeft (w));
        placeChoice (octave, r);
    }
    {
        auto r = inner (sections[2]);
        const int w = r.getWidth() / 5;
        placeKnob (bloom, r.removeFromLeft (w));
        placeKnob (drift, r.removeFromLeft (w));
        placeKnob (transmute, r.removeFromLeft (w));
        placeKnob (fracture, r.removeFromLeft (w));
        placeKnob (evolve, r);
    }
    {
        auto r = inner (sections[3]);
        const int w = r.getWidth() / 3;
        placeKnob (space, r.removeFromLeft (w));
        placeKnob (room, r.removeFromLeft (w));
        placeKnob (output, r);
    }
}
