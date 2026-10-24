// Teste de fumo do processador (sem interface): carrega TODOS os presets com os packs instalados,
// toca uma nota em cada um e verifica que há som, sem NaN/Inf e sem rebentar.
//   SK_TEST_PW=<senha master ou beta> ./synthkey_smoke
#include "../Source/PluginProcessor.h"
#include "../Source/UserAudio.h"
#include "../Source/GrooveStore.h"
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
    // ---- grooves internos + buffer do osciloscópio
    int grooveFail = 0;
    {
        proc.loadPresetIndex (0); juce::MessageManager::getInstance()->runDispatchLoopUntil (60);
        const sk::Groove* bass = nullptr; for (auto& g : sk::allGrooves()) if (g.kind == "Baixo") { bass = &g; break; }
        proc.groove.start (*bass, 0, 0); float pk = 0; juce::MidiBuffer mb; const int w0 = proc.scopeWrite.load(); float scopeMax = 0;
        for (int b = 0; b < 400; ++b) { mb.clear(); buf.clear(); proc.processBlock (buf, mb); for (int s = 0; s < 256; ++s) pk = std::max (pk, std::abs (buf.getSample (0, s))); }
        for (int i = 0; i < 8192; ++i) scopeMax = std::max (scopeMax, std::abs (proc.scopeBuf[i]));
        proc.groove.stop(); mb.clear(); buf.clear(); proc.processBlock (buf, mb);
        std::printf ("groove baixo: pico %.1f dB, scope max %.3f, scopeWrite %d -> %d\n", 20.0 * std::log10 (std::max (pk, 1e-10f)), scopeMax, w0, proc.scopeWrite.load());
        if (pk < 0.01f || scopeMax < 0.01f || proc.scopeWrite.load() == w0) { ++grooveFail; std::printf ("FALHOU: groove/osciloscopio sem sinal\n"); }
        // bateria com um kit
        int kit = -1; for (int i = 0; i < n; ++i) if (proc.presetList()[(size_t) i].name.rfind ("Kit", 0) == 0 && ! proc.presetIsLocked (proc.presetList()[(size_t) i])) { kit = i; break; }
        const sk::Groove* dr = nullptr; for (auto& g : sk::allGrooves()) if (g.kind == "Bateria") { dr = &g; break; }
        if (kit >= 0 && dr)
        {
            proc.loadPresetIndex (kit); juce::MessageManager::getInstance()->runDispatchLoopUntil (60); proc.groove.start (*dr, 0, 0); pk = 0;
            for (int b = 0; b < 400; ++b) { mb.clear(); buf.clear(); proc.processBlock (buf, mb); for (int s = 0; s < 256; ++s) pk = std::max (pk, std::abs (buf.getSample (0, s))); }
            proc.groove.stop(); std::printf ("groove bateria com '%s': pico %.1f dB\n", proc.presetList()[(size_t) kit].name.c_str(), 20.0 * std::log10 (std::max (pk, 1e-10f)));
            if (pk < 0.01f) { ++grooveFail; std::printf ("FALHOU: groove de bateria mudo\n"); }
        }
    }
    // ---- camada do groove: bateria num motor à parte enquanto tocas outro som
    {
        proc.setGrooveSoundKey ("auto"); proc.loadPresetIndex (0); juce::MessageManager::getInstance()->runDispatchLoopUntil (60);
        const sk::Groove* dr = nullptr; for (auto& g : sk::allGrooves()) if (g.kind == "Bateria") { dr = &g; break; }
        if (dr)
        {
            juce::MidiBuffer mb; proc.prepareGrooveLayer (*dr);
            if (! proc.grooveLayerOn() || proc.grooveLayerName().isEmpty()) { ++grooveFail; std::printf ("FALHOU: camada do groove nao ativou\n"); }
            proc.groove.start (*dr, 0, 0); float pkG = 0, pkBoth = 0;
            for (int b = 0; b < 300; ++b) { mb.clear(); buf.clear(); proc.processBlock (buf, mb); for (int s = 0; s < 256; ++s) pkG = std::max (pkG, std::abs (buf.getSample (0, s))); }
            for (int b = 0; b < 300; ++b) { mb.clear(); if (b == 0) mb.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0); buf.clear(); proc.processBlock (buf, mb); for (int s = 0; s < 256; ++s) pkBoth = std::max (pkBoth, std::abs (buf.getSample (0, s))); }
            proc.groove.stop(); mb.clear(); mb.addEvent (juce::MidiMessage::noteOff (1, 60), 0); buf.clear(); proc.processBlock (buf, mb);
            std::printf ("camada groove '%s': so groove %.1f dB, groove+tu %.1f dB, voz=%d\n", proc.grooveLayerName().toRawUTF8(), 20.0 * std::log10 (std::max (pkG, 1e-10f)), 20.0 * std::log10 (std::max (pkBoth, 1e-10f)), proc.voiceCount.load());
            if (pkG < 0.01f) { ++grooveFail; std::printf ("FALHOU: camada do groove muda\n"); }
            if (pkBoth < pkG * 0.5f || pkBoth > 1.0001f) { ++grooveFail; std::printf ("FALHOU: groove + notas tocadas\n"); }
            // modo "igual ao teu som": a camada desliga
            proc.setGrooveSoundKey ("same"); proc.prepareGrooveLayer (*dr);
            if (proc.grooveLayerOn()) { ++grooveFail; std::printf ("FALHOU: camada devia estar desligada\n"); }
            proc.setGrooveSoundKey ("auto");
        }
    }
    // ---- grooves do utilizador: importar MIDI, guardar, recarregar, editar ao vivo
    {
        auto tmp = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("sk_midi_test"); tmp.createDirectory();
        auto writeMid = [&] (const juce::String& nm, int ch, std::vector<std::vector<int>> notes /* {tick, note, len, vel} */, int bpm)
        {
            juce::MidiMessageSequence seq; seq.addEvent (juce::MidiMessage::tempoMetaEvent (60000000 / bpm), 0.0);
            for (auto& n : notes) { seq.addEvent (juce::MidiMessage::noteOn (ch, n[1], (juce::uint8) n[3]), n[0]); seq.addEvent (juce::MidiMessage::noteOff (ch, n[1]), n[0] + n[2]); }
            seq.updateMatchedPairs(); juce::MidiFile mf; mf.setTicksPerQuarterNote (480); mf.addTrack (seq);
            auto f = tmp.getChildFile (nm + ".mid"); f.deleteFile(); juce::FileOutputStream os (f); mf.writeTo (os); return f;
        };
        auto check = [&] (bool ok, const char* msg) { if (! ok) { ++grooveFail; std::printf ("FALHOU: %s\n", msg); } };
        sk::Groove g; juce::String err;
        // bateria GM no canal 10: bombo, caixa, hat de oitavo em oitavo, 2 compassos; nota 40 (caixa eléctrica) e 49 (prato) são mapeadas
        std::vector<std::vector<int>> dr; for (int s = 0; s < 32; s += 8) dr.push_back ({ s * 120, 36, 60, 110 }); for (int s = 4; s < 32; s += 8) dr.push_back ({ s * 120, 40, 60, 100 });
        for (int s = 0; s < 32; s += 2) dr.push_back ({ s * 120, 42, 60, 80 }); dr.push_back ({ 0, 49, 60, 90 });
        check (sk::GrooveStore::importMidi (writeMid ("sk_dr", 10, dr, 96), "kuduro", g, err), "importar bateria");
        check (g.kind == "Bateria" && g.bars == 2 && g.bpm == 96 && g.style == "kuduro", "bateria: tipo/compassos/bpm/estilo");
        bool lanesOk = true; for (auto& n : g.notes) if (n.note != 36 && n.note != 38 && n.note != 42 && n.note != 46) lanesOk = false; check (lanesOk, "bateria: pistas mapeadas");
        // baixo: notas graves, uma de cada vez
        std::vector<std::vector<int>> bs; for (int s = 0; s < 16; s += 4) bs.push_back ({ s * 120, 36 + (s % 8), 360, 100 });
        check (sk::GrooveStore::importMidi (writeMid ("sk_bs", 1, bs, 100), "", g, err) && g.kind == "Baixo" && g.bars == 1, "importar baixo");
        // acordes: tríades
        std::vector<std::vector<int>> ch; for (int s = 0; s < 32; s += 8) for (int k : { 60, 64, 67 }) ch.push_back ({ s * 120, k, 840, 90 });
        check (sk::GrooveStore::importMidi (writeMid ("sk_ch", 1, ch, 90), "", g, err) && g.kind == "Acordes" && g.bars == 2, "importar acordes");
        // melodia: notas agudas
        std::vector<std::vector<int>> me; for (int s = 0; s < 16; s += 2) me.push_back ({ s * 120, 72 + (s % 5), 120, 90 });
        check (sk::GrooveStore::importMidi (writeMid ("sk_me", 1, me, 110), "", g, err) && g.kind == "Melodia", "importar melodia");
        check (! sk::GrooveStore::importMidi (tmp.getChildFile ("nao_existe.mid"), "", g, err), "ficheiro inexistente deve falhar");
        // guardar / recarregar / apagar
        auto& store = sk::GrooveStore::get(); const size_t before = store.userGrooves().size();
        g.name = "sk_teste_groove"; auto* saved = store.save (g);
        check (saved && store.userGrooves().size() == before + 1, "guardar groove do utilizador");
        store.reload(); bool found = false; for (auto& u : store.userGrooves()) if (u.name == "sk_teste_groove") { found = u.notes.size() == g.notes.size() && u.kind == g.kind && u.user; }
        check (found, "recarregar groove do utilizador (notas iguais)");
        // edição ao vivo: o player continua a tocar e aceita o novo conteúdo
        const sk::Groove* live = nullptr; for (auto& u : store.userGrooves()) if (u.name == "sk_teste_groove") live = &u;
        if (live)
        {
            sk::Groove edit = *live; proc.groove.start (edit, 0, 0); juce::MidiBuffer mb; for (int b = 0; b < 40; ++b) { mb.clear(); buf.clear(); proc.processBlock (buf, mb); }
            const double p0 = proc.groove.stepPosition(); edit.notes.push_back ({ 8, 80, 2, 100 }); proc.groove.update (edit, 0);
            for (int b = 0; b < 4; ++b) { mb.clear(); buf.clear(); proc.processBlock (buf, mb); }
            check (proc.groove.isRunning() && proc.groove.stepPosition() >= p0, "edicao ao vivo mantem a posicao"); proc.groove.stop(); mb.clear(); buf.clear(); proc.processBlock (buf, mb);
        }
        for (auto& u : std::vector<sk::Groove> (store.userGrooves().begin(), store.userGrooves().end()))
            if (u.name == "sk_teste_groove") { for (auto it = store.userGrooves().begin(); it != store.userGrooves().end(); ++it) if (it->name == u.name) { store.remove (&*it); break; } }
        check (store.userGrooves().size() == before, "apagar groove do utilizador");
        tmp.deleteRecursively();
        std::printf ("grooves do utilizador: importar/guardar/editar ok=%d\n", grooveFail == 0);
    }
    // ---- importação do utilizador: sample tonal (altura detetada), percussivo, wavetable
    int importFail = 0;
    {
        auto tmp = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("sk_import_test"); tmp.createDirectory();
        auto writeWav = [&] (const juce::String& name, const std::vector<float>& d, double sr) -> juce::File
        {
            auto f = tmp.getChildFile (name); f.deleteFile();
            juce::WavAudioFormat wav; std::unique_ptr<juce::FileOutputStream> os (f.createOutputStream());
            std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (os.get(), sr, 1, 24, {}, 0)); if (w) { os.release(); juce::AudioBuffer<float> b (1, (int) d.size()); std::copy (d.begin(), d.end(), b.getWritePointer (0)); w->writeFromAudioSampleBuffer (b, 0, (int) d.size()); }
            return f;
        };
        std::vector<float> sine (48000), noise (12000); unsigned rs = 12345;
        for (size_t i = 0; i < sine.size(); ++i) sine[i] = 0.5f * std::sin (6.283185307f * 220.0f * (float) i / 48000.0f) * std::exp (-(float) i / 30000.0f);
        for (size_t i = 0; i < noise.size(); ++i) { rs = rs * 1664525u + 1013904223u; noise[i] = ((float) (rs >> 8) / 8388608.0f - 1.0f) * std::exp (-(float) i / 1500.0f); }
        auto& hub = PackHub::get(); std::string idT, idP;
        auto e1 = hub.importUserSample (writeWav ("sk_teste_tonal.wav", sine, 48000.0), &idT);
        auto e2 = hub.importUserSample (writeWav ("sk_teste_perc.wav", noise, 44100.0), &idP);
        auto lib = hub.userLibrary(); const sk::Instrument* iT = nullptr; const sk::Instrument* iP = nullptr;
        if (lib) for (auto& i : lib->inst) { if (i.id == idT) iT = &i; if (i.id == idP) iP = &i; }
        std::printf ("import: tonal='%s' (%s) percussivo='%s' (%s)\n", idT.c_str(), e1.toRawUTF8(), idP.c_str(), e2.toRawUTF8());
        if (e1.isNotEmpty() || e2.isNotEmpty() || ! iT || ! iP) { ++importFail; std::printf ("FALHOU: importacao de samples\n"); }
        else
        {
            std::printf ("   tonal: raiz %d fixa=%d | percussivo: raiz %d fixa=%d\n", iT->zones[0].root, (int) iT->fixedPitch, iP->zones[0].root, (int) iP->fixedPitch);
            if (iT->zones[0].root != 57 || iT->fixedPitch) { ++importFail; std::printf ("FALHOU: altura do sample tonal (esperado 57 = 220 Hz)\n"); }
            if (! iP->fixedPitch) { ++importFail; std::printf ("FALHOU: o ruido devia ser percussivo\n"); }
            // toca o sample importado no motor A, nota 57 → 220 Hz
            juce::MessageManager::getInstance()->runDispatchLoopUntil (300);
            proc.loadPresetIndex (0); juce::MessageManager::getInstance()->runDispatchLoopUntil (60);
            proc.setReal (sk::P_A_ON, 1.0f); proc.setReal (sk::P_A_TYPE, 2.0f); proc.setReal (sk::P_B_ON, 0.0f); proc.setReal (sk::P_SUB_LEVEL, -60.0f); proc.setSampleId (0, juce::String (idT));
            juce::MessageManager::getInstance()->runDispatchLoopUntil (300);
            float pk = 0; juce::MidiBuffer mb; for (int b = 0; b < 60; ++b) { mb.clear(); if (b == 0) mb.addEvent (juce::MidiMessage::noteOn (1, 57, (juce::uint8) 100), 0); buf.clear(); proc.processBlock (buf, mb); for (int s2 = 0; s2 < 256; ++s2) pk = std::max (pk, std::abs (buf.getSample (0, s2))); }
            std::printf ("   sample importado a tocar: pico %.1f dB\n", 20.0 * std::log10 (std::max (pk, 1e-10f)));
            if (pk < 0.01f) { ++importFail; std::printf ("FALHOU: sample importado mudo\n"); }
        }
        // wavetable (ciclos de 2048) + ida e volta pelo estado do projeto
        std::vector<float> wt (4 * 2048); for (int c = 0; c < 4; ++c) for (int i = 0; i < 2048; ++i) wt[(size_t) (c * 2048 + i)] = std::sin (6.2831853f * (float) (c + 1) * (float) i / 2048.0f);
        auto wf = writeWav ("sk_teste_wt.wav", wt, 48000.0);
        const auto we = proc.importWavetable (wf);
        juce::MemoryBlock st; proc.getStateInformation (st); proc.userWavetable().clear(); proc.setStateInformation (st.getData(), (int) st.getSize());
        std::printf ("wavetable: %s | recarregada do estado: %d\n", we.isEmpty() ? "ok" : we.toRawUTF8(), (int) proc.userWavetable().loaded());
        if (we.isNotEmpty() || ! proc.userWavetable().loaded()) { ++importFail; std::printf ("FALHOU: wavetable do utilizador\n"); }
        hub.removeUserSample (idT); hub.removeUserSample (idP); tmp.deleteRecursively();
    }
    proc.releaseResources();
    return (bad == 0 && grooveFail == 0 && importFail == 0) ? 0 : 1;
}
