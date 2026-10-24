// =============================================================================
//  SYNTHKEY by Piradex — páginas SOM, MATRIZ, EFEITOS e ARP
// =============================================================================
#pragma once
#include "Controls.h"
#include "../Core/MidiFx.h"

namespace ui
{
using namespace sk;

// ------------------------------------------------------------------------------ base das páginas
class Page : public juce::Component
{
public:
    explicit Page (SynthKeyProcessor& p) : proc (p) {}
    Section* addSection (const juce::String& title, juce::Rectangle<int> r)
    {
        auto* s = new Section (title); sections.add (s); addAndMakeVisible (s); s->setBounds (r); s->setMode (mode); return s;
    }
    virtual void setMode (int m) { mode = m; for (auto* s : sections) s->setMode (m); lastSig = 0; }
    virtual void tick()
    {
        int sig = 17; for (auto* s : sections) sig = sig * 31 + s->visibilitySignature();
        if (sig != lastSig) { lastSig = sig; for (auto* s : sections) s->relayout(); }
        const int ms = modMatrixSignature (proc);       // os anéis de modulação dos botões seguem a matriz
        if (ms != lastModSig) { lastModSig = ms; repaintKnobs (*this); }
    }
protected:
    int val (int idx) const { return (int) proc.getReal (idx); }
    Knob* K (Section* s, int idx, const juce::String& label, int level = 1, std::function<bool()> sh = {}, int w = 52, int h = 68)
    {
        const bool bip = kParams[idx].mn < 0 && kParams[idx].mx > 0;
        return s->add (new Knob (proc, idx, label, bip), w, h, level, std::move (sh));
    }
    Combo* C (Section* s, int idx, int w, int level = 1, std::function<bool()> sh = {}, int yoff = 0)
    {
        auto* c = s->add (new Combo (proc, idx), w, 28, level, std::move (sh)); s->setYOffset (c, yoff); return c;
    }
    Toggle* T (Section* s, int idx, const juce::String& label, int w, int level = 1, std::function<bool()> sh = {}, int yoff = 0)
    {
        auto* t = s->add (new Toggle (proc, idx, label), w, 28, level, std::move (sh)); s->setYOffset (t, yoff); return t;
    }
    static void repaintKnobs (juce::Component& c)
    {
        for (auto* ch : c.getChildren()) { if (dynamic_cast<Knob*> (ch)) ch->repaint(); else repaintKnobs (*ch); }
    }
    SynthKeyProcessor& proc; juce::OwnedArray<Section> sections; int mode = 2; int lastSig = 0; int lastModSig = 0;
};

// ----------------------------------------------------------------- seletor de instrumento (sample)
class SampleChooser : public juce::TextButton
{
public:
    SampleChooser (SynthKeyProcessor& p, int engine) : proc (p), E (engine)
    {
        onClick = [this] { showMenu(); };
        setTooltip (TX ("Escolher instrumento (sample)"));
    }
    void paintButton (juce::Graphics& g, bool hover, bool down) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (juce::Colour (0xFF121528)); g.fillRoundedRectangle (r, 6.0f);
        g.setColour (hover || down ? theme::accent() : theme::stroke()); g.drawRoundedRectangle (r, 6.0f, 1.0f);
        juce::String t = proc.sampleDisplayName (E); const auto id = proc.getSampleId (E).toStdString();
        PackEntry pe; juce::Colour tag = theme::dim();
        if (PackHub::get().entry (PackHub::packOfInstrument (id), pe)) { tag = juce::Colour::fromString ("FF" + juce::String (pe.accent).substring (1)); g.setColour (tag); g.fillEllipse (r.getX() + 9, r.getCentreY() - 4, 8, 8); }
        g.setColour (theme::text()); g.setFont (theme::font (13.0f));
        g.drawFittedText ((proc.sampleMissing (E) ? TX ("\xE2\x9A\xA0 ") : juce::String()) + t, 24, 0, getWidth() - 44, getHeight(), juce::Justification::centredLeft, 1);
        g.setColour (theme::dim()); g.fillPath ([&] { juce::Path a; a.addTriangle ((float) getWidth() - 16, 11, (float) getWidth() - 8, 11, (float) getWidth() - 12, 17); return a; }());
    }
private:
    void showMenu()
    {
        juce::PopupMenu menu; ids.clear(); int nextId = 1000;
        auto& acc = AccessManager::get();
        for (auto& e : PackHub::get().entries())
        {
            juce::PopupMenu sub; juce::String lastCat;
            const bool usable = acc.packStatus (e.id, e.core).usable();
            for (auto& in : e.insts)
            {
                if (in.cat != lastCat.toStdString()) { lastCat = TX (in.cat.c_str()); sub.addSectionHeader (lastCat); }
                ids.push_back (in.id);
                sub.addItem (nextId++, (usable ? juce::String() : TX ("\xF0\x9F\x94\x92 ")) + TX (in.name.c_str()), e.installed, proc.getSampleId (E).toStdString() == in.id);
            }
            menu.addSubMenu (TX (e.name.c_str()) + (e.installed ? (usable ? juce::String() : TX ("   \xF0\x9F\x94\x92")) : TX ("   (n\xC3\xA3o instalado)")), sub, e.installed);
        }
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMinimumWidth (getWidth()), [this] (int r)
        {
            if (r >= 1000 && r - 1000 < (int) ids.size()) { proc.setSampleId (E, TX (ids[(size_t) (r - 1000)].c_str())); repaint(); }
        });
    }
    SynthKeyProcessor& proc; int E; std::vector<std::string> ids;
};

// ===================================================================================== SOM
class SomPage : public Page
{
public:
    explicit SomPage (SynthKeyProcessor& p) : Page (p)
    {
        buildEngine (0, { 0, 0, 420, 250 });
        buildEngine (1, { 428, 0, 420, 250 });
        buildGlobal ({ 856, 0, 300, 250 });
        buildFilters ({ 0, 258, 566, 194 });
        buildAmp ({ 574, 258, 250, 194 });
        buildSubNoise ({ 832, 258, 324, 194 });
        setMode (2);
    }
private:
    void buildEngine (int E, juce::Rectangle<int> r)
    {
        const int o = E ? (P_B_ON - P_A_ON) : 0; auto* s = addSection (E ? "MOTOR B" : "MOTOR A", r);
        auto type = [this, o] { return val (P_A_TYPE + o); };
        T (s, P_A_ON + o, "ATIVO", 74);
        C (s, P_A_TYPE + o, 110);
        s->add (new SampleChooser (proc, E), 190, 28, 1, [=] { return type() == 2; });
        C (s, P_A_WAVE + o, 110, 1, [=] { return type() == 0; });
        C (s, P_A_WTTAB + o, 140, 1, [=] { return type() == 1; });
        T (s, P_A_RETRIG + o, "RETRIG.", 74, 3, [=] { return type() != 2; });
        s->newRow();
        K (s, P_A_LEVEL + o, TX ("NÍVEL")); K (s, P_A_PAN + o, "PAN", 2); K (s, P_A_COARSE + o, "SEMITONS"); K (s, P_A_FINE + o, "FINO", 2);
        K (s, P_A_UNI + o, "UNISSON"); K (s, P_A_UNIDET + o, "DETUNE", 1, [=] { return val (P_A_UNI + o) > 1; }); K (s, P_A_UNISPR + o, "ESPALHAR", 2, [=] { return val (P_A_UNI + o) > 1; });
        // analógico
        K (s, P_A_PWM + o, "PWM", 1, [=] { return type() == 0 && val (P_A_WAVE + o) == 3; });
        K (s, P_A_SYNC + o, "SYNC", 2, [=] { return type() == 0; });
        K (s, P_A_FMAMT + o, "FM", 2, [=] { return type() == 0; });
        auto* fr = C (s, P_A_FMRAT + o, 52, 3, [=] { return type() == 0; }, 20);
        (void) fr;
        K (s, P_A_FOLD + o, "DOBRA", 2, [=] { return type() != 2; });
        // wavetable
        K (s, P_A_WTPOS + o, TX ("POSIÇÃO"), 1, [=] { return type() == 1; });
        K (s, P_A_WARP + o, "WARP", 2, [=] { return type() == 1; });
        // sample
        K (s, P_A_START + o, TX ("INÍCIO"), 2, [=] { return type() == 2; });
        T (s, P_A_LOOP + o, "LOOP", 62, 2, [=] { return type() == 2; }, 18);
        T (s, P_A_REV + o, "REV", 60, 2, [=] { return type() == 2; }, 18);
        T (s, P_A_RR + o, "RR", 56, 3, [=] { return type() == 2; }, 18);
        T (s, P_A_TRACK + o, "TECLA", 64, 3, [=] { return type() == 2; }, 18);
        K (s, P_A_PHASE + o, "FASE", 3, [=] { return type() != 2; });
    }
    void buildGlobal (juce::Rectangle<int> r)
    {
        auto* s = addSection (TX ("VOZES & AFINAÇÃO"), r);
        C (s, P_VOICE_MODE, 124); C (s, P_GLIDE_MODE, 100, 2);
        s->newRow();
        K (s, P_POLYPHONY, "POLIFONIA"); K (s, P_GLIDE, "GLIDE"); K (s, P_TRANSPOSE, "TRANSP."); K (s, P_BLEND, TX ("A ◂▸ B")); K (s, P_PB_RANGE, "PITCH BEND", 2);
        K (s, P_TUNE, TX ("AFINAÇÃO"), 2); K (s, P_VEL_SENS, "VELOC.", 2);
        s->newRow(); C (s, P_OUT_BUS, 140, 3);
    }
    void buildFilters (juce::Rectangle<int> r)
    {
        auto* s = addSection ("FILTROS", r);
        C (s, P_F1_TYPE, 116, 1, {}, 18);
        K (s, P_F1_CUT, "CORTE", 1, [this] { return val (P_F1_TYPE) != 0; }); K (s, P_F1_RES, "RESS.", 1, [this] { return val (P_F1_TYPE) != 0; });
        K (s, P_F1_DRIVE, "DRIVE", 2, [this] { return val (P_F1_TYPE) != 0; }); K (s, P_F1_KEY, "TECLA", 2, [this] { return val (P_F1_TYPE) != 0; }); K (s, P_F1_ENV, "ENV 2", 1, [this] { return val (P_F1_TYPE) != 0; });
        C (s, P_FILT_ROUTE, 108, 2, [this] { return val (P_F2_TYPE) != 0; }, 18);
        s->newRow();
        C (s, P_F2_TYPE, 116, 1, {}, 18);
        K (s, P_F2_CUT, "CORTE", 1, [this] { return val (P_F2_TYPE) != 0; }); K (s, P_F2_RES, "RESS.", 1, [this] { return val (P_F2_TYPE) != 0; });
        K (s, P_F2_DRIVE, "DRIVE", 2, [this] { return val (P_F2_TYPE) != 0; }); K (s, P_F2_KEY, "TECLA", 2, [this] { return val (P_F2_TYPE) != 0; }); K (s, P_F2_ENV, "ENV 3", 1, [this] { return val (P_F2_TYPE) != 0; });
        K (s, P_FILT_BAL, TX ("BALANÇO"), 2, [this] { return val (P_F2_TYPE) != 0; });
    }
    void buildAmp (juce::Rectangle<int> r)
    {
        auto* s = addSection ("ENVELOPE DE VOLUME", r);
        K (s, P_AMP_A, "ATAQUE", 1, {}, 54); K (s, P_AMP_D, "DECAIM.", 1, {}, 54); K (s, P_AMP_S, "SUSTAIN", 1, {}, 54); K (s, P_AMP_R, "LIBERT.", 1, {}, 54);
        s->newRow(); auto* ev = s->add (new EnvView (proc, P_AMP_A), 230, 60, 1); (void) ev;
    }
    void buildSubNoise (juce::Rectangle<int> r)
    {
        auto* s = addSection (TX ("SUB & RUÍDO"), r);
        K (s, P_SUB_LEVEL, "SUB"); K (s, P_NOISE_LEVEL, TX ("RUÍDO")); K (s, P_NOISE_COLOR, "COR", 2);
        s->newRow(); C (s, P_SUB_WAVE, 100, 2); C (s, P_SUB_OCT, 80, 2);
    }
};

// ==================================================================================== MATRIZ
class ModRow : public juce::Component
{
public:
    ModRow (SynthKeyProcessor& p, int slot) : index (slot)
    {
        const int b = modSlotBase (slot);
        src = std::make_unique<Combo> (p, b); dst = std::make_unique<Combo> (p, b + 1);
        amt.setSliderStyle (juce::Slider::LinearHorizontal); amt.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        attach = std::make_unique<juce::SliderParameterAttachment> (*p.param (b + 2), amt, nullptr);
        amt.setDoubleClickReturnValue (true, 0.0);
        amt.textFromValueFunction = [] (double v) { return juce::String ((int) std::lround (v * 100.0)) + " %"; };
        amt.onValueChange = [this] { repaint(); };
        addAndMakeVisible (*src); addAndMakeVisible (*dst); addAndMakeVisible (amt);
    }
    void resized() override
    {
        auto r = getLocalBounds(); r.removeFromLeft (26);
        src->setBounds (r.removeFromLeft (150).reduced (0, 1)); r.removeFromLeft (22);
        dst->setBounds (r.removeFromLeft (172).reduced (0, 1)); r.removeFromLeft (8);
        amt.setBounds (r.removeFromLeft (136)); valueArea = r;
    }
    void paint (juce::Graphics& g) override
    {
        g.setFont (theme::font (11.0f, true)); g.setColour (theme::dim()); g.drawText (juce::String (index + 1), 0, 0, 22, getHeight(), juce::Justification::centred);
        g.setColour (theme::dim().withAlpha (0.7f)); g.drawText (TX ("\xE2\x86\x92"), 176, 0, 22, getHeight(), juce::Justification::centred);
        g.setColour (std::abs (amt.getValue()) > 0.001 ? theme::accent() : theme::dim()); g.setFont (theme::font (11.5f));
        g.drawText (amt.getTextFromValue (amt.getValue()), valueArea, juce::Justification::centredRight);
    }
private:
    int index; std::unique_ptr<Combo> src, dst; juce::Slider amt; std::unique_ptr<juce::SliderParameterAttachment> attach; juce::Rectangle<int> valueArea;
};

class MatrixPage : public Page
{
public:
    explicit MatrixPage (SynthKeyProcessor& p) : Page (p)
    {
        buildLfo (0, { 0, 0, 276, 222 }); buildLfo (1, { 284, 0, 276, 222 });
        buildEnv (1, { 0, 230, 276, 222 }); buildEnv (2, { 284, 230, 276, 222 });
        auto* m = addSection (TX ("MATRIZ DE MODULAÇÃO  ·  origem → destino  ·  quantidade"), { 568, 0, 588, 452 });
        for (int i = 0; i < kNumModSlots; ++i) m->add (new ModRow (proc, i), 570, 24, i < 4 ? 1 : (i < 8 ? 2 : 3), {}, true);
        setMode (2);
    }
private:
    void buildLfo (int n, juce::Rectangle<int> r)
    {
        const int o = n ? (P_LFO2_WAVE - P_LFO1_WAVE) : 0; auto* s = addSection (n ? "LFO 2" : "LFO 1", r);
        C (s, P_LFO1_WAVE + o, 130); T (s, P_LFO1_SYNC + o, "SYNC", 66);
        s->newRow();
        K (s, P_LFO1_RATE + o, "TAXA", 1, [=] { return val (P_LFO1_SYNC + o) == 0; });
        C (s, P_LFO1_DIV + o, 76, 1, [=] { return val (P_LFO1_SYNC + o) != 0; }, 20);
        K (s, P_LFO1_FADE + o, "FADE-IN", 2); K (s, P_LFO1_PHASE + o, "FASE", 3);
        T (s, P_LFO1_KEY + o, "TECLA", 70, 3, {}, 20);
    }
    void buildEnv (int n, juce::Rectangle<int> r)
    {
        const int base = n == 1 ? P_ENV2_A : P_ENV3_A; auto* s = addSection (n == 1 ? TX ("ENVELOPE 2  ·  filtro 1") : TX ("ENVELOPE 3  ·  filtro 2"), r);
        K (s, base, "ATAQUE", 1, {}, 54); K (s, base + 1, "DECAIM.", 1, {}, 54); K (s, base + 2, "SUSTAIN", 1, {}, 54); K (s, base + 3, "LIBERT.", 1, {}, 54);
        s->newRow(); s->add (new EnvView (proc, base), 260, 80, 1);
    }
};

// =================================================================================== EFEITOS
class FxSlot : public juce::Component
{
public:
    FxSlot (SynthKeyProcessor& p, int slot, const juce::String& label) : proc (p), name (label), base (fxSlotBase (slot))
    {
        type = std::make_unique<Combo> (p, base + 0); addAndMakeVisible (*type);
        mix = std::make_unique<Knob> (p, base + 6, TX ("MIX"), false, true); addAndMakeVisible (*mix);
        for (int i = 0; i < 5; ++i) { k[i] = std::make_unique<Knob> (p, base + 1 + i, "", false, true); addAndMakeVisible (*k[i]); }
        refresh();
    }
    void setMode (int m) { mode = m; refresh(); resized(); }
    int signature() const { return ((int) proc.getReal (base)) * 10 + mode; }
    void refresh()
    {
        static const char* names[11][5] = {
            { "", "", "", "", "" }, { "GRAVES", "MÉDIOS", "FREQ.", "AGUDOS", "Q" }, { "LIMIAR", "RAZÃO", "ATAQUE", "LIBERT.", "GANHO" }, { "DRIVE", "TOM", "BIAS", "TIPO", "SAÍDA" },
            { "TAXA", "PROF.", "ATRASO", "VOZES", "LARGURA" }, { "TEMPO", "FEEDBACK", "TOM", "PING-PONG", "DESVIO" }, { "TAMANHO", "DECAIM.", "ABAFAR", "PRÉ-DELAY", "LARGURA" },
            { "TAXA", "PROF.", "FEEDBACK", "FREQ.", "ESPALHAR" }, { "BITS", "TAXA", "", "TOM", "SAÍDA" }, { "LARGURA", "CRUZAM.", "BALANÇO", "MÉDIOS", "LATERAIS" }, { "TAXA", "PROF.", "FORMA", "ESPALHAR", "" } };
        const int t = juce::jlimit (0, 10, (int) proc.getReal (base));
        for (int i = 0; i < 5; ++i)
        {
            const juce::String n = TX (names[t][i]); k[i]->setCaption (n);
            k[i]->setVisible (t != 0 && n.isNotEmpty() && (i < 3 || mode >= 2));
        }
        mix->setVisible (t != 0);
        rowVisible = t != 0;
    }
    void resized() override
    {
        type->setBounds (0, 0, 150, 28); mix->setBounds (getWidth() - 52, -2, 52, 68);
        int x = 0; for (int i = 0; i < 5; ++i) { k[i]->setBounds (x, 34, 52, 68); x += 56; }
        // desloca visivelmente os knobs ocultos? não: fixos nas suas posições
    }
    void paint (juce::Graphics& g) override
    {
        g.setColour (theme::dim()); g.setFont (theme::font (10.5f, true)); g.drawText (name, 158, 0, 60, 28, juce::Justification::centredLeft);
        g.setColour (theme::stroke().withAlpha (0.5f)); g.drawHorizontalLine (getHeight() - 1, 0.0f, (float) getWidth());
    }
private:
    SynthKeyProcessor& proc; juce::String name; int base; int mode = 2; bool rowVisible = false;
    std::unique_ptr<Combo> type; std::unique_ptr<Knob> mix; std::unique_ptr<Knob> k[5];
};

class FxPage : public Page
{
public:
    explicit FxPage (SynthKeyProcessor& p) : Page (p)
    {
        const char* titles[3] = { "INSERT A", "INSERT B", "AUX  ·  envio paralelo" }; const char* tag[3] = { "A", "B", "X" };
        for (int c = 0; c < 3; ++c)
        {
            auto* s = addSection (TX (titles[c]), { c * 388, 0, 380, 346 });
            for (int i = 0; i < 3; ++i)
            {
                auto* slot = s->add (new FxSlot (proc, c * 3 + i, juce::String (tag[c]) + juce::String (i + 1)), 364, 104, 1, {}, true);
                slots.add (slot);
            }
        }
        auto* o = addSection (TX ("SAÍDAS & ENVIOS"), { 0, 354, 1156, 98 });
        K (o, P_INSA_LEVEL, "INSERT A"); K (o, P_INSB_LEVEL, "INSERT B"); K (o, P_SEND_A, TX ("A → AUX")); K (o, P_SEND_B, TX ("B → AUX")); K (o, P_AUX_RETURN, "RETORNO AUX");
        C (o, P_OUT_BUS, 130, 2, {}, 20); T (o, P_LIM_ON, "LIMITADOR", 110, 1, {}, 20); K (o, P_LIM_CEIL, "TETO", 2, [this] { return proc.getReal (P_LIM_ON) > 0.5f; });
        setMode (2);
    }
    void setMode (int m) override { Page::setMode (m); for (auto* s : slots) s->setMode (m); }
    void tick() override
    {
        Page::tick(); int sig = 0; for (auto* s : slots) sig = sig * 131 + s->signature();
        if (sig != fxSig) { fxSig = sig; for (auto* s : slots) { s->refresh(); s->resized(); } }
    }
private:
    juce::Array<FxSlot*> slots; int fxSig = -1;
};

// ======================================================================================= ARP
class PatternView : public juce::Component
{
public:
    explicit PatternView (SynthKeyProcessor& p) : proc (p) { setMouseCursor (juce::MouseCursor::UpDownResizeCursor); }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
        const bool edit = proc.getReal (P_ARP_EDIT) > 0.5f; const int len = edit ? juce::jlimit (1, 16, (int) proc.getReal (P_ARP_LEN)) : 16;
        g.setColour (theme::dim()); g.setFont (theme::font (11.0f));
        g.drawText (TX (edit ? "PADR\xC3\x83O PR\xC3\x93PRIO  \xC2\xB7  " : "PADR\xC3\x83O  \xC2\xB7  ") + juce::String (len) + " passos", r.removeFromTop (16).toNearestInt(), juce::Justification::centredLeft);
        g.drawText (TX ("arrasta as barras (em baixo = pausa)  \xC2\xB7  clica no n\xC3\xBAmero = oitava  \xC2\xB7  bot\xC3\xA3o direito = repetir nota"), getLocalBounds().withTrimmedRight (14).withHeight (28), juce::Justification::centredRight);
        const int pi = juce::jlimit (0, 7, (int) proc.getReal (P_ARP_PATTERN)); const auto& pat = kArpPatterns[pi];
        barArea = r; cw = r.getWidth() / 16.0f; H = r.getHeight() - 14.0f;
        for (int i = 0; i < 16; ++i)
        {
            ArpStep st = pat.s[i % pat.len];
            if (edit)
            {
                const int b = P_ARP_V1 + 3 * i; st.vel = proc.getReal (b); st.gate = 1.0f; st.oct = (int8_t) std::lround (proc.getReal (b + 1));
                const int rr = (int) std::lround (proc.getReal (b + 2)); st.flags = st.vel < 0.02f ? AF_REST : (rr >= 2 ? AF_R3 : (rr == 1 ? AF_R2 : 0));
            }
            const bool off = edit && i >= len;
            auto cell = juce::Rectangle<float> (r.getX() + cw * (float) i + 3, r.getY(), cw - 6, H);
            g.setColour (juce::Colour (off ? 0xFF0B0D1B : 0xFF11142A)); g.fillRoundedRectangle (cell, 5.0f);
            if (off) { g.setColour (theme::dim().withAlpha (0.25f)); g.fillRect (cell.getX() + 4, cell.getBottom() - 6, cell.getWidth() - 8, 2.0f); }
            else if (st.flags & AF_REST) { g.setColour (theme::dim().withAlpha (0.35f)); g.fillRect (cell.getX() + 4, cell.getBottom() - 6, cell.getWidth() - 8, 2.0f); }
            else if (st.flags & AF_TIE) { g.setColour (theme::accent2().withAlpha (0.8f)); g.fillRoundedRectangle (cell.withTop (cell.getBottom() - 10).reduced (2, 0), 3.0f); }
            else
            {
                const float h = std::max (6.0f, H * st.vel * (st.gate > 0.01f ? 1.0f : 0.5f)); const auto bar = juce::Rectangle<float> (cell.getX() + 3, cell.getBottom() - h, cell.getWidth() - 6, h);
                g.setGradientFill (juce::ColourGradient (theme::accent(), bar.getX(), bar.getY(), theme::accent2(), bar.getX(), bar.getBottom(), false)); g.fillRoundedRectangle (bar, 3.0f);
                if (st.oct != 0) { g.setColour (juce::Colours::white); g.setFont (theme::font (10.0f, true)); g.drawText (st.oct > 0 ? "+8" : "-8", cell.toNearestInt().withHeight (14), juce::Justification::centred); }
                if (st.flags & (AF_R2 | AF_R3)) { g.setColour (juce::Colours::white.withAlpha (0.9f)); g.setFont (theme::font (10.0f, true)); g.drawText ((st.flags & AF_R3) ? "x3" : "x2", cell.toNearestInt().withTrimmedTop ((int) H - 14), juce::Justification::centred); }
            }
            g.setColour (off ? theme::dim().withAlpha (0.3f) : theme::dim().withAlpha (0.7f)); g.setFont (theme::font (9.5f)); g.drawText (juce::String (i + 1), (int) cell.getX(), (int) (cell.getBottom() + 1), (int) cell.getWidth(), 12, juce::Justification::centred);
        }
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        const int i = columnAt (e.position.x); if (i < 0) return;
        if (proc.getReal (P_ARP_EDIT) > 0.5f && i >= juce::jlimit (1, 16, (int) proc.getReal (P_ARP_LEN))) return;
        const bool onLabel = e.position.y > barArea.getY() + H + 2.0f;
        ensureEditable();
        const int b = P_ARP_V1 + 3 * i;
        if (e.mods.isPopupMenu())                     // botão direito: repetições 0 -> x2 -> x3
        { proc.setReal (b + 2, (float) (((int) std::lround (proc.getReal (b + 2)) + 1) % 3)); if (proc.getReal (b) < 0.02f) proc.setReal (b, 0.7f); }
        else if (onLabel)                             // número: oitava 0 -> +8 -> -8
        { const int o = (int) std::lround (proc.getReal (b + 1)); proc.setReal (b + 1, (float) (o == 0 ? 1 : (o == 1 ? -1 : 0))); if (proc.getReal (b) < 0.02f) proc.setReal (b, 0.7f); }
        else setLevel (i, e.position.y);
        repaint();
    }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu() || e.mouseDownPosition.y > barArea.getY() + H + 2.0f) return;
        const int i = columnAt (e.position.x); if (i < 0) return;
        if (i >= juce::jlimit (1, 16, (int) proc.getReal (P_ARP_LEN))) return;
        setLevel (i, e.position.y); repaint();
    }
private:
    int columnAt (float x) const { const int i = (int) std::floor ((x - barArea.getX()) / cw); return i >= 0 && i < 16 ? i : -1; }
    void setLevel (int i, float y)
    {
        float v = 1.0f - (y - barArea.getY()) / std::max (1.0f, H); v = juce::jlimit (0.0f, 1.0f, v);
        v = v < 0.06f ? 0.0f : std::round (v * 20.0f) / 20.0f;         // em baixo = pausa; senão passos de 5 %
        proc.setReal (P_ARP_V1 + 3 * i, v);
    }
    // na primeira edição copia o padrão de fábrica para os 16 passos e liga o "padrão próprio"
    void ensureEditable()
    {
        if (proc.getReal (P_ARP_EDIT) > 0.5f) return;
        const auto& pat = kArpPatterns[juce::jlimit (0, 7, (int) proc.getReal (P_ARP_PATTERN))];
        for (int i = 0; i < 16; ++i)
        {
            const auto& st = pat.s[i % pat.len]; const int b = P_ARP_V1 + 3 * i;
            const bool rest = (st.flags & (AF_REST | AF_TIE)) != 0;
            proc.setReal (b, rest ? 0.0f : std::max (0.1f, st.vel)); proc.setReal (b + 1, (float) st.oct);
            proc.setReal (b + 2, (st.flags & AF_R3) ? 2.0f : ((st.flags & AF_R2) ? 1.0f : 0.0f));
        }
        proc.setReal (P_ARP_LEN, (float) (pat.len >= 16 ? 16 : (pat.len == 8 ? 8 : 16))); proc.setReal (P_ARP_EDIT, 1.0f);
    }
    SynthKeyProcessor& proc; juce::Rectangle<float> barArea; float cw = 60.0f, H = 100.0f;
};

class ArpPage : public Page
{
public:
    explicit ArpPage (SynthKeyProcessor& p) : Page (p)
    {
        auto* a = addSection ("ACORDES & ESCALA", { 0, 0, 380, 222 });
        C (a, P_CHORD_TYPE, 170); T (a, P_SCALE_SNAP, TX ("FORÇAR ESCALA"), 140);
        a->newRow(); C (a, P_SCALE_KEY, 110); C (a, P_SCALE_TYPE, 190);
        auto* b = addSection ("ARPEJADOR", { 388, 0, 768, 222 });
        T (b, P_ARP_ON, "LIGADO", 90); C (b, P_ARP_MODE, 150); C (b, P_ARP_RATE, 80); C (b, P_ARP_PATTERN, 190); T (b, P_ARP_LATCH, "LATCH", 80, 2);
        b->newRow(); K (b, P_ARP_GATE, "GATE"); K (b, P_ARP_OCT, "OITAVAS"); K (b, P_ARP_SWING, "SWING"); K (b, P_ARP_LEN, "PASSOS");
        T (b, P_ARP_EDIT, TX ("PADR\xC3\x83O PR\xC3\x93PRIO"), 170, 1, {}, 20);
        pattern = new PatternView (proc); addAndMakeVisible (pattern); pattern->setBounds (0, 230, 1156, 222);
        setMode (2);
    }
    ~ArpPage() override { delete pattern; }
    void paint (juce::Graphics& g) override { theme::drawCard (g, pattern->getBounds().toFloat()); }
    void tick() override { Page::tick(); pattern->repaint(); }
private:
    PatternView* pattern = nullptr;
};
} // namespace ui
