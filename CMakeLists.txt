cmake_minimum_required(VERSION 3.22)

# macOS: compatível com Macs antigos (10.13 High Sierra, Intel) e novos (Apple Silicon) — binário universal
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "macOS mínimo")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Arquiteturas")
endif()
# Windows: runtime do Visual C++ embutido (não precisa do "VC++ Redistributable")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(SYNTHKEY VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE: cópia local (-DJUCE_PATH=...) ou versão fixa descarregada
if(DEFINED JUCE_PATH)
    add_subdirectory(${JUCE_PATH} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.15 GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Source/Secrets.h")
    message(FATAL_ERROR "Falta Source/Secrets.h. Corra:  python3 tools/gen_secrets.py   (ou, no CI, defina o segredo SYNTHKEY_SECRETS_B64)")
endif()

if(APPLE)
    set(SK_FORMATS VST3 AU Standalone)
elseif(WIN32)
    set(SK_FORMATS VST3 Standalone)
else()
    set(SK_FORMATS VST3 Standalone)   # Linux: só desenvolvimento/testes
endif()

juce_add_binary_data(SynthKeyAssets SOURCES
    Resources/skins/liquid_chrome.jpg
    Resources/covers/nucleo.jpg Resources/covers/trap.jpg Resources/covers/rnb.jpg Resources/covers/afrobeat.jpg
    Resources/covers/amapiano.jpg Resources/covers/drill.jpg Resources/covers/kizomba.jpg Resources/covers/kuduro.jpg)
set_target_properties(SynthKeyAssets PROPERTIES POSITION_INDEPENDENT_CODE ON)

juce_add_plugin(SynthKey
    COMPANY_NAME "Piradex"
    BUNDLE_ID "com.piradex.synthkey"
    PLUGIN_MANUFACTURER_CODE Prdx
    PLUGIN_CODE PxSK
    FORMATS ${SK_FORMATS}
    PRODUCT_NAME "SYNTHKEY by Piradex"
    DESCRIPTION "SYNTHKEY - sintetizador com packs de estilos"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    MICROPHONE_PERMISSION_ENABLED FALSE)

target_sources(SynthKey PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Access.cpp
    Source/PackHub.cpp)
target_include_directories(SynthKey PRIVATE Source)

target_compile_definitions(SynthKey PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    SK_VERSION_STRING="${PROJECT_VERSION}")

if(MSVC)
    target_compile_options(SynthKey PRIVATE /utf-8)
endif()

target_link_libraries(SynthKey
    PRIVATE
        SynthKeyAssets
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_cryptography
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags)
option(SK_ENABLE_LTO "Otimização em tempo de ligação (CI)" ON)
if(SK_ENABLE_LTO)
    target_link_libraries(SynthKey PUBLIC juce::juce_recommended_lto_flags)
endif()

# ---- testes e ferramentas do núcleo (C++ puro, sem JUCE) -----------------------------------
option(SK_BUILD_TOOLS "Compilar testes e packgen" OFF)
if(SK_BUILD_TOOLS)
    foreach(t test_core test_packio test_presets)
        add_executable(${t} tests/${t}.cpp)
        target_include_directories(${t} PRIVATE Source)
    endforeach()
    add_executable(packgen tools/packgen.cpp)
    target_include_directories(packgen PRIVATE Source)
endif()

# ---- teste de fumo do processador (JUCE, sem interface) ------------------------------------
option(SK_BUILD_SMOKE "Compilar o teste de fumo do processador" OFF)
if(SK_BUILD_SMOKE)
    juce_add_console_app(SynthKeySmoke PRODUCT_NAME "synthkey_smoke")
    target_sources(SynthKeySmoke PRIVATE tests/smoke.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/Access.cpp Source/PackHub.cpp)
    target_include_directories(SynthKeySmoke PRIVATE Source)
    target_compile_definitions(SynthKeySmoke PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1 JUCE_STANDALONE_APPLICATION=1 SK_VERSION_STRING="${PROJECT_VERSION}")
    target_link_libraries(SynthKeySmoke PRIVATE SynthKeyAssets juce::juce_audio_utils juce::juce_audio_processors juce::juce_cryptography juce::juce_dsp juce::juce_recommended_config_flags)
endif()
