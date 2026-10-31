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
const juce::Colour wavetable { 0xff6be3ff };
const juce::Colour macro { 0xfff7d154 };

const juce::Colour layers[kNumLayers] = { juce::Colour (0xff4fd1c5), juce::Colour (0xffffb347),
                                          juce::Colour (0xffc77dff), juce::Colour (0xff5b9cff) };
} // namespace Colours

namespace
{
juce::Font uiFont (float height, bool bold = false)
{
    auto options = juce::FontOptions (height);
    return juce::Font (bold ? options.withStyle ("Bold") : options);
}

constexpr int kFactoryIdOffset = 1;  // preset combo IDs 1..N are factory presets
constexpr int kUserIdOffset = 1001;  // preset combo IDs 1001.. are user presets
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
    setColour (juce::ToggleButton::textColourId, Colours::text);
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

void ImpossibleLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool isMouseOver, bool)
{
    // A pill that lights up when on.
    const auto r = button.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = button.getToggleState();
    const auto colour = button.findColour (juce::ToggleButton::tickColourId);
    g.setColour (on ? colour.withAlpha (isMouseOver ? 0.9f : 0.75f) : Colours::knobBody.brighter (isMouseOver ? 0.1f : 0.0f));
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (Colours::panelEdge);
    g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);
    g.setColour (on ? Colours::background : Colours::dim);
    g.setFont (uiFont (14.0f, true));
    g.drawText (on ? "ON" : "OFF", r, juce::Justification::centred);
}

juce::Font ImpossibleLookAndFeel::getPopupMenuFont()
{
    return uiFont (15.0f);
}

//==============================================================================
void WavetableDisplay::refresh()
{
    const auto* table = getTable ? getTable() : nullptr;
    const float position = getPosition ? getPosition() : 0.0f;
    if (table != lastTable || std::abs (position - lastPosition) > 0.001f)
    {
        lastTable = table;
        lastPosition = position;
        repaint();
    }
}

void WavetableDisplay::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour (Colours::background);
    g.fillRoundedRectangle (bounds, 8.0f);
    g.setColour (Colours::panelEdge);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 8.0f, 1.0f);

    const auto* table = getTable ? getTable() : nullptr;
    if (table == nullptr)
        return;

    const auto area = bounds.reduced (12.0f, 14.0f).withTrimmedTop (12.0f);
    const float dx = area.getWidth() * 0.22f;
    const float dy = area.getHeight() * 0.38f;
    const float waveW = area.getWidth() - dx;
    const float waveH = area.getHeight() - dy;
    const int level = 3;
    const int points = 96;
    const int frames = table->numFrames();
    const int shown = juce::jmin (frames, 20);
    const float position = juce::jlimit (0.0f, 1.0f, getPosition ? getPosition() : 0.0f);

    auto framePath = [&] (float framePos, float depth)
    {
        juce::Path p;
        const float x0 = area.getX() + dx * depth;
        const float yMid = area.getBottom() - waveH * 0.5f - dy * depth;
        for (int n = 0; n <= points; ++n)
        {
            const float s = table->read (framePos, level, (float) (n % points) / (float) points);
            const float px = x0 + waveW * (float) n / (float) points;
            const float py = yMid - s * waveH * 0.42f;
            if (n == 0)
                p.startNewSubPath (px, py);
            else
                p.lineTo (px, py);
        }
        return p;
    };

    // Back to front.
    for (int i = shown - 1; i >= 0; --i)
    {
        const float depth = shown > 1 ? (float) i / (float) (shown - 1) : 0.0f;
        const float framePos = depth * (float) (frames - 1);
        g.setColour (colour.withAlpha (0.10f + 0.12f * (1.0f - depth)));
        g.strokePath (framePath (framePos, depth), juce::PathStrokeType (1.0f));
    }

    g.setColour (colour);
    g.strokePath (framePath (position * (float) (frames - 1), position), juce::PathStrokeType (2.2f));

    g.setColour (Colours::text);
    g.setFont (uiFont (13.0f, true));
    g.drawText (table->name(), bounds.reduced (12.0f, 6.0f).removeFromTop (18.0f), juce::Justification::centredLeft);
    g.setColour (Colours::dim);
    g.setFont (uiFont (12.0f));
    g.drawText (juce::String (frames) + (frames == 1 ? " frame" : " frames"),
                bounds.reduced (12.0f, 6.0f).removeFromTop (18.0f), juce::Justification::centredRight);
}

//==============================================================================
void CellsDisplay::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour (Colours::background);
    g.fillRoundedRectangle (bounds, 8.0f);
    g.setColour (Colours::panelEdge);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 8.0f, 1.0f);

    const auto* d = getDisplay ? getDisplay() : nullptr;
    auto area = bounds.reduced (10.0f).withTrimmedTop (16.0f).withTrimmedBottom (14.0f);
    const float cw = area.getWidth() / (float) impossible::kCellCols;
    const float ch = area.getHeight() / (float) impossible::kCellRows;

    bool any = false;
    for (int y = 0; y < impossible::kCellRows; ++y)
    {
        const uint32_t row = d != nullptr ? d->cells[(size_t) y].load (std::memory_order_relaxed) : 0u;
        for (int x = 0; x < impossible::kCellCols; ++x)
        {
            const auto cell = juce::Rectangle<float> (area.getX() + x * cw, area.getY() + y * ch, cw, ch).reduced (1.0f);
            if ((row >> x) & 1u)
            {
                any = true;
                g.setColour (colour.withAlpha (0.55f + 0.45f * (1.0f - (float) x / impossible::kCellCols)));
                g.fillRoundedRectangle (cell, 1.5f);
            }
            else
            {
                g.setColour (Colours::panel);
                g.fillRoundedRectangle (cell, 1.5f);
            }
        }
    }

    g.setFont (uiFont (12.0f, true));
    g.setColour (Colours::text);
    g.drawText ("LIFE", bounds.reduced (12.0f, 2.0f).removeFromTop (18.0f), juce::Justification::centredLeft);
    g.setColour (Colours::dim);
    g.setFont (uiFont (11.0f));
    g.drawText (juce::String::fromUTF8 ("low  \xe2\x86\x90  harmonics  \xe2\x86\x92  high"),
                bounds.reduced (12.0f, 2.0f).removeFromBottom (14.0f), juce::Justification::centred);
    if (! any)
    {
        g.setColour (Colours::text.withAlpha (0.7f));
        g.setFont (uiFont (13.0f));
        g.drawText ("Play a note to start life", area, juce::Justification::centred);
    }
}

void SwarmDisplay::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour (Colours::background);
    g.fillRoundedRectangle (bounds, 8.0f);
    g.setColour (Colours::panelEdge);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 8.0f, 1.0f);

    auto area = bounds.reduced (12.0f).withTrimmedTop (14.0f).withTrimmedBottom (12.0f);
    // Show pitch from 1 octave below the note to 5 above.
    constexpr float lo = -1.0f, hi = 5.0f;
    auto xFor = [&] (float octaves) { return area.getX() + (octaves - lo) / (hi - lo) * area.getWidth(); };

    g.setFont (uiFont (10.5f));
    for (int o = 0; o <= 4; ++o)
    {
        const float x = xFor ((float) o);
        g.setColour (Colours::panelEdge);
        g.drawVerticalLine ((int) x, area.getY(), area.getBottom());
        g.setColour (Colours::dim);
        g.drawText (o == 0 ? juce::String ("note") : "+" + juce::String (o), (int) x - 20, (int) area.getBottom(), 40, 12,
                    juce::Justification::centred);
    }

    const auto* d = getDisplay ? getDisplay() : nullptr;
    const int count = d != nullptr ? d->particleCount.load (std::memory_order_relaxed) : 0;
    for (int i = 0; i < count; ++i)
    {
        const uint32_t packed = d->particles[(size_t) i].load (std::memory_order_relaxed);
        const float px = (float) (packed >> 16) / 65535.0f * 8.0f - 3.0f;
        const float py = (float) (packed & 0xffffu) / 65535.0f * 2.0f - 1.0f;
        if (px < lo || px > hi)
            continue;
        const float x = xFor (px);
        const float y = area.getCentreY() - py * area.getHeight() * 0.45f;
        g.setColour (colour.withAlpha (0.18f));
        g.fillEllipse (x - 7.0f, y - 7.0f, 14.0f, 14.0f);
        g.setColour (colour);
        g.fillEllipse (x - 3.0f, y - 3.0f, 6.0f, 6.0f);
    }

    g.setFont (uiFont (12.0f, true));
    g.setColour (Colours::text);
    g.drawText ("SWARM", bounds.reduced (12.0f, 2.0f).removeFromTop (16.0f), juce::Justification::centredLeft);
    g.setColour (Colours::dim);
    g.setFont (uiFont (11.0f));
    g.drawText (juce::String::fromUTF8 ("pitch \xe2\x86\x92   \xc2\xb7   up/down = stereo"), bounds.reduced (12.0f, 2.0f).removeFromTop (16.0f),
                juce::Justification::centredRight);
    if (count == 0)
    {
        g.setColour (Colours::text.withAlpha (0.7f));
        g.setFont (uiFont (13.0f));
        g.drawText ("Play a note to release the swarm", area, juce::Justification::centred);
    }
}

//==============================================================================
void LayerTab::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (selected ? Colours::panel.brighter (0.08f) : Colours::panel);
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (selected ? colour : Colours::panelEdge);
    g.drawRoundedRectangle (r, 8.0f, selected ? 2.0f : 1.0f);

    if (layer < 0)
    {
        g.setColour (selected ? Colours::text : Colours::dim);
        g.setFont (uiFont (15.0f, true));
        g.drawText ("MIX", r, juce::Justification::centred);
        return;
    }

    // Power dot.
    const auto dot = juce::Rectangle<float> (r.getX() + 12.0f, r.getCentreY() - 7.0f, 14.0f, 14.0f);
    g.setColour (enabled ? colour : Colours::knobBody);
    g.fillEllipse (dot);
    g.setColour (enabled ? colour.brighter (0.3f) : Colours::dim);
    g.drawEllipse (dot, 1.5f);

    auto textArea = r.withTrimmedLeft (34.0f);
    g.setColour (enabled ? Colours::text : Colours::dim);
    g.setFont (uiFont (15.0f, true));
    g.drawText ("LAYER " + juce::String (layer + 1), textArea.removeFromTop (r.getHeight() * 0.55f),
                juce::Justification::bottomLeft);
    g.setColour (enabled ? colour : Colours::dim);
    g.setFont (uiFont (12.0f));
    g.drawText (engineName, textArea, juce::Justification::topLeft);
}

void LayerTab::mouseDown (const juce::MouseEvent& e)
{
    if (layer >= 0 && e.x < 34)
    {
        if (onTogglePower)
            onTogglePower();
        return;
    }
    if (onSelect)
        onSelect();
}

//==============================================================================
ImpossibleSynthEditor::ImpossibleSynthEditor (ImpossibleSynthProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    addAndMakeVisible (content);

    auto addKnobTo = [] (std::vector<juce::Component*>& v, Knob* k) { v.push_back (&k->slider); v.push_back (&k->label); };
    auto addChoiceTo = [] (std::vector<juce::Component*>& v, Choice* c) { v.push_back (&c->box); v.push_back (&c->label); };

    // ---- Physical page ----
    exciter = &addChoice ("exciter", "Exciter", "How the object is played. Bow, Breath, Osc and Wavetable keep sounding while you hold the key.");
    attack = &addKnob ("attack", "Attack", Colours::excite, "Fade-in time (sustained exciters, and the Wavetable amp).");
    hardness = &addKnob ("hardness", "Hardness", Colours::excite, "How hard the mallet is, or how bright the sustained exciter is. Velocity adds to it.");
    position = &addKnob ("position", "Position", Colours::excite, "Where on the object you hit it. Changes which overtones ring.");
    oscWave = &addKnob ("oscWave", "Wave", Colours::excite, "Osc exciter waveform: Sine, Triangle, Saw, Square.");
    unison = &addChoice ("unison", "Unison", "Osc exciter: number of stacked, detuned oscillators.");
    detune = &addKnob ("detune", "Detune", Colours::excite, "Osc exciter: how far apart the unison oscillators are tuned.");

    shape = &addKnob ("shape", "Shape", Colours::object, "Morphs the object: String, Tube, Bar, Plate, Bell.");
    material = &addKnob ("material", "Material", Colours::object, "Morphs what it's made of: Rubber, Wood, Metal, Glass.");
    size = &addKnob ("size", "Size", Colours::object, "Tiny to huge. Bigger rings longer, deeper and less perfectly.");
    decay = &addKnob ("decay", "Decay", Colours::object, "Multiplies how long the object rings.");
    release = &addKnob ("release", "Release", Colours::object, "How quickly the sound dies after you let go of the key.");
    octave = &addChoice ("octave", "Octave", "Transpose by octaves.");

    bloom = &addKnob ("bloom", "Bloom", Colours::impossible, "The sound swells louder after the hit instead of only fading.");
    drift = &addKnob ("drift", "Drift", Colours::impossible, "Overtones bend down (left) or up (right) as the note rings.", true);
    transmute = &addKnob ("transmute", "Transmute", Colours::impossible, "The material changes during the note: left heads to rubber, right to glass.", true);
    fracture = &addKnob ("fracture", "Fracture", Colours::impossible, "Overtones split apart and beat against each other, like the object cracking.");
    evolve = &addKnob ("evolve", "Evolve", Colours::impossible, "How fast Bloom, Drift, Transmute and Fracture happen.");

    for (auto* k : { attack, hardness, position, oscWave, detune, shape, material, size, decay, release,
                     bloom, drift, transmute, fracture, evolve })
        addKnobTo (physicalComponents, k);
    for (auto* c : { exciter, unison, octave })
        addChoiceTo (physicalComponents, c);
    physicalComponents.push_back (&editSourceButton);

    // ---- Wavetable page ----
    wtTable = &addChoice ("wtTable", "Table", "Built-in wavetables, or Imported for a WAV you loaded.");
    wtPos = &addKnob ("wtPos", "Position", Colours::wavetable, "Scan through the wavetable's frames.");
    wtWarpMode = &addChoice ("wtWarpMode", "Warp", "Bends the waveform: Bend, Sync, Mirror, Squeeze, Fold, Crush, FM.");
    wtWarp = &addKnob ("wtWarp", "Warp Amt", Colours::wavetable, "How much the Warp mode bends the waveform.");
    wtUnison = &addChoice ("wtUnison", "Unison", "Number of stacked, detuned copies.");
    wtDetune = &addKnob ("wtDetune", "Detune", Colours::wavetable, "How far apart the unison copies are tuned.");
    wtSpread = &addKnob ("wtSpread", "Spread", Colours::wavetable, "Stereo width of the unison copies.");
    wtSub = &addKnob ("wtSub", "Sub", Colours::wavetable, "A sine one octave below.");
    ampDecay = &addKnob ("ampDecay", "Decay", Colours::object, "Amp envelope decay.");
    ampSustain = &addKnob ("ampSustain", "Sustain", Colours::object, "Amp envelope sustain level.");

    wavetableDisplay.colour = Colours::wavetable;
    wavetableDisplay.getTable = [this] { return boundLayer >= 0 ? processor.getLayerWavetable (boundLayer) : nullptr; };
    wavetableDisplay.getPosition = [this] { return rawValue (ImpossibleSynthProcessor::layerId (boundLayer, "wtPos")); };
    content.addAndMakeVisible (wavetableDisplay);

    importButton.setTooltip ("Load a WAV wavetable (Serum/Vital style, 2048 samples per frame) or a single-cycle waveform.");
    importButton.onClick = [this] { importWavetable(); };
    content.addAndMakeVisible (importButton);

    for (auto* k : { wtPos, wtWarp, wtDetune, wtSpread, wtSub, attack, ampDecay, ampSustain, release })
        addKnobTo (wavetableComponents, k);
    for (auto* c : { wtTable, wtWarpMode, wtUnison, octave })
        addChoiceTo (wavetableComponents, c);
    wavetableComponents.push_back (&wavetableDisplay);
    wavetableComponents.push_back (&importButton);
    wavetableComponents.push_back (&backToObjectButton);

    // ---- Cells page ----
    cellRule = &addChoice ("cellRule", "Rule", "The rules of life: how many neighbours make a cell be born or survive.");
    cellSeed = &addChoice ("cellSeed", "Seed", "The starting pattern each note begins with.");
    cellSync = &addChoice ("cellSync", "Sync", "Free uses Speed. Note values step the grid in time with the song.");
    cellDensity = &addKnob ("cellDensity", "Density", Colours::wavetable, "How full the starting grid is (Random/Symmetric seeds; number of Gliders/Spaceships).");
    cellSpeed = &addKnob ("cellSpeed", "Speed", Colours::wavetable, "Generations per second (when Sync is Free).");
    cellGlide = &addKnob ("cellGlide", "Glide", Colours::wavetable, "Smooth blend between generations. 0 = hard steps.");
    cellTilt = &addKnob ("cellTilt", "Tilt", Colours::wavetable, "Darker (right) or brighter (left): how much quieter the high harmonics are.");
    cellMutate = &addKnob ("cellMutate", "Mutate", Colours::wavetable, "Randomly flips cells each generation so life never settles.");
    cellsDisplay.colour = Colours::wavetable;
    cellsDisplay.getDisplay = [this] { return boundLayer >= 0 ? &processor.getDisplay (boundLayer) : nullptr; };
    content.addAndMakeVisible (cellsDisplay);

    for (auto* k : { cellDensity, cellSpeed, cellGlide, cellTilt, cellMutate, wtSub, attack, ampDecay, ampSustain, release })
        addKnobTo (cellsComponents, k);
    for (auto* c : { cellRule, cellSeed, cellSync, octave })
        addChoiceTo (cellsComponents, c);
    cellsComponents.push_back (&cellsDisplay);
    cellsComponents.push_back (&backToObjectButton);

    // ---- Swarm page ----
    swarmCount = &addChoice ("swarmCount", "Particles", "How many particles (each one is a tiny oscillator).");
    swarmHome = &addChoice ("swarmHome", "Home", "The notes the particles are pulled toward.");
    swarmPull = &addKnob ("swarmPull", "Pull", Colours::wavetable, "Gravity toward the home notes. High = clean tone, low = free-floating cloud.");
    swarmChaos = &addKnob ("swarmChaos", "Chaos", Colours::wavetable, "Random kicks that knock the particles around.");
    swarmFlock = &addKnob ("swarmFlock", "Flock", Colours::wavetable, "Right: particles clump together. Left: they scatter apart.", true);
    swarmSpread = &addKnob ("swarmSpread", "Spread", Colours::wavetable, "Squeeze the home notes together (left) or stretch them apart (right). Middle = exact.");
    swarmSpeed = &addKnob ("swarmSpeed", "Speed", Colours::wavetable, "How fast the whole swarm moves.");
    swarmWave = &addKnob ("swarmWave", "Wave", Colours::wavetable, "Each particle's waveform: Sine, Triangle, Saw.");
    swarmDisplay.colour = Colours::wavetable;
    swarmDisplay.getDisplay = [this] { return boundLayer >= 0 ? &processor.getDisplay (boundLayer) : nullptr; };
    content.addAndMakeVisible (swarmDisplay);

    for (auto* k : { swarmPull, swarmChaos, swarmFlock, swarmSpread, swarmSpeed, swarmWave, wtSub, attack, ampDecay, ampSustain, release })
        addKnobTo (swarmComponents, k);
    for (auto* c : { swarmCount, swarmHome, octave })
        addChoiceTo (swarmComponents, c);
    swarmComponents.push_back (&swarmDisplay);
    swarmComponents.push_back (&backToObjectButton);

    editSourceButton.setTooltip ("This layer's exciter comes from another engine: edit it.");
    editSourceButton.onClick = [this]
    {
        const int ex = juce::roundToInt (rawValue (ImpossibleSynthProcessor::layerId (boundLayer, "exciter")));
        showView (view, ex == kExcWavetable ? kLayerWavetable : ex == kExcCells ? kLayerCells : kLayerSwarm);
    };
    content.addChildComponent (editSourceButton);
    backToObjectButton.onClick = [this] { showView (view, -1); };
    content.addChildComponent (backToObjectButton);

    // ---- Shared: filter & modulation ----
    filterMode = &addChoice ("filterMode", "Filter", "Filter type, applied to each note.");
    cutoff = &addKnob ("cutoff", "Cutoff", Colours::filter, "Filter cutoff frequency.");
    resonance = &addKnob ("resonance", "Resonance", Colours::filter, "Filter resonance: emphasis around the cutoff.");
    filterEnv = &addKnob ("filterEnv", "Env Amt", Colours::filter, "How much the Mod Env moves the cutoff (up to 6 octaves either way).", true);
    drive = &addKnob ("drive", "Drive", Colours::filter, "Distortion before the filter.");
    body = &addKnob ("body", "Body", Colours::filter, "How much of the sound goes through the object. Lower it to hear the raw exciter.");

    for (int l = 0; l < kNumLfos; ++l)
    {
        const juce::String n = "lfo" + juce::String (l + 1);
        lfoShape[(size_t) l] = &addChoice (n + "Shape", "Shape", "LFO waveform. S&H jumps randomly, Drift wanders smoothly.");
        lfoSync[(size_t) l] = &addChoice (n + "Sync", "Sync", "Free uses the Rate knob. Note values lock the LFO to the song tempo.");
        lfoRetrig[(size_t) l] = &addChoice (n + "Retrig", "Mode", "Free: all notes share one LFO. Retrig: restarts on every note.");
        lfoRate[(size_t) l] = &addKnob (n + "Rate", "Rate", Colours::mod, "LFO speed (when Sync is Free).");
    }
    envA = &addKnob ("envAttack", "A", Colours::mod, "Mod Env attack.");
    envD = &addKnob ("envDecay", "D", Colours::mod, "Mod Env decay.");
    envS = &addKnob ("envSustain", "S", Colours::mod, "Mod Env sustain level.");
    envR = &addKnob ("envRelease", "R", Colours::mod, "Mod Env release.");

    for (auto* k : { cutoff, resonance, filterEnv, drive, body, lfoRate[0], lfoRate[1], envA, envD, envS, envR })
        addKnobTo (sharedComponents, k);
    for (auto* c : { filterMode, lfoShape[0], lfoSync[0], lfoRetrig[0], lfoShape[1], lfoSync[1], lfoRetrig[1] })
        addChoiceTo (sharedComponents, c);

    for (int i = 0; i < kNumModSlots; ++i)
    {
        auto& row = modRows[(size_t) i];
        row.index = i;
        const juce::String n = "mod" + juce::String (i + 1);
        auto fill = [this] (juce::ComboBox& box, const juce::String& id)
        {
            if (auto* param = dynamic_cast<juce::AudioParameterChoice*> (processor.apvts.getParameter (ImpossibleSynthProcessor::layerId (0, id))))
                box.addItemList (param->choices, 1);
            box.setJustificationType (juce::Justification::centredLeft);
            content.addAndMakeVisible (box);
        };
        fill (row.source, n + "Src");
        fill (row.target, n + "Dst");
        row.source.setTooltip ("Modulation source. Macros 1-4 are the global knobs at the bottom.");
        row.target.setTooltip ("What the source moves.");
        row.amount.setSliderStyle (juce::Slider::LinearHorizontal);
        row.amount.setTextBoxStyle (juce::Slider::TextBoxRight, false, 46, 20);
        row.amount.setColour (juce::Slider::trackColourId, Colours::mod);
        row.amount.setTooltip ("Modulation amount. Double-click to reset.");
        row.amount.setDoubleClickReturnValue (true, 0.0);
        content.addAndMakeVisible (row.amount);
        sharedComponents.push_back (&row.source);
        sharedComponents.push_back (&row.target);
        sharedComponents.push_back (&row.amount);
    }

    // ---- Mix page ----
    for (int i = 0; i < kNumLayers; ++i)
    {
        auto& col = mixColumns[(size_t) i];
        const auto colour = Colours::layers[i];
        col.power.setColour (juce::ToggleButton::tickColourId, colour);
        col.power.setTooltip ("Switch this layer on or off.");
        content.addAndMakeVisible (col.power);
        col.powerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
            processor.apvts, ImpossibleSynthProcessor::layerId (i, "on"), col.power);

        col.type = &addChoice ("type", "Engine", "Physical (objects) or Wavetable.", i);
        col.volume = &addKnob ("volume", "Volume", colour, "Layer volume.", false, i);
        col.pan = &addKnob ("pan", "Pan", colour, "Layer pan.", true, i);
        col.fine = &addKnob ("fine", "Fine", colour, "Fine tune in cents. Detune layers against each other for width.", true, i);
        col.octave = &addChoice ("octave", "Octave", "Transpose this layer by octaves.", i);
        col.keyLow = &addChoice ("keyLow", "Key Low", "Lowest note this layer plays (for keyboard splits).", i);
        col.keyHigh = &addChoice ("keyHigh", "Key High", "Highest note this layer plays (for keyboard splits).", i);
        col.velLow = &addKnob ("velLow", "Vel Low", colour, "Softest velocity this layer plays (for velocity switching).", false, i);
        col.velHigh = &addKnob ("velHigh", "Vel High", colour, "Hardest velocity this layer plays.", false, i);
        col.edit.onClick = [this, i] { showView (i); };
        col.edit.setColour (juce::TextButton::buttonColourId, colour.withAlpha (0.3f));
        content.addAndMakeVisible (col.edit);

        mixComponents.push_back (&col.power);
        mixComponents.push_back (&col.edit);
        for (auto* k : { col.volume, col.pan, col.fine, col.velLow, col.velHigh })
            addKnobTo (mixComponents, k);
        for (auto* c : { col.type, col.octave, col.keyLow, col.keyHigh })
            addChoiceTo (mixComponents, c);
    }

    // ---- Global strip ----
    for (int i = 0; i < kNumMacros; ++i)
        macros[(size_t) i] = &addKnob ("macro" + juce::String (i + 1), "Macro " + juce::String (i + 1), Colours::macro,
                                       "A knob you can route in any layer's Matrix (source: Macro "
                                           + juce::String (i + 1) + "). One macro can move several layers at once.",
                                       false, kGlobal);
    space = &addKnob ("space", "Space", Colours::space, "Amount of reverb.", false, kGlobal);
    room = &addKnob ("room", "Room", Colours::space, "Size of the reverb room.", false, kGlobal);
    output = &addKnob ("output", "Output", Colours::space, "Output volume.", false, kGlobal);

    // ---- Layer tabs ----
    for (int t = 0; t <= kNumLayers; ++t)
    {
        auto& tab = tabs[(size_t) t];
        tab.layer = t - 1;
        tab.colour = t == 0 ? Colours::text : Colours::layers[t - 1];
        tab.onSelect = [this, t] { showView (t - 1); };
        if (t > 0)
        {
            tab.setTooltip ("Click to edit layer " + juce::String (t) + ". Click the dot to switch it on/off.");
            tab.onTogglePower = [this, t]
            {
                if (auto* param = processor.apvts.getParameter (ImpossibleSynthProcessor::layerId (t - 1, "on")))
                {
                    param->beginChangeGesture();
                    param->setValueNotifyingHost (param->getValue() > 0.5f ? 0.0f : 1.0f);
                    param->endChangeGesture();
                }
            };
        }
        else
        {
            tab.setTooltip ("All four layers side by side: volume, pan, tuning, key and velocity ranges.");
        }
        content.addAndMakeVisible (tab);
    }
    engineType = &addChoice ("type", "", "This layer's engine: Physical (objects) or Wavetable.");
    engineLabel.setText ("Engine", juce::dontSendNotification);
    engineLabel.setFont (uiFont (14.0f, true));
    engineLabel.setColour (juce::Label::textColourId, Colours::dim);
    engineLabel.setJustificationType (juce::Justification::centredRight);
    content.addAndMakeVisible (engineLabel);

    // ---- Preset bar ----
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
    randomButton.setTooltip ("Roll a random sound (often with two or three layers).");
    randomButton.setColour (juce::TextButton::buttonColourId, Colours::impossible.withAlpha (0.35f));
    presetBox.onChange = [this] { presetChosen(); };
    refreshPresetList();

    keyboard.setAvailableRange (36, 96);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, Colours::impossible.withAlpha (0.6f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, Colours::impossible.withAlpha (0.25f));
    keyboard.setWantsKeyboardFocus (false);
    keyboard.setOctaveForMiddleC (5); // same note names as FL Studio
    content.addAndMakeVisible (keyboard);

    showView (0);

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

juce::String ImpossibleSynthEditor::paramIdFor (const juce::String& baseId, int layer) const
{
    if (layer == kGlobal)
        return baseId;
    return ImpossibleSynthProcessor::layerId (layer == kCurrentLayer ? juce::jmax (0, boundLayer) : layer, baseId);
}

float ImpossibleSynthEditor::rawValue (const juce::String& id) const
{
    if (auto* v = processor.apvts.getRawParameterValue (id))
        return v->load();
    return 0.0f;
}

ImpossibleSynthEditor::Knob& ImpossibleSynthEditor::addKnob (const juce::String& baseId, const juce::String& name,
                                                             juce::Colour colour, const juce::String& tooltip,
                                                             bool bipolar, int layer)
{
    auto k = std::make_unique<Knob>();
    k->baseId = baseId;
    k->layer = layer;
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

    if (layer != kCurrentLayer)
    {
        const auto id = paramIdFor (baseId, layer);
        k->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, id, k->slider);
        if (auto* param = processor.apvts.getParameter (id))
            k->slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    }

    knobs.push_back (std::move (k));
    return *knobs.back();
}

ImpossibleSynthEditor::Choice& ImpossibleSynthEditor::addChoice (const juce::String& baseId, const juce::String& name,
                                                                 const juce::String& tooltip, int layer)
{
    auto c = std::make_unique<Choice>();
    c->baseId = baseId;
    c->layer = layer;
    const auto id = paramIdFor (baseId, layer == kCurrentLayer ? 0 : layer);
    if (auto* param = dynamic_cast<juce::AudioParameterChoice*> (processor.apvts.getParameter (id)))
        c->box.addItemList (param->choices, 1);
    c->box.setJustificationType (juce::Justification::centred);
    c->box.setTooltip (tooltip);
    content.addAndMakeVisible (c->box);

    c->label.setText (name, juce::dontSendNotification);
    c->label.setJustificationType (juce::Justification::centred);
    c->label.setFont (uiFont (14.0f, true));
    content.addAndMakeVisible (c->label);

    if (layer != kCurrentLayer)
        c->attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, id, c->box);

    choices.push_back (std::move (c));
    return *choices.back();
}

void ImpossibleSynthEditor::bindToLayer (int layer)
{
    boundLayer = layer;
    for (auto& k : knobs)
    {
        if (k->layer != kCurrentLayer)
            continue;
        k->attachment.reset();
        const auto id = ImpossibleSynthProcessor::layerId (layer, k->baseId);
        k->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, id, k->slider);
        if (auto* param = processor.apvts.getParameter (id))
            k->slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    }
    for (auto& c : choices)
    {
        if (c->layer != kCurrentLayer)
            continue;
        c->attachment.reset();
        c->attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            processor.apvts, ImpossibleSynthProcessor::layerId (layer, c->baseId), c->box);
    }
    for (auto& row : modRows)
    {
        const juce::String n = "mod" + juce::String (row.index + 1);
        row.sourceAttachment.reset();
        row.targetAttachment.reset();
        row.amountAttachment.reset();
        row.sourceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            processor.apvts, ImpossibleSynthProcessor::layerId (layer, n + "Src"), row.source);
        row.targetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            processor.apvts, ImpossibleSynthProcessor::layerId (layer, n + "Dst"), row.target);
        row.amountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            processor.apvts, ImpossibleSynthProcessor::layerId (layer, n + "Amt"), row.amount);
    }
}

int ImpossibleSynthEditor::currentLayerType() const
{
    if (boundLayer < 0)
        return kLayerPhysical;
    return juce::roundToInt (rawValue (ImpossibleSynthProcessor::layerId (boundLayer, "type")));
}

void ImpossibleSynthEditor::showView (int newView, int sourcePage)
{
    view = newView;
    if (view >= 0 && view != boundLayer)
        bindToLayer (view);
    shownType = currentLayerType();
    sourceOverride = (view >= 0 && shownType == kLayerPhysical) ? sourcePage : -1;
    layoutContent();
    timerCallback();
    content.repaint();
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
    sections.clear();
    subTitles.clear();
    linksArea = {};

    const bool mixView = view < 0;
    const int page = mixView ? -1 : (sourceOverride >= 0 ? sourceOverride : shownType);

    for (auto* list : { &physicalComponents, &wavetableComponents, &cellsComponents, &swarmComponents,
                        &sharedComponents, &mixComponents })
        for (auto* c : *list)
            c->setVisible (false);
    auto show = [] (std::vector<juce::Component*>& list) { for (auto* c : list) c->setVisible (true); };
    if (mixView)
        show (mixComponents);
    else
    {
        show (page == kLayerWavetable ? wavetableComponents
              : page == kLayerCells   ? cellsComponents
              : page == kLayerSwarm   ? swarmComponents
                                      : physicalComponents);
        show (sharedComponents);
    }
    editSourceButton.setVisible (false); // the timer shows it when relevant
    backToObjectButton.setVisible (sourceOverride >= 0);
    engineLabel.setVisible (! mixView);
    engineType->box.setVisible (! mixView);

    auto area = juce::Rectangle<int> (0, 0, kBaseWidth, kBaseHeight).reduced (16, 0);

    // Header: preset bar on the right.
    {
        auto header = area.removeFromTop (56).withSizeKeepingCentre (area.getWidth(), 32);
        randomButton.setBounds (header.removeFromRight (110));
        header.removeFromRight (8);
        saveButton.setBounds (header.removeFromRight (64));
        header.removeFromRight (8);
        nextButton.setBounds (header.removeFromRight (32));
        header.removeFromRight (4);
        presetBox.setBounds (header.removeFromRight (260));
        header.removeFromRight (4);
        prevButton.setBounds (header.removeFromRight (32));
    }

    // Layer tabs.
    {
        auto bar = area.removeFromTop (46);
        tabs[0].setBounds (bar.removeFromLeft (90).reduced (0, 2));
        for (int t = 1; t <= kNumLayers; ++t)
        {
            bar.removeFromLeft (8);
            tabs[(size_t) t].setBounds (bar.removeFromLeft (168).reduced (0, 2));
        }
        engineType->box.setBounds (bar.removeFromRight (150).withSizeKeepingCentre (150, 30));
        engineLabel.setBounds (bar.removeFromRight (80));
    }
    area.removeFromTop (10);

    auto kb = area.removeFromBottom (88);
    kb.removeFromBottom (12);
    keyboard.setBounds (kb);
    keyboard.setKeyWidth ((float) kb.getWidth() / 36.0f); // 36 white keys in C3..C8
    area.removeFromBottom (12);

    // Global strip: macros, space, output.
    {
        auto strip = area.removeFromBottom (124);
        area.removeFromBottom (12);
        Section macroSection { "MACROS", Colours::macro, strip.removeFromLeft (620) };
        strip.removeFromLeft (12);
        Section spaceSection { "SPACE", Colours::space, strip.removeFromLeft (380) };
        strip.removeFromLeft (12);
        Section outSection { "OUTPUT", Colours::space, strip };

        // Label on the left, knob (with its value underneath) on the right.
        auto placeSmall = [] (Knob* k, juce::Rectangle<int> cell)
        {
            auto labelArea = cell.removeFromLeft (cell.getWidth() * 2 / 5);
            k->label.setBounds (labelArea.withSizeKeepingCentre (labelArea.getWidth(), 20));
            k->slider.setBounds (cell);
        };
        {
            auto r = macroSection.bounds.reduced (10).withTrimmedTop (24);
            const int w = r.getWidth() / kNumMacros;
            for (int i = 0; i < kNumMacros; ++i)
                placeSmall (macros[(size_t) i], i == kNumMacros - 1 ? r : r.removeFromLeft (w));
        }
        {
            auto r = spaceSection.bounds.reduced (10).withTrimmedTop (24);
            placeSmall (space, r.removeFromLeft (r.getWidth() / 2));
            placeSmall (room, r);
        }
        placeSmall (output, outSection.bounds.reduced (10).withTrimmedTop (24));

        sections.push_back (macroSection);
        sections.push_back (spaceSection);
        sections.push_back (outSection);
    }

    if (mixView)
        layoutMixPage (area);
    else if (page == kLayerWavetable)
        layoutWavetablePage (area);
    else if (page == kLayerCells)
        layoutCellsPage (area);
    else if (page == kLayerSwarm)
        layoutSwarmPage (area);
    else
        layoutPhysicalPage (area);
}

namespace
{
std::vector<juce::Rectangle<int>> cellsOf (juce::Rectangle<int> r, int count)
{
    std::vector<juce::Rectangle<int>> cells;
    const int w = r.getWidth() / count;
    for (int i = 0; i < count; ++i)
        cells.push_back (i == count - 1 ? r : r.removeFromLeft (w));
    return cells;
}

juce::Rectangle<int> innerOf (juce::Rectangle<int> section)
{
    return section.reduced (10).withTrimmedTop (30);
}
} // namespace

void ImpossibleSynthEditor::layoutPhysicalPage (juce::Rectangle<int> page)
{
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

    const int gap = 12;
    auto rowA = page.removeFromTop (196);
    page.removeFromTop (gap);
    auto rowB = page.removeFromTop (196);
    page.removeFromTop (gap);
    auto rowC = page;

    const auto colour = Colours::layers[juce::jmax (0, boundLayer)];
    juce::ignoreUnused (colour);

    Section excite { "EXCITE", Colours::excite, rowA.removeFromLeft (590) };
    rowA.removeFromLeft (gap);
    Section object { "OBJECT", Colours::object, rowA };
    Section imp { "IMPOSSIBLE", Colours::impossible, rowB.removeFromLeft (590) };
    rowB.removeFromLeft (gap);
    sections.push_back (excite);
    sections.push_back (object);
    sections.push_back (imp);

    {
        auto r = innerOf (excite.bounds);
        auto exciterCell = r.removeFromLeft (112);
        exciter->label.setBounds (exciterCell.removeFromTop (20));
        exciter->box.setBounds (exciterCell.withSizeKeepingCentre (106, 30).translated (0, -10));
        auto c = cellsOf (r, 6);
        placeKnob (attack, c[0]);
        placeKnob (hardness, c[1]);
        placeKnob (position, c[2]);
        placeKnob (oscWave, c[3]);
        placeChoice (unison, c[4]);
        placeKnob (detune, c[5]);
        editSourceButton.setBounds (excite.bounds.getRight() - 160, excite.bounds.getY() + 8, 150, 26);
    }
    {
        auto c = cellsOf (innerOf (object.bounds), 6);
        placeKnob (shape, c[0]);
        placeKnob (material, c[1]);
        placeKnob (size, c[2]);
        placeKnob (decay, c[3]);
        placeKnob (release, c[4]);
        placeChoice (octave, c[5]);
    }
    {
        auto c = cellsOf (innerOf (imp.bounds), 5);
        placeKnob (bloom, c[0]);
        placeKnob (drift, c[1]);
        placeKnob (transmute, c[2]);
        placeKnob (fracture, c[3]);
        placeKnob (evolve, c[4]);
    }

    layoutSharedSections (rowB, rowC);
}

void ImpossibleSynthEditor::layoutWavetablePage (juce::Rectangle<int> page)
{
    auto placeKnob = [] (Knob* k, juce::Rectangle<int> cell)
    {
        k->label.setBounds (cell.removeFromTop (20));
        k->slider.setBounds (cell.reduced (2, 0));
    };
    auto placeChoice = [] (Choice* c, juce::Rectangle<int> cell)
    {
        c->label.setBounds (cell.removeFromTop (20));
        c->box.setBounds (cell.withSizeKeepingCentre (juce::jmin (cell.getWidth() - 8, 120), 30).translated (0, -10));
    };

    const int gap = 12;
    auto rowA = page.removeFromTop (196);
    page.removeFromTop (gap);
    auto rowB = page.removeFromTop (196);
    page.removeFromTop (gap);
    auto rowC = page;

    Section wt { sourceOverride >= 0 ? "WAVETABLE  (exciter)" : "WAVETABLE", Colours::wavetable, rowA.removeFromLeft (800) };
    rowA.removeFromLeft (gap);
    Section osc { "UNISON & SUB", Colours::wavetable, rowA };
    Section amp { sourceOverride >= 0 ? "ENVELOPE" : "AMP", Colours::object, rowB.removeFromLeft (590) };
    rowB.removeFromLeft (gap);
    sections.push_back (wt);
    sections.push_back (osc);
    sections.push_back (amp);

    {
        auto r = innerOf (wt.bounds);
        wavetableDisplay.setBounds (r.removeFromLeft (330));
        r.removeFromLeft (14);
        auto top = r.removeFromTop (34);
        wtTable->label.setBounds ({}); // the display shows the name
        wtTable->box.setBounds (top.removeFromLeft (260).withSizeKeepingCentre (260, 30));
        top.removeFromLeft (8);
        importButton.setBounds (top.removeFromLeft (120).withSizeKeepingCentre (120, 30));
        r.removeFromTop (4);
        auto c = cellsOf (r, 3);
        placeKnob (wtPos, c[0]);
        placeChoice (wtWarpMode, c[1]);
        placeKnob (wtWarp, c[2]);
        backToObjectButton.setBounds (wt.bounds.getRight() - 170, wt.bounds.getY() + 8, 160, 26);
    }
    {
        auto c = cellsOf (innerOf (osc.bounds), 4);
        placeChoice (wtUnison, c[0]);
        placeKnob (wtDetune, c[1]);
        placeKnob (wtSpread, c[2]);
        placeKnob (wtSub, c[3]);
    }
    {
        auto c = cellsOf (innerOf (amp.bounds), 5);
        placeKnob (attack, c[0]);
        placeKnob (ampDecay, c[1]);
        placeKnob (ampSustain, c[2]);
        placeKnob (release, c[3]);
        placeChoice (octave, c[4]);
    }

    layoutSharedSections (rowB, rowC);
}

void ImpossibleSynthEditor::layoutSourceRowB (juce::Rectangle<int> rowB, juce::Rectangle<int> rowC)
{
    auto placeKnob = [] (Knob* k, juce::Rectangle<int> cell)
    {
        k->label.setBounds (cell.removeFromTop (20));
        k->slider.setBounds (cell.reduced (2, 0));
    };
    Section amp { sourceOverride >= 0 ? "ENVELOPE" : "AMP", Colours::object, rowB.removeFromLeft (590) };
    rowB.removeFromLeft (12);
    sections.push_back (amp);

    auto c = cellsOf (innerOf (amp.bounds), 5);
    placeKnob (attack, c[0]);
    placeKnob (ampDecay, c[1]);
    placeKnob (ampSustain, c[2]);
    placeKnob (release, c[3]);
    octave->label.setBounds (c[4].removeFromTop (20));
    octave->box.setBounds (c[4].withSizeKeepingCentre (juce::jmin (c[4].getWidth() - 8, 110), 30).translated (0, -10));

    layoutSharedSections (rowB, rowC);
}

void ImpossibleSynthEditor::layoutCellsPage (juce::Rectangle<int> page)
{
    auto placeKnob = [] (Knob* k, juce::Rectangle<int> cell)
    {
        k->label.setBounds (cell.removeFromTop (20));
        k->slider.setBounds (cell.reduced (2, 0));
    };
    auto placeChoice = [] (Choice* c, juce::Rectangle<int> cell)
    {
        c->label.setBounds (cell.removeFromTop (20));
        c->box.setBounds (cell.withSizeKeepingCentre (juce::jmin (cell.getWidth() - 8, 136), 28).withY (cell.getY()));
    };

    const int gap = 12;
    auto rowA = page.removeFromTop (196);
    page.removeFromTop (gap);
    auto rowB = page.removeFromTop (196);
    page.removeFromTop (gap);

    Section cells { sourceOverride >= 0 ? "CELLS  (exciter)" : "CELLS", Colours::wavetable, rowA.removeFromLeft (800) };
    rowA.removeFromLeft (gap);
    Section tone { "TONE", Colours::wavetable, rowA };
    sections.push_back (cells);
    sections.push_back (tone);

    {
        auto r = innerOf (cells.bounds);
        cellsDisplay.setBounds (r.removeFromLeft (330));
        r.removeFromLeft (14);
        auto combos = cellsOf (r.removeFromTop (52), 3);
        placeChoice (cellRule, combos[0]);
        placeChoice (cellSeed, combos[1]);
        placeChoice (cellSync, combos[2]);
        r.removeFromTop (4);
        auto knobsRow = cellsOf (r.withTrimmedLeft (r.getWidth() / 6).withTrimmedRight (r.getWidth() / 6), 2);
        placeKnob (cellDensity, knobsRow[0]);
        placeKnob (cellSpeed, knobsRow[1]);
        backToObjectButton.setBounds (cells.bounds.getRight() - 170, cells.bounds.getY() + 8, 160, 26);
    }
    {
        auto c = cellsOf (innerOf (tone.bounds), 4);
        placeKnob (cellGlide, c[0]);
        placeKnob (cellTilt, c[1]);
        placeKnob (cellMutate, c[2]);
        placeKnob (wtSub, c[3]);
    }

    layoutSourceRowB (rowB, page);
}

void ImpossibleSynthEditor::layoutSwarmPage (juce::Rectangle<int> page)
{
    auto placeKnob = [] (Knob* k, juce::Rectangle<int> cell)
    {
        k->label.setBounds (cell.removeFromTop (20));
        k->slider.setBounds (cell.reduced (2, 0));
    };
    auto placeChoice = [] (Choice* c, juce::Rectangle<int> cell)
    {
        c->label.setBounds (cell.removeFromTop (20));
        c->box.setBounds (cell.withSizeKeepingCentre (juce::jmin (cell.getWidth() - 8, 136), 28).withY (cell.getY()));
    };

    const int gap = 12;
    auto rowA = page.removeFromTop (196);
    page.removeFromTop (gap);
    auto rowB = page.removeFromTop (196);
    page.removeFromTop (gap);

    Section swarm { sourceOverride >= 0 ? "SWARM  (exciter)" : "SWARM", Colours::wavetable, rowA.removeFromLeft (800) };
    rowA.removeFromLeft (gap);
    Section motion { "MOTION", Colours::wavetable, rowA };
    sections.push_back (swarm);
    sections.push_back (motion);

    {
        auto r = innerOf (swarm.bounds);
        swarmDisplay.setBounds (r.removeFromLeft (330));
        r.removeFromLeft (14);
        auto combos = cellsOf (r.removeFromTop (52).withTrimmedRight (r.getWidth() / 3), 2);
        placeChoice (swarmCount, combos[0]);
        placeChoice (swarmHome, combos[1]);
        r.removeFromTop (4);
        auto knobsRow = cellsOf (r, 3);
        placeKnob (swarmPull, knobsRow[0]);
        placeKnob (swarmChaos, knobsRow[1]);
        placeKnob (swarmFlock, knobsRow[2]);
        backToObjectButton.setBounds (swarm.bounds.getRight() - 170, swarm.bounds.getY() + 8, 160, 26);
    }
    {
        auto c = cellsOf (innerOf (motion.bounds), 4);
        placeKnob (swarmSpread, c[0]);
        placeKnob (swarmSpeed, c[1]);
        placeKnob (swarmWave, c[2]);
        placeKnob (wtSub, c[3]);
    }

    layoutSourceRowB (rowB, page);
}

void ImpossibleSynthEditor::layoutSharedSections (juce::Rectangle<int> filterArea, juce::Rectangle<int> modArea)
{
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

    Section filter { "FILTER & DRIVE", Colours::filter, filterArea };
    Section mod { "MODULATION", Colours::mod, modArea };
    sections.push_back (filter);
    sections.push_back (mod);

    {
        auto c = cellsOf (innerOf (filter.bounds), 6);
        placeChoice (filterMode, c[0]);
        placeKnob (cutoff, c[1]);
        placeKnob (resonance, c[2]);
        placeKnob (filterEnv, c[3]);
        placeKnob (drive, c[4]);
        placeKnob (body, c[5]);
    }

    auto r = innerOf (mod.bounds);
    for (int l = 0; l < kNumLfos; ++l)
    {
        auto block = r.removeFromLeft (200);
        r.removeFromLeft (16);
        subTitles.push_back ({ block.removeFromTop (20), "LFO " + juce::String (l + 1) });
        block.removeFromTop (4);
        auto combos = block.removeFromLeft (100);
        for (auto* c : { lfoShape[(size_t) l], lfoSync[(size_t) l], lfoRetrig[(size_t) l] })
        {
            c->label.setBounds (combos.removeFromTop (18));
            c->box.setBounds (combos.removeFromTop (28));
            combos.removeFromTop (8);
        }
        placeKnob (lfoRate[(size_t) l], block.withTrimmedTop (30).withTrimmedBottom (30));
    }

    auto envBlock = r.removeFromLeft (250);
    r.removeFromLeft (16);
    subTitles.push_back ({ envBlock.removeFromTop (20), "MOD ENV" });
    envBlock.removeFromTop (30);
    envBlock.removeFromBottom (30);
    auto c = cellsOf (envBlock, 4);
    placeKnob (envA, c[0]);
    placeKnob (envD, c[1]);
    placeKnob (envS, c[2]);
    placeKnob (envR, c[3]);

    subTitles.push_back ({ r.removeFromTop (20), "MATRIX" });
    r.removeFromTop (3);
    for (auto& m : modRows)
    {
        auto line = r.removeFromTop (24);
        r.removeFromTop (3);
        m.source.setBounds (line.removeFromLeft (130));
        line.removeFromLeft (4);
        m.target.setBounds (line.removeFromLeft (140));
        line.removeFromLeft (8);
        m.amount.setBounds (line);
    }
}

void ImpossibleSynthEditor::layoutMixPage (juce::Rectangle<int> page)
{
    auto placeKnob = [] (Knob* k, juce::Rectangle<int> cell)
    {
        k->label.setBounds (cell.removeFromTop (20));
        k->slider.setBounds (cell.reduced (2, 0));
    };
    auto placeChoice = [] (Choice* c, juce::Rectangle<int> cell)
    {
        c->label.setBounds (cell.removeFromTop (20));
        c->box.setBounds (cell.withSizeKeepingCentre (juce::jmin (cell.getWidth() - 6, 120), 28));
    };

    linksArea = page.removeFromBottom (150);
    page.removeFromBottom (12);
    sections.push_back ({ "HOW LAYERS LINK", Colours::macro, linksArea });

    const int gap = 12;
    const int w = (page.getWidth() - gap * (kNumLayers - 1)) / kNumLayers;
    for (int i = 0; i < kNumLayers; ++i)
    {
        auto colBounds = page.removeFromLeft (i == kNumLayers - 1 ? page.getWidth() : w);
        page.removeFromLeft (gap);
        sections.push_back ({ "LAYER " + juce::String (i + 1), Colours::layers[i], colBounds });

        auto& col = mixColumns[(size_t) i];
        auto r = colBounds.reduced (12).withTrimmedTop (28);

        auto top = r.removeFromTop (32);
        col.power.setBounds (top.removeFromLeft (64));
        top.removeFromLeft (8);
        col.type->label.setBounds ({});
        col.type->box.setBounds (top);
        r.removeFromTop (10);

        auto knobsRow = cellsOf (r.removeFromTop (110), 3);
        placeKnob (col.volume, knobsRow[0]);
        placeKnob (col.pan, knobsRow[1]);
        placeKnob (col.fine, knobsRow[2]);
        r.removeFromTop (6);

        auto rangeRow = cellsOf (r.removeFromTop (52), 3);
        placeChoice (col.octave, rangeRow[0]);
        placeChoice (col.keyLow, rangeRow[1]);
        placeChoice (col.keyHigh, rangeRow[2]);
        r.removeFromTop (6);

        auto velRow = cellsOf (r.removeFromTop (100).withTrimmedLeft (r.getWidth() / 6).withTrimmedRight (r.getWidth() / 6), 2);
        placeKnob (col.velLow, velRow[0]);
        placeKnob (col.velHigh, velRow[1]);

        col.edit.setBounds (r.removeFromBottom (32));
    }
}

void ImpossibleSynthEditor::paintContent (juce::Graphics& g)
{
    g.fillAll (Colours::background);

    // Header.
    auto header = juce::Rectangle<int> (0, 0, kBaseWidth, 56).reduced (24, 0);
    g.setColour (Colours::text);
    g.setFont (uiFont (26.0f, true));
    g.drawText ("IMPOSSIBLE SYNTH", header.removeFromLeft (300), juce::Justification::centredLeft);
    g.setColour (Colours::dim);
    g.setFont (uiFont (13.0f));
    g.drawText ("v0.4", header.removeFromLeft (40), juce::Justification::centredLeft);

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
        g.drawText (s.title, (int) r.getX() + 26, (int) r.getY() + 10, 300, 22, juce::Justification::centredLeft);
    }

    // Which layer is being edited.
    if (view >= 0 && ! sections.empty())
    {
        g.setColour (Colours::layers[juce::jmax (0, boundLayer)].withAlpha (0.8f));
        g.fillRect (16, 102, kBaseWidth - 32, 2);
    }

    g.setFont (uiFont (12.0f, true));
    for (auto& [r, text] : subTitles)
    {
        g.setColour (Colours::dim);
        g.drawText (text, r, juce::Justification::centredLeft);
        g.setColour (Colours::panelEdge);
        g.drawHorizontalLine (r.getBottom(), (float) r.getX(), (float) r.getRight());
    }

    if (! linksArea.isEmpty())
    {
        auto r = linksArea.reduced (26, 12).withTrimmedTop (28);
        const char* lines[] = {
            "Macros: route Macro 1-4 in any layer's Matrix. One macro knob can move several layers at once (e.g. fade one layer in while another fades out).",
            "Wavetable into Object: set a Physical layer's Exciter to Wavetable, then use Edit Wavetable to pick the table that gets played through the object.",
            "Splits and switches: Key Low/High give each layer its own part of the keyboard; Vel Low/High swap layers depending on how hard you play.",
            "Stacks: switch on more layers to play them together. Detune them slightly with Fine, or spread them with Pan, for a wider sound."
        };
        g.setFont (uiFont (13.5f));
        for (auto* line : lines)
        {
            g.setColour (Colours::macro);
            g.fillEllipse ((float) r.getX(), (float) r.getY() + 8.0f, 5.0f, 5.0f);
            g.setColour (Colours::text.withAlpha (0.85f));
            g.drawText (line, r.removeFromTop (22).withTrimmedLeft (14), juce::Justification::centredLeft);
        }
    }
}

void ImpossibleSynthEditor::timerCallback()
{
    // Tabs.
    for (int t = 0; t <= kNumLayers; ++t)
    {
        auto& tab = tabs[(size_t) t];
        const bool selected = (t == 0) ? view < 0 : view == t - 1;
        bool changed = tab.selected != selected;
        tab.selected = selected;
        if (t > 0)
        {
            const bool on = rawValue (ImpossibleSynthProcessor::layerId (t - 1, "on")) > 0.5f;
            const int type = juce::roundToInt (rawValue (ImpossibleSynthProcessor::layerId (t - 1, "type")));
            const juce::String name = kLayerTypeNames[juce::jlimit (0, kNumLayerTypes - 1, type)];
            changed = changed || tab.enabled != on || tab.engineName != name;
            tab.enabled = on;
            tab.engineName = name;
        }
        if (changed)
            tab.repaint();
    }

    if (view >= 0)
    {
        // The engine type changed (e.g. from the Engine box or a preset): switch page.
        const int type = currentLayerType();
        if (type != shownType)
        {
            showView (view, false);
            return;
        }

        auto id = [this] (const char* base) { return ImpossibleSynthProcessor::layerId (boundLayer, base); };
        const int ex = juce::roundToInt (rawValue (id ("exciter")));
        const bool physical = type == kLayerPhysical;
        const bool continuous = ex == kBow || ex == kBreath || ex == kOsc || ex == kExcWavetable;
        const bool filterOn = juce::roundToInt (rawValue (id ("filterMode"))) != kFilterOff;

        attack->slider.setEnabled (! physical || sourceOverride >= 0 || continuous);
        oscWave->slider.setEnabled (ex == kOsc);
        unison->box.setEnabled (ex == kOsc);
        detune->slider.setEnabled (ex == kOsc);
        for (auto* k : { cutoff, resonance, filterEnv })
            k->slider.setEnabled (filterOn);
        body->slider.setEnabled (physical);
        ampDecay->slider.setEnabled (! physical);
        ampSustain->slider.setEnabled (! physical);
        lfoRate[0]->slider.setEnabled (juce::roundToInt (rawValue (id ("lfo1Sync"))) == 0);
        lfoRate[1]->slider.setEnabled (juce::roundToInt (rawValue (id ("lfo2Sync"))) == 0);

        const int borrowed = ex == kExcWavetable ? kLayerWavetable : ex == kExcCells ? kLayerCells : ex == kExcSwarm ? kLayerSwarm : -1;
        const bool onPhysicalPage = physical && sourceOverride < 0;
        editSourceButton.setVisible (onPhysicalPage && borrowed >= 0);
        if (borrowed >= 0)
        {
            const auto text = juce::String ("Edit ") + kLayerTypeNames[borrowed] + juce::String::fromUTF8 (" \xe2\x80\xba");
            if (editSourceButton.getButtonText() != text)
                editSourceButton.setButtonText (text);
        }
        if (sourceOverride >= 0 && borrowed != sourceOverride)
        {
            showView (view, -1); // the exciter changed away from that engine
            return;
        }

        wavetableDisplay.refresh();
    }

    if (processor.getPresetName() != shownPresetName)
        refreshPresetList();
}

//==============================================================================
void ImpossibleSynthEditor::importWavetable()
{
    fileChooser = std::make_unique<juce::FileChooser> ("Import a wavetable", juce::File(), "*.wav;*.aif;*.aiff;*.flac");
    juce::Component::SafePointer<ImpossibleSynthEditor> safeThis (this);
    const int layer = boundLayer;
    fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [safeThis, layer] (const juce::FileChooser& chooser)
    {
        if (safeThis == nullptr)
            return;
        const auto file = chooser.getResult();
        if (! file.existsAsFile())
            return;
        if (! safeThis->processor.importWavetable (layer, file))
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Import WAV",
                                                    "Couldn't read that file as a wavetable.");
        safeThis->wavetableDisplay.refresh();
    });
}

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
