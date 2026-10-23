// =============================================================================
//  SYNTHKEY by Piradex — gestor de packs (instalação, leitura, carregamento assíncrono)
//  Partilhado por todas as instâncias do plugin no mesmo processo: os samples carregam uma só vez.
// =============================================================================
#pragma once
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include "Core/PackIO.h"
#include "Core/PresetBuilder.h"

struct PackEntry
{
    std::string id, name, style, desc, accent, version;
    bool core = false, installed = false;
    juce::File file;
    std::vector<sk::packio::PackInfo::Inst> insts;      // lista de instrumentos (vem das definições, mesmo sem instalar)
    int presetCount = 0;
    juce::Image cover;
};

class PackHub : public juce::ChangeBroadcaster
{
public:
    static PackHub& get();

    void rescan();                                          // lê as pastas de packs (rápido)
    std::vector<PackEntry> entries() const;
    bool entry (const std::string& id, PackEntry& out) const;

    void ensureLoaded (const std::string& packId);          // carrega a biblioteca em segundo plano
    std::shared_ptr<const sk::Library> library (const std::string& packId) const;
    bool isLoading (const std::string& packId) const;
    bool isLoadFailed (const std::string& packId) const;

    juce::String install (const juce::File& src);           // copia um .bfpack para a pasta do utilizador; devolve erro ou ""
    juce::String uninstall (const std::string& packId);

    std::vector<sk::Preset> factoryPresets() const;         // Núcleo + packs instalados (já calibrados)
    std::vector<sk::Preset> userPresets() const;
    static juce::File userPresetFolder();
    static juce::Image coverFor (const std::string& packId);
    static std::string packOfInstrument (const std::string& instId) { const auto d = instId.find ('.'); return d == std::string::npos ? std::string() : instId.substr (0, d); }

private:
    PackHub();
    ~PackHub() override;
    void loadJob (std::string id);
    mutable std::mutex mtx;
    std::vector<PackEntry> list;
    std::map<std::string, std::vector<sk::Preset>> presetsOf;
    std::map<std::string, std::shared_ptr<const sk::Library>> libs;
    std::set<std::string> loading, failed;
    std::vector<sk::PackDef> defs;
    std::unique_ptr<juce::ThreadPool> pool;
};
