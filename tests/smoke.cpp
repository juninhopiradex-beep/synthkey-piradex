// Teste de fumo do processador (sem interface): carrega TODOS os presets com os packs instalados,
// toca uma nota em cada um e verifica que há som, sem NaN/Inf e sem rebentar.
//   SK_TEST_PW=<senha master ou beta> ./synthkey_smoke
#include "../Source/PluginProcessor.h"
#include <cmath>
#include <cstdio>

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    auto* pw = std::getenv ("SK_TEST_PW");
    if (pw)
    {
        auto m = AccessManager::matchPassword (pw);
        std::printf ("login: %s\n", AccessManager::get().applyLogin (m, false) == AccessManager::LoginResult::Ok ? "ok" : "FALHOU");
    }
    auto& hub = PackHub::get(); hub.rescan();
    for (auto& e : hub.entries()) if (e.installed) hub.ensureLoaded (e.id);
    for (int i = 0; i < 600; ++i) { bool any = false; for (auto& e : hub.entries()) any |= hub.isLoading (e.id); if (! any) break; juce::MessageManager::getInstance()->runDispatchLoopUntil (100); }
    int installed = 0; for (auto& e : hub.entries()) installed += e.installed ? 1 : 0;
    std::printf ("packs instalados: %d\n", installed);

    SynthKeyProcessor proc; proc.setPlayConfigDetails (0, 2, 48000.0, 256); proc.prepareToPlay (48000.0, 256);
    juce::MessageManager::getInstance()->runDispatchLoopUntil (200);
    proc.refreshPresetList();
    const int n = (int) proc.presetList().size(); std::printf ("presets: %d\n", n);
    int silent = 0, bad = 0, ok = 0, locked = 0; juce::AudioBuffer<float> buf (2, 256); double worst = 1e9, loud = -1e9;
    for (int i = 0; i < n; ++i)
    {
        proc.loadPresetIndex (i); juce::MessageManager::getInstance()->runDispatchLoopUntil (60);
        const auto& p = proc.presetList()[(size_t) i];
        if (proc.presetIsLocked (p)) ++locked;
        float peak = 0, tail = 0; bool nan = false; juce::MidiBuffer mb; const bool kit = p.name.rfind ("Kit", 0) == 0; const int n1 = kit ? 36 : 48, n2 = kit ? 38 : 55;   // kits: pads a partir da nota 36
        for (int b = 0; b < 140; ++b)
        {
            mb.clear();
            if (b == 0) { mb.addEvent (juce::MidiMessage::noteOn (1, n1, (juce::uint8) 100), 0); mb.addEvent (juce::MidiMessage::noteOn (1, n2, (juce::uint8) 100), 0); }
            if (b == 80) { mb.addEvent (juce::MidiMessage::noteOff (1, n1), 0); mb.addEvent (juce::MidiMessage::noteOff (1, n2), 0); }
            buf.clear(); proc.processBlock (buf, mb);
            for (int c = 0; c < 2; ++c) for (int s = 0; s < 256; ++s) { const float v = buf.getSample (c, s); if (! std::isfinite (v)) nan = true; if (b < 80) peak = std::max (peak, std::abs (v)); }
        }
        const double db = peak > 0 ? 20.0 * std::log10 (peak) : -200.0;
        worst = std::min (worst, db); loud = std::max (loud, db);
        if (nan) { ++bad; std::printf ("NaN/Inf: %s\n", p.name.c_str()); }
        else if (db < -50.0) { ++silent; std::printf ("MUDO (%.1f dB): [%s] %s\n", db, p.pack.c_str(), p.name.c_str()); }
        else ++ok;
        if (db > 0.5) std::printf ("PICO ALTO (%.1f dB): [%s] %s\n", db, p.pack.c_str(), p.name.c_str());
    }
    std::printf ("resultado: %d ok, %d mudos, %d com NaN, %d bloqueados (pre-visualizacao). pico minimo %.1f dB, maximo %.1f dB\n", ok, silent, bad, locked, worst, loud);
    if (std::getenv ("SK_SIM"))   // cliente sem chave: pack bloqueado => 15 s de pré-visualização e depois silêncio
    {
        AccessManager::get().setSimulateCustomer (true);
        int idx = -1; for (int i = 0; i < n; ++i) if (proc.presetList()[(size_t) i].pack == "trap") { idx = i; break; }
        if (idx >= 0)
        {
            proc.loadPresetIndex (idx); juce::MessageManager::getInstance()->runDispatchLoopUntil (100);
            std::printf ("preset '%s' bloqueado: %s\n", proc.presetList()[(size_t) idx].name.c_str(), proc.presetIsLocked (proc.presetList()[(size_t) idx]) ? "sim" : "NAO");
            float early = 0, late = 0; juce::MidiBuffer mb; const int total = 48000 * 20 / 256;
            for (int b = 0; b < total; ++b)
            {
                mb.clear(); if (b % 150 == 0) { mb.addEvent (juce::MidiMessage::noteOn (1, 48, (juce::uint8) 100), 0); } if (b % 150 == 100) mb.addEvent (juce::MidiMessage::noteOff (1, 48), 0);
                buf.clear(); proc.processBlock (buf, mb); float pk = 0; for (int s = 0; s < 256; ++s) pk = std::max (pk, std::abs (buf.getSample (0, s)));
                if (b < 48000 * 8 / 256) early = std::max (early, pk); if (b > 48000 * 17 / 256) late = std::max (late, pk);
                if (b % 100 == 0) juce::MessageManager::getInstance()->runDispatchLoopUntil (5);
            }
            std::printf ("pre-visualizacao: primeiros 8 s = %.1f dB, depois dos 17 s = %.1f dB, expirada=%d\n", 20.0 * std::log10 (std::max (early, 1e-10f)), 20.0 * std::log10 (std::max (late, 1e-10f)), (int) proc.previewExpired.load());
        }
    }
    proc.releaseResources();
    return bad == 0 ? 0 : 1;
}
