// =============================================================================
//  SYNTHKEY by Piradex — páginas PACKS e ADMIN, janela de login e caixa de diálogo
// =============================================================================
#pragma once
#include "Pages1.h"

namespace ui
{
struct Services
{
    std::function<void (const juce::String& title, const juce::String& info, const juce::String& initial, const juce::String& okText,
                        std::function<juce::String (const juce::String&)> onOk)> prompt;
    std::function<void (const juce::String&, bool isError)> toast;
};

// ================================================================================ caixa de diálogo (texto)
class PromptOverlay : public juce::Component
{
public:
    PromptOverlay()
    {
        addAndMakeVisible (edit); addAndMakeVisible (ok); addAndMakeVisible (cancel);
        edit.setFont (theme::font (16.0f)); edit.setTextToShowWhenEmpty ("", theme::dim()); edit.setSelectAllWhenFocused (true);
        edit.onReturnKey = [this] { submit(); }; edit.onEscapeKey = [this] { close(); };
        ok.onClick = [this] { submit(); }; cancel.setButtonText (TX ("CANCELAR")); cancel.onClick = [this] { close(); };
        setVisible (false);
    }
    void open (const juce::String& t, const juce::String& i, const juce::String& initial, const juce::String& okText, std::function<juce::String (const juce::String&)> cb)
    {
        title = t; info = i; error.clear(); onOk = std::move (cb); ok.setButtonText (okText); edit.setText (initial, false);
        setVisible (true); toFront (true); resized(); edit.grabKeyboardFocus(); repaint();
    }
    void close() { setVisible (false); onOk = nullptr; }
    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xB0050610));
        theme::drawCard (g, card.toFloat(), 14.0f, true);
        g.setColour (theme::accent()); g.fillRoundedRectangle ((float) card.getX() + 20, (float) card.getY() + 22, 3.0f, 16.0f, 1.5f);
        g.setColour (theme::text()); g.setFont (theme::font (17.0f, true)); g.drawText (title, card.getX() + 32, card.getY() + 16, card.getWidth() - 50, 28, juce::Justification::centredLeft);
        g.setColour (theme::dim()); g.setFont (theme::font (12.5f)); g.drawFittedText (info, card.getX() + 24, card.getY() + 52, card.getWidth() - 48, 34, juce::Justification::topLeft, 2);
        if (error.isNotEmpty()) { g.setColour (theme::bad()); g.setFont (theme::font (12.5f, true)); g.drawText (error, card.getX() + 24, card.getBottom() - 78, card.getWidth() - 48, 20, juce::Justification::centredLeft); }
    }
    void resized() override
    {
        card = juce::Rectangle<int> (0, 0, 480, 220).withCentre (getLocalBounds().getCentre());
        edit.setBounds (card.getX() + 24, card.getY() + 94, card.getWidth() - 48, 34);
        ok.setBounds (card.getRight() - 24 - 150, card.getBottom() - 52, 150, 34); cancel.setBounds (card.getRight() - 24 - 150 - 8 - 110, card.getBottom() - 52, 110, 34);
    }
    void mouseDown (const juce::MouseEvent& e) override { if (! card.contains (e.getPosition())) close(); }
private:
    void submit()
    {
        if (! onOk) return;
        const auto msg = onOk (edit.getText());
        if (msg.isEmpty()) close(); else { error = msg; repaint(); }
    }
    juce::TextEditor edit; juce::TextButton ok, cancel; juce::String title, info, error; juce::Rectangle<int> card;
    std::function<juce::String (const juce::String&)> onOk;
};

// ======================================================================================= login
class LoginOverlay : public juce::Component
{
public:
    std::function<void()> onUnlocked;
    LoginOverlay()
    {
        addAndMakeVisible (pw); addAndMakeVisible (remember); addAndMakeVisible (enter);
        pw.setPasswordCharacter ((juce::juce_wchar) 0x2022); pw.setFont (theme::font (18.0f)); pw.setJustification (juce::Justification::centred);
        pw.setTextToShowWhenEmpty (TX ("senha de acesso"), theme::dim()); pw.onReturnKey = [this] { attempt(); };
        remember.setButtonText (TX ("Lembrar neste computador")); remember.setToggleState (true, juce::dontSendNotification);
        enter.setButtonText (TX ("ENTRAR")); enter.onClick = [this] { attempt(); };
    }
    void focus() { pw.grabKeyboardFocus(); }
    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xE8050610));
        theme::drawCard (g, card.toFloat(), 16.0f, true);
        // logótipo
        auto t = card.withHeight (70).translated (0, 28);
        for (int i = 3; i >= 0; --i) { g.setColour (theme::accent().withAlpha (0.05f * (float) (4 - i))); g.setFont (theme::font (44.0f + (float) i * 2, true)); g.drawText ("SYNTHKEY", t.expanded (i * 2, 0), juce::Justification::centred); }
        g.setGradientFill (juce::ColourGradient (theme::accent(), (float) t.getX() + 100, 0, theme::accent2(), (float) t.getRight() - 100, 0, false)); g.setFont (theme::font (44.0f, true)); g.drawText ("SYNTHKEY", t, juce::Justification::centred);
        g.setColour (theme::dim()); g.setFont (theme::font (12.5f)); g.drawText (TX ("by Piradex  \xC2\xB7  BeatFreak Studio"), card.getX(), card.getY() + 92, card.getWidth(), 18, juce::Justification::centred);
        g.setColour (theme::text()); g.setFont (theme::font (13.0f)); g.drawText (TX ("Acesso restrito \xC2\xB7 vers\xC3\xA3o beta"), card.getX(), card.getY() + 126, card.getWidth(), 18, juce::Justification::centred);
        if (message.isNotEmpty()) { g.setColour (isError ? theme::bad() : theme::good()); g.setFont (theme::font (13.0f, true)); g.drawText (message, card.getX() + 20, card.getBottom() - 60, card.getWidth() - 40, 22, juce::Justification::centred); }
        g.setColour (theme::dim().withAlpha (0.7f)); g.setFont (theme::font (11.0f)); g.drawText ("v" SK_VERSION_STRING, card.getX(), card.getBottom() - 24, card.getWidth(), 16, juce::Justification::centred);
    }
    void resized() override
    {
        card = juce::Rectangle<int> (0, 0, 460, 360).withCentre (getLocalBounds().getCentre());
        pw.setBounds (card.getX() + 60, card.getY() + 158, card.getWidth() - 120, 40);
        remember.setBounds (card.getX() + 100, card.getY() + 208, card.getWidth() - 200, 24);
        enter.setBounds (card.getX() + 100, card.getY() + 244, card.getWidth() - 200, 40);
    }
private:
    void attempt()
    {
        if (busy || pw.getText().trim().isEmpty()) return;
        busy = true; enter.setEnabled (false); message = TX ("A verificar\xE2\x80\xA6"); isError = false; repaint();
        const auto text = pw.getText(); const bool rem = remember.getToggleState(); juce::Component::SafePointer<LoginOverlay> self (this);
        juce::Thread::launch ([self, text, rem]
        {
            const auto m = AccessManager::matchPassword (text);          // PBKDF2: fora da thread de mensagens
            juce::MessageManager::callAsync ([self, m, rem]
            {
                if (self == nullptr) return;
                const auto r = AccessManager::get().applyLogin (m, rem);
                self->busy = false; self->enter.setEnabled (true);
                if (r == AccessManager::LoginResult::Ok) { self->pw.clear(); self->message.clear(); if (self->onUnlocked) self->onUnlocked(); }
                else { self->isError = true; self->message = r == AccessManager::LoginResult::Expired ? TX ("Esta senha beta j\xC3\xA1 expirou.") : TX ("Senha incorreta."); self->pw.selectAll(); self->pw.grabKeyboardFocus(); }
                self->repaint();
            });
        });
    }
    juce::TextEditor pw; juce::ToggleButton remember; juce::TextButton enter; juce::String message; bool isError = false, busy = false; juce::Rectangle<int> card;
};

// ======================================================================================= PACKS
class PackCard : public juce::Component
{
public:
    PackCard (SynthKeyProcessor& p, Services& s, const std::string& packId) : proc (p), svc (s), id (packId)
    {
        addAndMakeVisible (primary); addAndMakeVisible (secondary);
        primary.onClick = [this] { onPrimary(); }; secondary.onClick = [this] { onSecondary(); };
        refresh (true);
    }
    int signature() const { return sig; }
    void refresh (bool force = false)
    {
        PackEntry e2; if (! PackHub::get().entry (id, e2)) return;
        const auto st = AccessManager::get().packStatus (id, e2.core);
        const bool loading = PackHub::get().isLoading (id);
        const int ns = (int) st.access * 1000 + (e2.installed ? 1 : 0) * 100 + (loading ? 10 : 0) + st.daysLeft * 100000 + (PackHub::get().isLoadFailed (id) ? 1 : 0);
        if (! force && ns == sig) return;
        sig = ns; e = e2; status = st; isLoading = loading;
        if (! e.installed) { primary.setButtonText (TX ("INSTALAR\xE2\x80\xA6")); secondary.setVisible (false); }
        else if (! st.usable()) { primary.setButtonText (TX ("INSERIR CHAVE")); secondary.setButtonText (TX ("PR\xC3\x89-OUVIR")); secondary.setVisible (true); }
        else { primary.setButtonText (TX ("USAR PRESETS")); secondary.setButtonText (TX ("MAIS CHAVE")); secondary.setVisible (st.access == sk::lic::Access::Key); }
        resized(); repaint();
    }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat(); const auto accent = juce::Colour::fromString ("FF" + juce::String (e.accent).substring (1));
        theme::drawCard (g, r, 12.0f, true);
        // capa inteira (sem cortes): proporção original centrada; as laterais levam a própria capa desfocada/escurecida
        const auto strip = juce::Rectangle<float> (r.getX() + 4.0f, r.getY() + 4.0f, r.getWidth() - 8.0f, kCoverH);
        auto cover = strip;
        if (e.cover.isValid())
        {
            const float ar = (float) e.cover.getWidth() / (float) e.cover.getHeight();
            cover = juce::Rectangle<float> (0, 0, std::min (strip.getWidth(), strip.getHeight() * ar), std::min (strip.getHeight(), strip.getWidth() / ar)).withCentre (strip.getCentre());
        }
        {
            juce::Path clip; clip.addRoundedRectangle (strip, 9.0f); juce::Graphics::ScopedSaveState ss (g); g.reduceClipRegion (clip);
            g.setColour (juce::Colour (0xFF06070E)); g.fillRect (strip);
            if (e.cover.isValid())
            {
                const float sc = std::max (strip.getWidth() / (float) e.cover.getWidth(), strip.getHeight() / (float) e.cover.getHeight());
                const float w = (float) e.cover.getWidth() * sc, h = (float) e.cover.getHeight() * sc;
                g.setOpacity (0.28f); g.drawImage (e.cover, juce::Rectangle<float> (strip.getCentreX() - w * 0.5f, strip.getCentreY() - h * 0.5f, w, h), juce::RectanglePlacement::stretchToFit);
                g.setOpacity (e.installed && ! status.usable() ? 0.62f : 1.0f);
                g.drawImage (e.cover, cover, juce::RectanglePlacement::stretchToFit); g.setOpacity (1.0f);
            }
        }
        g.setColour (accent.withAlpha (0.9f)); g.drawRoundedRectangle (strip, 9.0f, 1.2f);
        // estado (esquerda) e contagem (direita)
        juce::String tx; juce::Colour col = theme::dim();
        using A = sk::lic::Access;
        if (! e.installed) { tx = TX ("N\xC3\x83O INSTALADO"); }
        else if (isLoading) { tx = TX ("A CARREGAR\xE2\x80\xA6"); col = theme::accent(); }
        else switch (status.access)
        {
            case A::Core:    tx = TX ("INCLU\xC3\x8D" "DO"); col = theme::good(); break;
            case A::Master:  tx = "MESTRE"; col = theme::good(); break;
            case A::Beta:    tx = TX ("BETA \xC2\xB7 ABERTO"); col = theme::good(); break;
            case A::Key:     tx = status.daysLeft < 0 ? juce::String ("ATIVO") : TX ("ATIVO \xC2\xB7 ") + juce::String (status.daysLeft) + (status.daysLeft == 1 ? " DIA" : " DIAS"); col = status.daysLeft >= 0 && status.daysLeft <= 7 ? theme::warn() : theme::good(); break;
            case A::Expired: tx = "EXPIRADO"; col = theme::bad(); break;
            case A::Locked:  tx = TX ("\xF0\x9F\x94\x92 BLOQUEADO"); col = theme::warn(); break;
        }
        const int ty = (int) strip.getBottom() + 4;
        g.setFont (theme::font (10.5f, true)); g.setColour (col); g.drawText (tx, (int) r.getX() + 10, ty, (int) r.getWidth() / 2, 14, juce::Justification::centredLeft);
        if (e.installed) { g.setColour (theme::dim()); g.setFont (theme::font (10.5f));
            g.drawText (juce::String (e.insts.size()) + " inst.  " + TX ("\xC2\xB7") + "  " + juce::String (e.presetCount) + " presets", (int) r.getX() + (int) r.getWidth() / 2, ty, (int) r.getWidth() / 2 - 10, 14, juce::Justification::centredRight); }
    }
    static constexpr float kCoverH = 156.0f;
    void resized() override
    {
        auto b = getLocalBounds().reduced (10, 0).removeFromBottom (26).withTrimmedBottom (4);
        if (secondary.isVisible()) { auto l = b.removeFromLeft (b.getWidth() * 55 / 100 - 3); primary.setBounds (l); b.removeFromLeft (6); secondary.setBounds (b); }
        else primary.setBounds (b);
    }
private:
    void onPrimary()
    {
        if (! e.installed) { chooseFile(); return; }
        if (! status.usable()) { askKey(); return; }
        loadFirstPreset();
    }
    void onSecondary() { if (! status.usable()) loadFirstPreset(); else askKey(); }
    void loadFirstPreset()
    {
        PackHub::get().ensureLoaded (id);
        const auto& list = proc.presetList();
        for (size_t i = 0; i < list.size(); ++i) if (list[i].pack == id) { proc.loadPresetIndex ((int) i); svc.toast (TX ("Preset carregado: ") + TX (list[i].name.c_str()), false); return; }
        svc.toast (TX ("Este pack ainda n\xC3\xA3o tem presets."), true);
    }
    void askKey()
    {
        const juce::String nm = TX (e.name.c_str()); const std::string pid = id;
        svc.prompt (TX ("Inserir chave \xC2\xB7 ") + nm, TX ("Cole a chave do pack (formato PREFIXO-XXXX-XXXX-XXXX-XXXX). Cada estilo tem a sua pr\xC3\xB3pria chave."), "", TX ("ATIVAR"),
            [this, pid, nm] (const juce::String& txt) -> juce::String
            {
                using R = sk::lic::ActivateResult; const auto r = AccessManager::get().activate (pid, txt);
                if (r == R::Invalid) return TX ("Chave inv\xC3\xA1lida para este pack.");
                if (r == R::Expired) return TX ("Esta chave j\xC3\xA1 expirou.");
                if (r == R::AlreadyActive) return TX ("Esta chave j\xC3\xA1 est\xC3\xA1 ativa.");
                PackHub::get().ensureLoaded (pid); svc.toast (nm + TX (" ativado."), false); refresh (true); return {};
            });
    }
    void chooseFile()
    {
        chooser = std::make_shared<juce::FileChooser> (TX ("Escolha o ficheiro do pack (.bfpack)"), juce::File::getSpecialLocation (juce::File::userDocumentsDirectory), "*.bfpack");
        auto ch = chooser; juce::Component::SafePointer<PackCard> self (this);
        ch->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [self, ch] (const juce::FileChooser& fc)
        {
            if (self == nullptr || fc.getResult() == juce::File()) return;
            const auto err = PackHub::get().install (fc.getResult());
            self->svc.toast (err.isEmpty() ? TX ("Pack instalado.") : err, err.isNotEmpty());
        });
    }
    SynthKeyProcessor& proc; Services& svc; std::string id; PackEntry e; sk::lic::PackStatus status; bool isLoading = false; int sig = -1;
    juce::TextButton primary, secondary; std::shared_ptr<juce::FileChooser> chooser;
};

class PacksPage : public Page
{
public:
    PacksPage (SynthKeyProcessor& p, Services& s) : Page (p), svc (s)
    {
        int i = 0;
        for (auto& e : PackHub::get().entries())
        {
            auto* c = new PackCard (p, s, e.id); cards.add (c); addAndMakeVisible (c);
            const int col = i % 4, row = i / 4; c->setBounds (col * 291, row * 214, 283, 210); ++i;
        }
        install.setButtonText (TX ("INSTALAR PACK (.bfpack)\xE2\x80\xA6")); folder.setButtonText (TX ("ABRIR PASTA DE PACKS"));
        addAndMakeVisible (install); addAndMakeVisible (folder);
        install.setBounds (0, 428, 230, 24); folder.setBounds (238, 428, 200, 24);
        install.onClick = [this] { chooseFile(); };
        folder.onClick = [] { skpaths::userPacks().revealToUser(); };
    }
    void tick() override { ++ticks; if (ticks % 10 == 1) for (auto* c : cards) c->refresh(); }
    void paint (juce::Graphics& g) override
    {
        g.setColour (theme::dim()); g.setFont (theme::font (11.5f));
        g.drawText (TX ("Arraste ficheiros .bfpack para a janela do plugin \xC2\xB7 ") + skpaths::userPacks().getFullPathName(), 450, 428, getWidth() - 450, 24, juce::Justification::centredLeft);
    }
    void rebuildIfNeeded()
    {
        if ((int) PackHub::get().entries().size() != cards.size()) { /* nº de packs é fixo */ }
        for (auto* c : cards) c->refresh (true);
    }
private:
    void chooseFile()
    {
        chooser = std::make_shared<juce::FileChooser> (TX ("Escolha o ficheiro do pack (.bfpack)"), juce::File::getSpecialLocation (juce::File::userDocumentsDirectory), "*.bfpack");
        auto ch = chooser; juce::Component::SafePointer<PacksPage> self (this);
        ch->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [self, ch] (const juce::FileChooser& fc)
        {
            if (self == nullptr || fc.getResult() == juce::File()) return;
            const auto err = PackHub::get().install (fc.getResult());
            self->svc.toast (err.isEmpty() ? TX ("Pack instalado.") : err, err.isNotEmpty());
            self->rebuildIfNeeded();
        });
    }
    Services& svc; juce::OwnedArray<PackCard> cards; juce::TextButton install, folder; std::shared_ptr<juce::FileChooser> chooser; int ticks = 0;
};

// ======================================================================================= ADMIN
class AdminPage : public Page, private juce::ListBoxModel
{
public:
    AdminPage (SynthKeyProcessor& p, Services& s) : Page (p), svc (s)
    {
        static const char* packIds[7] = { "trap", "rnb", "afrobeat", "amapiano", "drill", "kizomba", "kuduro" };
        static const char* packNames[7] = { "Trap", "R&B", "Afrobeats", "Amapiano", "Drill", "Kizomba \xC2\xB7 Zouk", "Kuduro" };
        for (int i = 0; i < 7; ++i)
        {
            auto* t = new juce::ToggleButton (TX (packNames[i])); packToggles.add (t); addAndMakeVisible (t); t->getProperties().set ("pack", packIds[i]);
            t->setBounds (24 + (i / 4) * 240, 52 + (i % 4) * 24, 220, 24);
        }
        for (auto* c : { (juce::Component*) &validity, (juce::Component*) &seats }) addAndMakeVisible (c);
        validity.addItem (TX ("Permanente"), 1); validity.addItem (TX ("At\xC3\xA9 uma data"), 2); validity.addItem (TX ("N dias ap\xC3\xB3s a ativa\xC3\xA7\xC3\xA3o"), 3); validity.setSelectedId (1);
        validity.onChange = [this] { updateFields(); };
        seats.addItem (TX ("1 computador"), 1); seats.addItem (TX ("2 computadores"), 2); seats.addItem (TX ("3 computadores"), 3); seats.setSelectedId (1);
        for (auto* e : { &dateEd, &daysEd, &qtyEd, &noteEd }) { addAndMakeVisible (e); e->setFont (theme::font (14.0f)); }
        dateEd.setTextToShowWhenEmpty ("dd/mm/aaaa", theme::dim()); dateEd.setInputRestrictions (10, "0123456789/");
        daysEd.setText ("30", false); daysEd.setInputRestrictions (4, "0123456789"); qtyEd.setText ("1", false); qtyEd.setInputRestrictions (3, "0123456789");
        noteEd.setTextToShowWhenEmpty (TX ("cliente / nota (opcional)"), theme::dim());
        addAndMakeVisible (gen); gen.setButtonText (TX ("GERAR CHAVES")); gen.onClick = [this] { generate(); };
        addAndMakeVisible (out); out.setMultiLine (true); out.setReadOnly (true); out.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::plain)); out.setColour (juce::TextEditor::textColourId, theme::accent());
        addAndMakeVisible (copyOut); copyOut.setButtonText (TX ("COPIAR")); copyOut.onClick = [this] { juce::SystemClipboard::copyTextToClipboard (out.getText()); svc.toast (TX ("Copiado."), false); };
        addAndMakeVisible (csvOut); csvOut.setButtonText (TX ("EXPORTAR CSV")); csvOut.onClick = [this] { exportCsv (lastBatch); };
        addAndMakeVisible (sim); sim.setButtonText (TX ("Simular cliente (ignorar acesso mestre/beta aos packs)")); sim.onClick = [this] { AccessManager::get().setSimulateCustomer (sim.getToggleState()); };
        addAndMakeVisible (list); list.setModel (this); list.setRowHeight (26);
        addAndMakeVisible (copyKey); copyKey.setButtonText (TX ("COPIAR CHAVE")); copyKey.onClick = [this] { copySelected(); };
        addAndMakeVisible (delKey); delKey.setButtonText (TX ("REMOVER")); delKey.onClick = [this] { removeSelected(); };
        addAndMakeVisible (csvAll); csvAll.setButtonText (TX ("EXPORTAR TUDO")); csvAll.onClick = [this] { exportCsv (rows); };
        validity.setBounds (24, 176, 230, 28); dateEd.setBounds (266, 176, 130, 28); daysEd.setBounds (266, 176, 80, 28);
        seats.setBounds (24, 226, 150, 28); qtyEd.setBounds (184, 226, 60, 28); noteEd.setBounds (254, 226, 242, 28);
        gen.setBounds (24, 268, 190, 36); out.setBounds (24, 312, 472, 88); copyOut.setBounds (24, 410, 110, 30); csvOut.setBounds (142, 410, 130, 30);
        sim.setBounds (284, 410, 212, 30); list.setBounds (544, 54, 596, 326);
        copyKey.setBounds (544, 394, 140, 32); delKey.setBounds (692, 394, 110, 32); csvAll.setBounds (810, 394, 140, 32);
        updateFields(); reload();
    }
    ~AdminPage() override { list.setModel (nullptr); }
    void paint (juce::Graphics& g) override
    {
        theme::drawCard (g, juce::Rectangle<float> (0, 0, 520, 452)); theme::drawCard (g, juce::Rectangle<float> (528, 0, 628, 452));
        g.setColour (theme::accent()); g.fillRoundedRectangle (10.0f, 9.0f, 3.0f, 12.0f, 1.5f); g.fillRoundedRectangle (538.0f, 9.0f, 3.0f, 12.0f, 1.5f);
        g.setFont (theme::font (11.5f, true)); g.setColour (theme::text()); g.drawText (TX ("GERADOR DE CHAVES  \xC2\xB7  s\xC3\xB3 administrador"), 18, 5, 480, 20, juce::Justification::centredLeft);
        g.drawText (TX ("REGISTO DE CHAVES EMITIDAS"), 546, 5, 400, 20, juce::Justification::centredLeft);
        g.setColour (theme::dim()); g.setFont (theme::font (11.0f));
        g.drawText (TX ("PACKS (um\xE2\x80\xA6 ou v\xC3\xA1rios)"), 24, 32, 300, 18, juce::Justification::centredLeft); g.drawText ("VALIDADE", 24, 156, 200, 16, juce::Justification::centredLeft);
        g.drawText ("COMPUTADORES  /  QUANT.  /  NOTA", 24, 208, 400, 14, juce::Justification::centredLeft);
        // cabeçalho da tabela
        g.setColour (theme::dim().withAlpha (0.9f)); g.setFont (theme::font (10.5f, true));
        g.drawText ("DATA", 556, 34, 80, 16, juce::Justification::centredLeft); g.drawText ("PACK", 640, 34, 70, 16, juce::Justification::centredLeft); g.drawText ("CHAVE", 712, 34, 200, 16, juce::Justification::centredLeft);
        g.drawText ("ESTADO", 904, 34, 130, 16, juce::Justification::centredLeft); g.drawText ("NOTA", 1030, 34, 100, 16, juce::Justification::centredLeft);
        g.setColour (theme::dim().withAlpha (0.6f)); g.setFont (theme::font (10.5f));
        g.drawText (TX ("As chaves ficam registadas em ") + skpaths::issuedFile().getFileName() + TX (" (neste computador, s\xC3\xB3 para o administrador)."), 544, 430, 600, 16, juce::Justification::centredLeft);
    }
    void tick() override { ++ticks; if (ticks % 30 == 1) list.repaint(); }
    void reload() { rows = AccessManager::get().issuedKeys(); std::reverse (rows.begin(), rows.end()); list.updateContent(); list.repaint(); }
private:
    void updateFields() { const int v = validity.getSelectedId(); dateEd.setVisible (v == 2); daysEd.setVisible (v == 3); }
    void generate()
    {
        auto& acc = AccessManager::get(); if (! acc.isAdmin()) { svc.toast (TX ("Sem permiss\xC3\xA3o de administrador."), true); return; }
        sk::lic::IssueRequest rq; for (auto* t : packToggles) if (t->getToggleState()) rq.packs.push_back (t->getProperties()["pack"].toString().toStdString());
        if (rq.packs.empty()) { svc.toast (TX ("Escolha pelo menos um pack."), true); return; }
        const int v = validity.getSelectedId(); const int today = acc.today();
        if (v == 1) rq.validity = sk::lic::IssueRequest::Permanent;
        else if (v == 2)
        {
            const auto parts = juce::StringArray::fromTokens (dateEd.getText(), "/", ""); int d = 0, m = 0, y = 0;
            if (parts.size() == 3) { d = parts[0].getIntValue(); m = parts[1].getIntValue(); y = parts[2].getIntValue(); }
            if (d < 1 || d > 31 || m < 1 || m > 12 || y < 2026 || y > 2060) { svc.toast (TX ("Data inv\xC3\xA1lida (use dd/mm/aaaa)."), true); return; }
            rq.validity = sk::lic::IssueRequest::UntilDate; rq.untilDay = sk::keys::daysSince2026 (y, m, d);
            if (rq.untilDay < today) { svc.toast (TX ("A data j\xC3\xA1 passou."), true); return; }
        }
        else { rq.validity = sk::lic::IssueRequest::DaysAfterActivation; rq.daysAfter = juce::jlimit (1, 8000, daysEd.getText().getIntValue()); }
        rq.seats = seats.getSelectedId(); rq.quantity = juce::jlimit (1, 200, qtyEd.getText().getIntValue()); rq.note = noteEd.getText().toStdString();
        lastBatch = acc.issue (rq);
        juce::String txt; for (auto& k : lastBatch) txt << TX (k.pack.c_str()).paddedRight (' ', 10) << k.key << "\n";
        out.setText (txt, false); svc.toast (juce::String ((int) lastBatch.size()) + TX (" chave(s) gerada(s)."), false); reload();
    }
    void exportCsv (const std::vector<sk::lic::IssuedKey>& keys)
    {
        if (keys.empty()) { svc.toast (TX ("Nada para exportar."), true); return; }
        const auto f = juce::File::getSpecialLocation (juce::File::userDesktopDirectory).getNonexistentChildFile ("SYNTHKEY-chaves", ".csv");
        f.replaceWithText (AccessManager::get().issuedCsv (keys), false, true); svc.toast (TX ("Exportado: ") + f.getFileName() + TX (" (Ambiente de trabalho)"), false);
    }
    void copySelected() { const int r = list.getSelectedRow(); if (r >= 0 && r < (int) rows.size()) { juce::SystemClipboard::copyTextToClipboard (TX (rows[(size_t) r].key.c_str())); svc.toast (TX ("Chave copiada."), false); } }
    void removeSelected() { const int r = list.getSelectedRow(); if (r >= 0 && r < (int) rows.size()) { AccessManager::get().removeIssued (TX (rows[(size_t) r].key.c_str())); reload(); } }
    int getNumRows() override { return (int) rows.size(); }
    void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected) override
    {
        if (row >= (int) rows.size()) return; const auto& k = rows[(size_t) row]; const int today = AccessManager::get().today();
        if (selected) { g.setColour (theme::accent().withAlpha (0.14f)); g.fillRoundedRectangle (0, 1, (float) w, (float) h - 2, 5.0f); }
        const juce::String st = TX (sk::lic::issuedState (k, today, AccessManager::get().isIssuedKeyActivatedHere (k)).c_str());
        juce::Colour sc = st.startsWith ("EXPIRADA") ? theme::bad() : (st.startsWith ("EXPIRA") ? theme::warn() : (st.startsWith ("ATIVA") ? theme::good() : theme::dim()));
        g.setFont (theme::font (12.0f)); g.setColour (theme::dim()); g.drawText (sk::lic::formatDay (k.createdDay), 8, 0, 80, h, juce::Justification::centredLeft);
        g.setColour (theme::text()); g.drawText (TX (k.pack.c_str()), 92, 0, 70, h, juce::Justification::centredLeft);
        g.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 12.5f, juce::Font::plain)); g.setColour (theme::accent()); g.drawText (TX (k.key.c_str()), 164, 0, 190, h, juce::Justification::centredLeft);
        g.setFont (theme::font (11.0f, true)); g.setColour (sc); g.drawText (st, 356, 0, 130, h, juce::Justification::centredLeft);
        g.setFont (theme::font (11.5f)); g.setColour (theme::dim()); g.drawText (TX (k.note.c_str()), 484, 0, w - 490, h, juce::Justification::centredLeft);
    }
    Services& svc; juce::OwnedArray<juce::ToggleButton> packToggles; juce::ComboBox validity, seats; juce::TextEditor dateEd, daysEd, qtyEd, noteEd, out;
    juce::TextButton gen, copyOut, csvOut, copyKey, delKey, csvAll; juce::ToggleButton sim; juce::ListBox list;
    std::vector<sk::lic::IssuedKey> rows, lastBatch; int ticks = 0;
};
} // namespace ui
