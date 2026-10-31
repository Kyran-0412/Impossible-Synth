// Developer tool: checks that saved states load correctly, including saves
// from v0.1/v0.2 (single layer, unprefixed IDs), which must land in layer 1.
// Build with -DIMPOSSIBLE_UI_SNAPSHOT=ON.

#include "PluginProcessor.h"

#include <cstdio>

static int failures = 0;

static void check (bool ok, const char* what)
{
    std::printf ("%s  %s\n", ok ? "ok  " : "FAIL", what);
    if (! ok)
        ++failures;
}

static float value (ImpossibleSynthProcessor& p, const juce::String& id)
{
    return p.apvts.getRawParameterValue (id)->load();
}

// Processors are big (every voice owns its buffers), so they go on the heap
// like a DAW would create them: Windows only gives a program 1 MB of stack.
static std::unique_ptr<ImpossibleSynthProcessor> makeProcessor()
{
    return std::make_unique<ImpossibleSynthProcessor>();
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;

    // 1. A v0.2-style state: no "version", unprefixed IDs.
    {
        auto owner = makeProcessor();
        auto& p = *owner;
        p.setCurrentProgram (35); // something with several layers on, to prove they get switched off

        juce::XmlElement xml ("ImpossibleSynth");
        xml.setAttribute ("presetName", "My Old Sound");
        auto add = [&xml] (const char* id, double v)
        {
            auto* e = xml.createNewChildElement ("PARAM");
            e->setAttribute ("id", id);
            e->setAttribute ("value", v);
        };
        add ("shape", 0.9);
        add ("material", 0.2);
        add ("exciter", 2);
        add ("mod1Src", 1);
        add ("mod1Dst", 6);
        add ("mod1Amt", 0.5);
        add ("space", 0.7);

        juce::MemoryBlock block;
        juce::AudioProcessor::copyXmlToBinary (xml, block);
        p.setStateInformation (block.getData(), (int) block.getSize());

        check (std::abs (value (p, "L1_shape") - 0.9f) < 1e-4f, "old shape -> L1_shape");
        check (std::abs (value (p, "L1_material") - 0.2f) < 1e-4f, "old material -> L1_material");
        check (juce::roundToInt (value (p, "L1_exciter")) == 2, "old exciter -> L1_exciter");
        check (juce::roundToInt (value (p, "L1_mod1Dst")) == 6, "old matrix slot -> L1_mod1Dst");
        check (std::abs (value (p, "space") - 0.7f) < 1e-4f, "global space stays global");
        check (value (p, "L1_on") > 0.5f, "layer 1 on");
        check (value (p, "L2_on") < 0.5f && value (p, "L3_on") < 0.5f && value (p, "L4_on") < 0.5f,
               "layers 2-4 off after loading an old single-layer sound");
        check (juce::roundToInt (value (p, "L1_type")) == impossible::kLayerPhysical, "layer 1 is Physical");
        check (p.getPresetName() == "My Old Sound", "preset name kept");
    }

    // 2. Round trip of a new layered state.
    {
        auto ownerA = makeProcessor(), ownerB = makeProcessor();
        auto& a = *ownerA;
        auto& b = *ownerB;
        a.setCurrentProgram (35);
        juce::MemoryBlock block;
        a.getStateInformation (block);
        b.setStateInformation (block.getData(), (int) block.getSize());

        bool same = true;
        for (auto* param : a.getParameters())
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
                if (std::abs (ranged->getValue() - b.apvts.getParameter (ranged->getParameterID())->getValue()) > 1e-5f)
                    same = false;
        check (same, "layered state round-trips exactly");
        check (b.getPresetName() == a.getPresetName(), "preset name round-trips");
    }

    // 3. Every factory preset applies without leaving stray layers on.
    {
        auto owner = makeProcessor();
        auto& p = *owner;
        bool ok = true;
        for (int i = 0; i < p.getNumPrograms(); ++i)
        {
            p.setCurrentProgram (i);
            auto patch = impossible::factoryPresets()[(size_t) i].patch;
            for (int l = 0; l < impossible::kNumLayers; ++l)
                if ((value (p, ImpossibleSynthProcessor::layerId (l, "on")) > 0.5f) != (patch.layers[(size_t) l].enabled != 0))
                    ok = false;
        }
        check (ok, "factory presets set every layer's on/off");
    }

    std::printf ("%s\n", failures == 0 ? "ALL PASSED" : "SOME FAILED");
    return failures == 0 ? 0 : 1;
}
