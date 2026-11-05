// =============================================================================
//  SYNTHKEY by Piradex — MODO TOCAR (simples): escolher um som e tocar, sem páginas de síntese
//    cartão do preset (nome, DNA em uma linha, estrelas, favorito) · ◀ ▶ dentro de uma categoria · categorias em botões grandes
//    variação rápida (escuro, brilhante, largo…) · 8 macros grandes · volume. Os botões de macro e o volume são os do MainView.
// =============================================================================
#pragma once
#include "Browser.h"

namespace ui
{
class SimplePage : public juce::Component
{
public:
    struct Hooks { std::function<void()> openBrowser; std::function<void (const std::string&)> variation; };
    SimplePage (SynthKeyProcessor& p, Hooks h) : proc (p), hooks (std::move (h))
    {
        static const char* cats[kNumChips] = { "*fav", "*foryou", "Teclas", "Pad", "Baixo", "Lead", "Pluck", "Sintetizador", "Cordas", "Vozes", "Arp", "Bateria", "FX", "Cinem\xC3\xA1tico" };
        static const char* label[kNumChips] = { "\xE2\x99\xA5 FAVORITOS", "\xE2\x9C\xA6 PARA TI", "TECLAS", "PADS", "BAIXOS", "LEADS", "PLUCKS", "SINTETIZADORES", "CORDAS", "VOZES", "ARPEJOS", "BATERIA", "FX", "CINEM\xC3\x81TICO" };
        for (int i = 0; i < kNumChips; ++i) { chipKey[i] = cats[i]; chip[i].setButtonText (TX (label[i])); chip[i].setClickingTogglesState (false); addAndMakeVisible (chip[i]); chip[i].onClick = [this, i] { pickChip (i); }; }
        static const char* vl[kNumVar] = { "ESCURO", "BRILHANTE", "LARGO", "CURTO", "QUENTE", "MOVIMENTO" };
        static const char* vw[kNumVar] = { "escuro", "brilhante", "largo", "curto", "quente", "movimento" };
        for (int i = 0; i < kNumVar; ++i) { var[i].setButtonText (TX (vl[i])); varWord[i] = vw[i]; addAndMakeVisible (var[i]); var[i].onClick = [this, i] { if (hooks.variation) hooks.variation (varWord[i]); }; }
        prev.setButtonText (TX ("\xE2\x97\x80")); next.setButtonText (TX ("\xE2\x96\xB6")); lib.setButtonText (TX ("BIBLIOTECA"));
        for (auto* b : { &prev, &next, &lib }) addAndMakeVisible (b);
        prev.onClick = [this] { step (-1); }; next.onClick = [this] { step (1); }; lib.onClick = [this] { if (hooks.openBrowser) hooks.openBrowser(); };
        prev.setTooltip (TX ("Preset anterior (dentro da categoria escolhida)")); next.setTooltip (TX ("Preset seguinte (dentro da categoria escolhida)"));
        for (auto& v : var) v.setTooltip (TX ("Cria uma variação calibrada do som atual nessa direção (desfaz com Ctrl/Cmd+Z)"));
    }
    void tick()
    {
        const auto key = proc.currentPresetName() + juce::String (proc.currentPresetIndex()) + juce::String (Ratings::get().count());
        if (key != lastKey) { lastKey = key; const auto live = proc.capturePreset (proc.currentPresetName()); const auto d = sk::dna::of (live); dnaLine = juce::String (); 
            for (size_t i = 0; i < d.engines.size(); ++i) dnaLine << (i ? " + " : "") << TX (d.engines[i].c_str());
            dnaLine << "  \xC2\xB7  " << (d.cpuClass == 1 ? "CPU leve" : (d.cpuClass == 2 ? "CPU m\xC3\xA9" "dio" : "CPU pesado"));
            if (d.arp) dnaLine << "  \xC2\xB7  arpejador"; repaint(); }
    }
    juce::Rectangle<int> macroArea() const { return { 24, 408, 1132, 222 }; }
    void paint (juce::Graphics& g) override
    {
        theme::drawCard (g, juce::Rectangle<float> (12.0f, 4.0f, 1156.0f, 130.0f), 14.0f);
        g.setColour (theme::text()); g.setFont (theme::font (30.0f, true)); g.drawFittedText (proc.currentPresetName(), 36, 10, 700, 40, juce::Justification::centredLeft, 1);
        const auto& L = proc.presetList(); const int idx = proc.currentPresetIndex(); const sk::Preset* cp = (idx >= 0 && idx < (int) L.size() && L[(size_t) idx].name == proc.currentPresetName().toStdString()) ? &L[(size_t) idx] : nullptr;
        juce::String sub; if (cp) { PackEntry pe; sub = (PackHub::get().entry (cp->pack, pe) && ! pe.name.empty() ? TX (pe.name.c_str()) : juce::String (cp->pack)) + "  \xC2\xB7  " + TX (cp->category.c_str()); } else sub = TX ("som atual (n\xC3\xA3o guardado)");
        g.setColour (theme::dim()); g.setFont (theme::font (14.0f)); g.drawText (sub, 38, 50, 700, 20, juce::Justification::centredLeft);
        g.setColour (theme::accent2()); g.setFont (theme::font (12.5f)); g.drawText (dnaLine, 38, 70, 700, 18, juce::Justification::centredLeft);
        const int r = cp ? Ratings::get().of (cp->pack, cp->name) : 0;
        for (int i = 0; i < 5; ++i) { g.setColour (i < r ? theme::warn() : juce::Colour (0x55FFFFFF)); g.setFont (theme::font (26.0f)); g.drawText (i < r ? TX ("\xE2\x98\x85") : TX ("\xE2\x98\x86"), 36 + i * 36, 94, 36, 34, juce::Justification::centred); }
        const bool fav = cp && Favourites::get().has (cp->pack, cp->name);
        g.setColour (fav ? theme::bad() : juce::Colour (0x55FFFFFF)); g.setFont (theme::font (26.0f)); g.drawText (fav ? TX ("\xE2\x99\xA5") : TX ("\xE2\x99\xA1"), 228, 94, 40, 34, juce::Justification::centred);
        g.setColour (theme::dim()); g.setFont (theme::font (11.0f, true)); g.drawText (TX ("ESCOLHE UM TIPO DE SOM"), 24, 142, 300, 16, juce::Justification::centredLeft);
        g.drawText (TX ("VARIA\xC3\x87\xC3\x83O R\xC3\x81PIDA DO SOM ATUAL"), 24, 258, 400, 16, juce::Justification::centredLeft);
        g.drawText (TX ("MACROS  \xC2\xB7  VOLUME"), 24, 326, 400, 16, juce::Justification::centredLeft);
        if (activeChip >= 0) { g.setColour (theme::accent()); g.setFont (theme::font (11.5f, true)); g.drawText (juce::String (list.size()) + TX (" sons nesta sele\xC3\xA7\xC3\xA3o"), 700, 142, 456, 16, juce::Justification::centredRight); }
    }
    void resized() override
    {
        prev.setBounds (772, 16, 108, 106); next.setBounds (888, 16, 108, 106); lib.setBounds (1008, 16, 140, 106);
        for (int i = 0; i < kNumChips; ++i) chip[i].setBounds (24 + (i % 7) * 162, 162 + (i / 7) * 48, 154, 42);
        for (int i = 0; i < kNumVar; ++i) var[i].setBounds (24 + i * 162, 278, 154, 42);
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        const auto& L = proc.presetList(); const int idx = proc.currentPresetIndex();
        if (idx < 0 || idx >= (int) L.size() || L[(size_t) idx].name != proc.currentPresetName().toStdString()) return; const auto& p = L[(size_t) idx];
        if (e.y >= 94 && e.y < 128 && e.x >= 36 && e.x < 216) { const int s = (e.x - 36) / 36 + 1; Ratings::get().set (p.pack, p.name, Ratings::get().of (p.pack, p.name) == s ? 0 : s); lastKey = {}; tick(); }
        else if (e.y >= 94 && e.y < 128 && e.x >= 228 && e.x < 268) { Favourites::get().toggle (p.pack, p.name); repaint(); }
    }
    void visibilityChanged() override { if (isVisible()) { lastKey = {}; tick(); } }

private:
    static constexpr int kNumChips = 14, kNumVar = 6;
    void pickChip (int i)
    {
        const auto& L = proc.presetList(); list.clear(); const std::string key = chipKey[i];
        if (key == "*fav" || key == "*foryou")
        {
            std::vector<std::pair<int, float>> liked;
            for (size_t k = 0; k < L.size(); ++k) { float w = 0; const int r = Ratings::get().of (L[k].pack, L[k].name); if (r >= 3) w += (float) (r - 2); if (Favourites::get().has (L[k].pack, L[k].name)) w += 1.5f; if (w > 0) liked.push_back ({ (int) k, w }); }
            if (key == "*fav") { for (auto& l : liked) if (Favourites::get().has (L[(size_t) l.first].pack, L[(size_t) l.first].name)) list.add (l.first); }
            else { if (liked.empty()) { say (TX ("Marca favoritos (\xE2\x99\xA5) ou d\xC3\xA1 3+ estrelas para receber recomenda\xC3\xA7\xC3\xB5" "es."), false); return; } for (auto& h : sk::search::recommend (liked, L, 60)) list.add (h.index); }
            if (list.isEmpty()) { say (TX ("Ainda n\xC3\xA3o tens favoritos \xE2\x80\x94 clica no cora\xC3\xA7\xC3\xA3o do cart\xC3\xA3o."), false); return; }
        }
        else for (size_t k = 0; k < L.size(); ++k) if (L[k].category == key && L[k].module.empty() && ! proc.presetIsLocked (L[k])) list.add ((int) k);
        if (list.isEmpty()) { say (TX ("Sem sons nesta categoria."), false); return; }
        activeChip = i; for (int k = 0; k < kNumChips; ++k) chip[k].setToggleState (k == i, juce::dontSendNotification);
        pos = juce::Random::getSystemRandom().nextInt (list.size()); proc.loadPresetIndex (list[pos]); repaint();
    }
    void step (int dir)
    {
        const int cur = proc.currentPresetIndex();
        if (activeChip >= 0 && ! list.isEmpty())
        {
            int at = list.indexOf (cur); if (at < 0) at = dir > 0 ? -1 : 0;
            pos = (at + dir + list.size()) % list.size(); proc.loadPresetIndex (list[pos]); repaint(); return;
        }
        proc.stepPreset (dir); repaint();
    }
    void say (const juce::String& m, bool e) { ui::say (m, e); }

    SynthKeyProcessor& proc; Hooks hooks;
    juce::TextButton chip[kNumChips], var[kNumVar], prev, next, lib; std::string chipKey[kNumChips], varWord[kNumVar];
    juce::Array<int> list; int activeChip = -1, pos = 0; juce::String lastKey, dnaLine;
};
} // namespace ui
