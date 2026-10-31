// Developer tool: renders the plugin UI to PNG files without a DAW.
// Build with -DIMPOSSIBLE_UI_SNAPSHOT=ON, then run (on Linux, under xvfb-run):
//   ui_snapshot <output-folder> [preset-index]
// Writes one image per tab: mix.png, layer1.png ... layer4.png.

#include "PluginEditor.h"
#include "PluginProcessor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File outDir (argc > 1 ? juce::File::getCurrentWorkingDirectory().getChildFile (argv[1])
                                      : juce::File::getCurrentWorkingDirectory());
    outDir.createDirectory();

    // On the heap, like a DAW creates it (it's too big for Windows' 1 MB stack).
    auto processorOwner = std::make_unique<ImpossibleSynthProcessor>();
    auto& processor = *processorOwner;
    if (argc > 2)
        processor.setCurrentProgram (juce::String (argv[2]).getIntValue());

    // Play a chord for a moment so the live displays (cells, swarm) have something to show.
    processor.prepareToPlay (48000.0, 512);
    {
        juce::AudioBuffer<float> buffer (2, 512);
        for (int block = 0; block < 140; ++block)
        {
            juce::MidiBuffer midi;
            if (block == 0)
                for (int note : { 48, 60, 64, 67 })
                    midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), 0);
            processor.processBlock (buffer, midi);
        }
    }

    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    editor->setSize (ImpossibleSynthEditor::kBaseWidth, ImpossibleSynthEditor::kBaseHeight);

    // Find the layer tabs and "click" each one.
    std::vector<LayerTab*> tabs;
    std::function<void (juce::Component&)> collect = [&] (juce::Component& c)
    {
        if (auto* tab = dynamic_cast<LayerTab*> (&c))
            tabs.push_back (tab);
        for (auto* child : c.getChildren())
            collect (*child);
    };
    collect (*editor);

    for (auto* tab : tabs)
    {
        tab->onSelect();
        const auto name = tab->layer < 0 ? juce::String ("mix") : "layer" + juce::String (tab->layer + 1);
        auto image = editor->createComponentSnapshot (editor->getLocalBounds());
        juce::FileOutputStream out (outDir.getChildFile (name + ".png"));
        out.setPosition (0);
        out.truncate();
        juce::PNGImageFormat().writeImageToStream (image, out);
    }
    return 0;
}
