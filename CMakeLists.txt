cmake_minimum_required(VERSION 3.22)

# Link the C++ runtime statically on Windows so the plugin works without
# installing the Visual C++ redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(ImpossibleSynth VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

# JUCE is downloaded automatically the first time you configure.
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.12
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(ImpossibleSynth
    PRODUCT_NAME "Impossible Synth"
    COMPANY_NAME "Kyran"
    BUNDLE_ID "com.kyran.impossiblesynth"
    PLUGIN_MANUFACTURER_CODE Kyrn
    PLUGIN_CODE ImpS
    FORMATS VST3 Standalone
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Synth)

target_sources(ImpossibleSynth PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/dsp/Engine.cpp)

target_include_directories(ImpossibleSynth PRIVATE Source)

target_compile_definitions(ImpossibleSynth PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(ImpossibleSynth
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Engine test that runs without JUCE: renders WAVs and checks levels.
add_executable(render_test tools/render_test.cpp Source/dsp/Engine.cpp)
target_include_directories(render_test PRIVATE Source)
