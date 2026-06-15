cmake_minimum_required(VERSION 3.22)

project(BoratoEq VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BORATOEQ_FETCH_JUCE "Download JUCE if it is not found locally" OFF)
option(BORATOEQ_COPY_PLUGIN_AFTER_BUILD "Copy the built plugin to the system plugin folder after build" OFF)
option(BORATOEQ_BUILD_CLAP "Build the CLAP plugin through clap-juce-extensions" OFF)
set(BORATOEQ_JUCE_SOURCE_DIR "" CACHE PATH "Path to a JUCE source checkout")

# ----------------------------------------------------------------------------
# Locate JUCE. Mirrors the borato-224 strategy: prefer a local checkout
# (C:/JUCE on this machine), fall back to find_package, then optionally fetch.
# ----------------------------------------------------------------------------
if(NOT BORATOEQ_JUCE_SOURCE_DIR AND EXISTS "C:/JUCE/CMakeLists.txt")
    set(BORATOEQ_JUCE_SOURCE_DIR "C:/JUCE")
endif()

if(BORATOEQ_JUCE_SOURCE_DIR AND EXISTS "${BORATOEQ_JUCE_SOURCE_DIR}/CMakeLists.txt")
    set(JUCE_BUILD_HELPER_TOOLS OFF CACHE BOOL "Bootstrap JUCE helper tools at configure time" FORCE)
    add_subdirectory("${BORATOEQ_JUCE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/juce" EXCLUDE_FROM_ALL)
else()
    find_package(JUCE CONFIG QUIET)
    if(NOT JUCE_FOUND)
        if(NOT BORATOEQ_FETCH_JUCE)
            message(FATAL_ERROR "JUCE not found. Set BORATOEQ_JUCE_SOURCE_DIR/JUCE_DIR/CMAKE_PREFIX_PATH or enable BORATOEQ_FETCH_JUCE.")
        endif()

        include(FetchContent)
        FetchContent_Declare(
            juce
            GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
            GIT_TAG 8.0.12
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(juce)
    endif()
endif()

# Optional CLAP support reusing the submodule from the sibling borato-224 repo
# if present. Off by default so the project builds standalone.
if(BORATOEQ_BUILD_CLAP)
    set(BORATOEQ_CLAP_EXTENSIONS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/clap-juce-extensions")
    if(NOT EXISTS "${BORATOEQ_CLAP_EXTENSIONS_DIR}/CMakeLists.txt")
        set(BORATOEQ_CLAP_EXTENSIONS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../borato-224/libs/clap-juce-extensions")
    endif()
    if(EXISTS "${BORATOEQ_CLAP_EXTENSIONS_DIR}/CMakeLists.txt")
        add_subdirectory("${BORATOEQ_CLAP_EXTENSIONS_DIR}" "${CMAKE_BINARY_DIR}/clap-juce-extensions" EXCLUDE_FROM_ALL)
    else()
        message(FATAL_ERROR "clap-juce-extensions not found. Disable BORATOEQ_BUILD_CLAP or provide the submodule.")
    endif()
endif()

# JUCE 8's Windows resource helper may be emitted as the literal command
# "juce::juceaide" by the VS generator in this environment. Redefine the helper
# locally so the generated project uses the resolved executable path.
if(WIN32)
    function(_juce_add_resources_rc source_target dest_target)
        if(NOT TARGET ${source_target}_rc_lib)
            get_target_property(juce_library_code ${source_target} JUCE_GENERATED_SOURCES_DIRECTORY)
            get_target_property(input_info_file ${source_target} JUCE_INFO_FILE)
            get_target_property(generated_icon ${source_target} JUCE_ICON_FILE)

            set(dependency)
            if(generated_icon)
                set(dependency DEPENDS "${generated_icon}")
            endif()

            set(resource_rc_file "${juce_library_code}/${source_target}_resources.rc")

            add_custom_command(OUTPUT "${resource_rc_file}"
                COMMAND "$<TARGET_FILE:juce::juceaide>" rcfile "${input_info_file}" "${resource_rc_file}"
                ${dependency}
                VERBATIM)

            add_library(${source_target}_rc_lib OBJECT "${resource_rc_file}")
        endif()

        target_link_libraries(${dest_target} PRIVATE ${source_target}_rc_lib)
    endfunction()
endif()

juce_add_plugin(BoratoEq
    COMPANY_NAME "Borato Company"
    BUNDLE_ID com.boratocompany.boratoeq
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD ${BORATOEQ_COPY_PLUGIN_AFTER_BUILD}
    PLUGIN_MANUFACTURER_CODE Bora
    PLUGIN_CODE BqEq
    FORMATS Standalone AU VST3
    PRODUCT_NAME "BORATO EQ"
)

juce_generate_juce_header(BoratoEq)

juce_add_binary_data(BoratoEqAssets
    HEADER_NAME BinaryData.h
    NAMESPACE BinaryData
    SOURCES
        assets/iron_glass_redesign.svg
)

target_sources(BoratoEq
    PRIVATE
        Source/PresetManager.cpp
        Source/PresetManager.h
        Source/Settings.cpp
        Source/Settings.h
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginParameters.cpp
        Source/PluginParameters.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/dsp/ParallelEq.cpp
        Source/dsp/ParallelEq.h
        Source/dsp/ColourStage.cpp
        Source/dsp/ColourStage.h
        Source/ui/BoratoEqLookAndFeel.cpp
        Source/ui/BoratoEqLookAndFeel.h
        Source/ui/HeaderBarComponent.cpp
        Source/ui/HeaderBarComponent.h
        Source/ui/FilterPanelComponent.cpp
        Source/ui/FilterPanelComponent.h
        Source/ui/EqBandComponent.cpp
        Source/ui/EqBandComponent.h
        Source/ui/RightPanelComponent.cpp
        Source/ui/RightPanelComponent.h
)

target_include_directories(BoratoEq
    PRIVATE
        Source
)

target_link_libraries(BoratoEq
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        BoratoEqAssets
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

target_compile_definitions(BoratoEq
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

if(MSVC)
    target_compile_options(BoratoEq PRIVATE /W4 /permissive- /wd4244 /wd4267)
else()
    target_compile_options(BoratoEq PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BORATOEQ_BUILD_CLAP)
    clap_juce_extensions_plugin(TARGET BoratoEq
        CLAP_ID "com.boratocompany.boratoeq"
        CLAP_FEATURES audio-effect equalizer stereo)
endif()
