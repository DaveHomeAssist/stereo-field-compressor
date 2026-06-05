cmake_minimum_required(VERSION 3.22)

project(StereoFieldCompressor VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- JUCE via FetchContent ----
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.13
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# ---- Plugin target ----
juce_add_plugin(StereoFieldCompressor
    COMPANY_NAME              "Dave Robertson"
    BUNDLE_ID                 com.daverobertson.stereofieldcompressor
    PLUGIN_MANUFACTURER_CODE  Drob         # Dave Robertson (vendor) — final
    PLUGIN_CODE               Sfc1         # Stereo Field Compressor v1 — final
    FORMATS                   VST3 Standalone
    PRODUCT_NAME              "Stereo Field Compressor"
    IS_SYNTH                  FALSE
    NEEDS_MIDI_INPUT          FALSE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    HARDENED_RUNTIME_ENABLED  TRUE
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Stereo Field Compressor uses the microphone to process audio input."
    COPY_PLUGIN_AFTER_BUILD   TRUE
    VST3_CATEGORIES           Fx Dynamics
)

target_sources(StereoFieldCompressor PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/SpatialDetector.cpp
    Source/ConeWindow.cpp
    Source/ConeCompressor.cpp
)

target_compile_definitions(StereoFieldCompressor PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(StereoFieldCompressor PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp          # retained for planned juce::dsp::Oversampling (see README next-steps)
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
