#include "PluginProcessor.h"
#include "UserAudio.h"
#include "PluginEditor.h"
#include <random>

using namespace sk;

// ------------------------------------------------------------------------------------------------
//  Parâmetros: gerados a partir da tabela única (Core/ParamTable.h)
// ------------------------------------------------------------------------------------------------
static juce::String U (const char* s) { return juce::String::fromUTF8 (s); }

static juce::StringArray choicesFor (const ParamInfo& pi)
{
    const juce::String c (pi.choices);
    juce::StringArray a;
    if (c == "@MODSRC") { for (auto* n : kModSrcNames) a.add (U (n)); return a; }
    if (c == "@MODDST") { for (auto* n : kModDstNames) a.add (U (n)); return a; }
    a.addTokens (U (pi.choices), "|", ""); return a;
}

static juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (int i = 0; i < P_COUNT; ++i)
    {
        if (i == P_A_SMP || i == P_B_SMP) continue;           // índice do instrumento: guardado pelo id (texto), não é automatizável
        const auto& pi = kParams[i]; const juce::ParameterID pid (pi.id, 1); const auto name = U (pi.name);
        switch (pi.kind)
        {
            case PK_FLOAT:
                layout.add (std::make_unique<juce::AudioParameterFloat> (pid, name, juce::NormalisableRange<float> (pi.mn, pi.mx), pi.def,
                            juce::AudioParameterFloatAttributes().withLabel (pi.unit))); break;
            case PK_LOG:
            {
                juce::NormalisableRange<float> r (pi.mn, pi.mx,
                    [] (float s, float e, float v) { return s * std::pow (e / s, v); },
                    [] (float s, float e, float v) { return std::log (std::max (v, s) / s) / std::log (e / s); });
                layout.add (std::make_unique<juce::AudioParameterFloat> (pid, name, r, pi.def, juce::AudioParameterFloatAttributes().withLabel (pi.unit))); break;
            }
            case PK_INT:
                layout.add (std::make_unique<juce::AudioParameterInt> (pid, name, (int) pi.mn, (int) pi.mx, (int) pi.def, juce::AudioParameterIntAttributes().withLabel (pi.unit))); break;
            case PK_BOOL:
                layout.add (std::make_unique<juce::AudioParameterBool> (pid, name, pi.def > 0.5f)); break;
            case PK_CHOICE:
                layout.add (std::make_unique<juce::AudioParameterChoice> (pid, name, choicesFor (pi), (int) pi.def)); break;
        }
    }
    return layout;
}

SynthKeyProcessor::SynthKeyProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "SYNTHKEY", createLayout()),
      engine (new Engine())
{
    for (int i = 0; i < P_COUNT; ++i)
    {
        if (i == P_A_SMP || i == P_B_SMP) continue;
        params[(size_t) i] = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter (kParams[i].id));
        raw[(size_t) i] = apvts.getRawParameterValue (kParams[i].id);
    }
    for (int i = 0; i < P_COUNT; ++i) prm[i] = kParams[i].def;
    engine->setParams (prm); midiFx.setParams (prm);

    auto& hub = PackHub::get();
    hub.addChangeListener (this);
    hub.ensureLoaded ("nucleo");
    rebuildLibrary();
    refreshPresetList();
    if (! presets.empty()) loadPreset (presets.front(), 0);
    startTimer (1000);
}

SynthKeyProcessor::~SynthKeyProcessor()
{
    stopTimer();
    PackHub::get().removeChangeListener (this);
}

juce::AudioProcessorEditor* SynthKeyProcessor::createEditor() { return new SynthKeyEditor (*this); }

void SynthKeyProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    grooveEvs.reserve (1024); groove.stop();
    sr = sampleRate; engineMax = std::max (512, samplesPerBlock); uiMidi.reset (sampleRate);
    engine->setQuality (raw[(size_t) P_QUALITY] ? (int) raw[(size_t) P_QUALITY]->load() : 0);
    engine->prepare (sampleRate, engineMax); midiFx.prepare ((float) sampleRate);
    engineLib.store (nullptr);                               // força a re-ligação da biblioteca no próximo bloco
    { const juce::ScopedLock sl (libLock); pendingLib.store (combined.get()); }
    setLatencySamples (engine->latencySamples());
    evs.reserve (512);
}

// ------------------------------------------------------------------------------------------------
//  Áudio
// ------------------------------------------------------------------------------------------------
void SynthKeyProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    keyboardState.processNextMidiBuffer (midi, 0, n, true);
    uiMidi.removeNextBlockOfMessages (midi, n);
    if (! AccessManager::get().isUnlocked()) { midi.clear(); voiceCount.store (0); return; }

    for (int i = 0; i < P_COUNT; ++i) prm[i] = raw[(size_t) i] ? raw[(size_t) i]->load() : kParams[i].def;
    prm[P_A_SMP] = (float) smpIdx[0].load(); prm[P_B_SMP] = (float) smpIdx[1].load();

    { const int q = (int) prm[P_QUALITY]; if (q != engine->quality()) { engine->setQuality (q); midiFx.allNotesOff(); engineLib.store (nullptr); { const juce::ScopedLock sl (libLock); pendingLib.store (combined.get()); } } }
    if (auto* pl = pendingLib.exchange (nullptr)) { engine->setLibrary (pl); engineLib.store (pl); midiFx.allNotesOff(); }

    double bpm = 120.0, ppq = 0.0; bool playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto p = pos->getPpqPosition()) ppq = *p;
            playing = pos->getIsPlaying();
        }
    engine->setBpm (bpm);

    // ---- grooves internos: as notas entram no mesmo caminho do MIDI externo (escala / acordes / arpejador)
    grooveEvs.clear(); groove.process (n, sr, grooveEvs);
    for (const auto& ge : grooveEvs)
        midi.addEvent (ge.on ? juce::MidiMessage::noteOn (1, ge.note, (juce::uint8) ge.vel) : juce::MidiMessage::noteOff (1, ge.note), ge.offset);

    // ---- MIDI de entrada -> MidiFx (escala / acordes / arpejador)
    float pitchBend = 0, aftertouch = 0; bool gotPb = false, gotAt = false;
    const bool mpe = prm[P_MPE_ON] > 0.5f; int releasedNotes[32]; int nReleased = 0;
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage(); const int pos = juce::jlimit (0, n - 1, meta.samplePosition);
        const int ch = m.getChannel(); const bool memberCh = mpe && ch >= 2;        // MPE: canal 1 = mestre, 2-16 = uma nota por canal
        if (m.isNoteOn()) { midiFx.noteOn (m.getNoteNumber(), m.getFloatVelocity(), pos); if (memberCh) chanNote[ch] = m.getNoteNumber(); }
        else if (m.isNoteOff()) { midiFx.noteOff (m.getNoteNumber(), pos); if (memberCh && chanNote[ch] == m.getNoteNumber()) chanNote[ch] = -1; if (nReleased < 32) releasedNotes[nReleased++] = m.getNoteNumber(); }
        else if (m.isAllNotesOff() || m.isAllSoundOff()) midiFx.allNotesOff();
        else if (m.isController())
        {
            if (memberCh && m.getControllerNumber() == 74) { if (chanNote[ch] >= 0) engine->setNoteSlide (chanNote[ch], (float) m.getControllerValue() / 127.0f); }
            else if (m.getControllerNumber() == 1) modWheel = (float) m.getControllerValue() / 127.0f;
            else if (m.getControllerNumber() == 64) sustainPedal = m.getControllerValue() >= 64 ? 1.0f : 0.0f;
        }
        else if (m.isPitchWheel())
        {
            const float pbv = (float) (m.getPitchWheelValue() - 8192) / 8192.0f;
            if (memberCh) { if (chanNote[ch] >= 0) engine->setNoteBend (chanNote[ch], pbv); } else { pitchBend = pbv; gotPb = true; }
        }
        else if (m.isChannelPressure())
        {
            const float pv = (float) m.getChannelPressureValue() / 127.0f;
            if (memberCh) { if (chanNote[ch] >= 0) engine->setNotePressure (chanNote[ch], pv); } else { aftertouch = pv; gotAt = true; }
        }
        else if (m.isAftertouch()) { engine->setNotePressure (m.getNoteNumber(), (float) m.getAfterTouchValue() / 127.0f); }   // aftertouch polifónico
    }
    midi.clear();
    if (gotPb) engine->setPitchBend (pitchBend);
    if (gotAt) engine->setAftertouch (aftertouch);
    engine->setModWheel (modWheel); engine->setSustain (sustainPedal > 0.5f);

    midiFx.setParams (prm); engine->setParams (prm);
    midiFx.process (n, bpm, ppq, playing, evs);

    float* L = buffer.getWritePointer (0); float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : buffer.getWritePointer (0);
    int pos = 0;
    auto renderTo = [&] (int end)
    {
        while (pos < end) { const int c = std::min (end - pos, engineMax); engine->render (L + pos, R + pos, c); pos += c; }
    };
    for (const auto& e : evs)
    {
        renderTo (juce::jlimit (pos, n, e.offset));
        if (e.on) engine->noteOn (e.note, e.vel); else engine->noteOff (e.note);
    }
    renderTo (n);
    for (int i = 0; i < nReleased; ++i) { engine->setNotePressure (releasedNotes[i], 0.0f); engine->setNoteSlide (releasedNotes[i], 0.0f); if (mpe) engine->setNoteBend (releasedNotes[i], 0.0f); }
    if (buffer.getNumChannels() > 1) { /* R já escrito */ } else { /* mono: L contém o canal esquerdo */ }

    // ---- pré-visualização de packs bloqueados: 15 s de áudio acumulado, depois silêncio
    if (previewMode.load())
    {
        if (engine->activeVoices() > 0) previewLeftSamples.fetch_sub (n);
        const bool expired = previewLeftSamples.load() <= 0;
        previewExpired.store (expired);
        const float target = expired ? 0.0f : 1.0f; const float step = 1.0f / (0.05f * (float) sr);
        for (int i = 0; i < n; ++i)
        {
            if (previewGain < target) previewGain = std::min (target, previewGain + step); else if (previewGain > target) previewGain = std::max (target, previewGain - step);
            L[i] *= previewGain; R[i] *= previewGain;
        }
        previewSecondsLeft.store (std::max (0, previewLeftSamples.load()) / (float) sr);
    }
    else { previewGain = 1.0f; previewExpired.store (false); }

    {   // alimenta o osciloscópio / espectro
        int w = scopeWrite.load (std::memory_order_relaxed);
        for (int i = 0; i < n; ++i) { scopeBuf[w] = 0.5f * (L[i] + R[i]); w = (w + 1) & (kScopeSize - 1); }
        scopeWrite.store (w, std::memory_order_release);
    }
    meterL.store (engine->getPeakL()); meterR.store (engine->getPeakR()); voiceCount.store (engine->activeVoices());
}

void SynthKeyProcessor::setReal (int idx, float v)
{
    if (auto* p = params[(size_t) idx]) p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit (kParams[idx].mn, kParams[idx].mx, v)));
}

// ------------------------------------------------------------------------------------------------
//  Bibliotecas e samples
// ------------------------------------------------------------------------------------------------
void SynthKeyProcessor::changeListenerCallback (juce::ChangeBroadcaster*) { rebuildLibrary(); refreshPresetList(); }

void SynthKeyProcessor::rebuildLibrary()
{
    auto& hub = PackHub::get();
    auto lib = std::make_shared<Library>();
    for (auto& e : hub.entries()) if (auto l = hub.library (e.id)) lib->appendShared (*l);
    if (auto ul = hub.userLibrary()) lib->appendShared (*ul);          // samples importados pelo utilizador
    {
        const juce::ScopedLock sl (libLock);
        retired.push_back (combined); combined = lib;
        pendingLib.store (combined.get());
        // liberta bibliotecas antigas que o motor já não usa
        retired.erase (std::remove_if (retired.begin(), retired.end(), [this] (const std::shared_ptr<const Library>& r) { return ! r || (r.get() != engineLib.load() && r.get() != pendingLib.load()); }), retired.end());
    }
    resolveSamples();
}

std::shared_ptr<const Library> SynthKeyProcessor::currentLibrary() const { const juce::ScopedLock sl (libLock); return combined; }

std::string SynthKeyProcessor::fallbackFor (const std::string& id)
{
    const auto dot = id.find ('.'); const std::string s = dot == std::string::npos ? id : id.substr (dot + 1);
    auto has = [&] (const char* k) { return s.find (k) != std::string::npos; };
    if (s == "kit") return "nucleo.kit";
    for (const char* d : { "kick", "snare", "clap", "hat", "ohat", "rim", "tom", "shaker", "riser", "impact" }) if (s == d) return std::string ("nucleo.") + d;
    if (has ("808") || has ("logdrum") || has ("logshort")) return "nucleo.808";
    if (has ("bass")) return "nucleo.sub";
    if (has ("pad") || has ("strings")) return "nucleo.pad";
    if (has ("vox") || has ("chop") || has ("shout")) return "nucleo.vox";
    if (has ("ep") || has ("rhodes") || has ("wurli") || has ("piano") || has ("keys")) return "nucleo.ep";
    if (has ("bell") || has ("kalimba")) return "nucleo.bell";
    if (has ("stab") || has ("horn")) return "nucleo.stab";
    if (has ("shaker") || has ("guiro") || has ("conga") || has ("bongo") || has ("talking")) return "nucleo.shaker";
    return "nucleo.pluck";
}

void SynthKeyProcessor::resolveSamples()
{
    std::shared_ptr<const Library> lib; juce::String ids[2];
    { const juce::ScopedLock sl (libLock); lib = combined; ids[0] = sampleId[0]; ids[1] = sampleId[1]; }
    for (int e = 0; e < 2; ++e)
    {
        int idx = -1; bool miss = false;
        if (ids[e].isNotEmpty() && lib)
        {
            idx = lib->indexOf (ids[e].toStdString());
            if (idx < 0) { miss = true; idx = lib->indexOf (fallbackFor (ids[e].toStdString())); }
        }
        if (idx < 0 && lib && ! lib->inst.empty()) idx = 0;
        smpIdx[e].store (std::max (0, idx)); missing[e].store (miss && ids[e].isNotEmpty());
    }
    updatePreviewState();
}

juce::String SynthKeyProcessor::getSampleId (int e) const { const juce::ScopedLock sl (libLock); return sampleId[e]; }

void SynthKeyProcessor::setSampleId (int e, const juce::String& id)
{
    { const juce::ScopedLock sl (libLock); sampleId[e] = id; }
    apvts.state.setProperty (e == 0 ? "sampleA" : "sampleB", id, nullptr);
    auto& hub = PackHub::get();
    if (id.isNotEmpty())
    {
        const auto pack = PackHub::packOfInstrument (id.toStdString());
        PackEntry pe; if (hub.entry (pack, pe) && pe.installed) hub.ensureLoaded (pack);
    }
    resolveSamples();
}

juce::String SynthKeyProcessor::sampleDisplayName (int e) const
{
    const auto id = getSampleId (e).toStdString(); if (id.empty()) return juce::CharPointer_UTF8 ("\xE2\x80\x94");
    PackEntry pe; if (PackHub::get().entry (PackHub::packOfInstrument (id), pe))
        for (auto& i : pe.insts) if (i.id == id) return U (i.name.c_str());
    if (PackHub::packOfInstrument (id) == "user")
        for (auto& i : PackHub::get().userInstruments()) if (i.id == id) return U (i.name.c_str());
    return juce::String (id);
}

void SynthKeyProcessor::updatePreviewState()
{
    bool locked = false;
    for (int e = 0; e < 2; ++e)
    {
        if (getReal (e == 0 ? P_A_ON : P_B_ON) < 0.5f || (int) getReal (e == 0 ? P_A_TYPE : P_B_TYPE) != 2) continue;
        const auto id = getSampleId (e).toStdString(); if (id.empty() || sampleMissing (e)) continue;
        const auto pack = PackHub::packOfInstrument (id); PackEntry pe;
        if (PackHub::get().entry (pack, pe) && ! AccessManager::get().packStatus (pack, pe.core).usable()) locked = true;
    }
    if (locked && ! previewMode.load()) previewLeftSamples.store ((int) (15.0 * sr));
    previewMode.store (locked);
}

// ------------------------------------------------------------------------------------------------
//  Desfazer / refazer
// ------------------------------------------------------------------------------------------------
SynthKeyProcessor::Snapshot SynthKeyProcessor::takeSnapshot() const
{
    Snapshot s; s.v.resize ((size_t) P_COUNT);
    for (int i = 0; i < P_COUNT; ++i) s.v[(size_t) i] = (i == P_A_SMP || i == P_B_SMP) ? 0.0f : getReal (i);
    s.a = getSampleId (0); s.b = getSampleId (1); return s;
}
bool SynthKeyProcessor::sameSnapshot (const Snapshot& x, const Snapshot& y)
{
    if (x.a != y.a || x.b != y.b || x.v.size() != y.v.size()) return false;
    for (size_t i = 0; i < x.v.size(); ++i) if (std::fabs (x.v[i] - y.v[i]) > 1.0e-5f) return false;
    return true;
}
void SynthKeyProcessor::applySnapshot (const Snapshot& s)
{
    for (int i = 0; i < P_COUNT; ++i) if (i != P_A_SMP && i != P_B_SMP && std::fabs (getReal (i) - s.v[(size_t) i]) > 1.0e-5f) setReal (i, s.v[(size_t) i]);
    if (getSampleId (0) != s.a) setSampleId (0, s.a);
    if (getSampleId (1) != s.b) setSampleId (1, s.b);
}
void SynthKeyProcessor::undoWatch()
{
    if (! haveCommitted) { committed = takeSnapshot(); haveCommitted = true; return; }
    const auto cur = takeSnapshot();
    if (sameSnapshot (cur, committed)) { quietTicks = 0; return; }
    if (juce::ModifierKeys::currentModifiers.isAnyMouseButtonDown()) { quietTicks = 0; return; }     // gesto em curso: espera que acabe
    if (++quietTicks < 7) return;
    undoStack.push_back (std::move (committed)); redoStack.clear(); committed = cur; quietTicks = 0;
    if (undoStack.size() > 100) undoStack.erase (undoStack.begin());
}
bool SynthKeyProcessor::undo()
{
    if (! haveCommitted) return false;
    { const auto cur = takeSnapshot(); if (! sameSnapshot (cur, committed)) { undoStack.push_back (committed); committed = cur; redoStack.clear(); } }   // fecha uma alteração ainda pendente
    if (undoStack.empty()) return false;
    redoStack.push_back (takeSnapshot()); const auto target = std::move (undoStack.back()); undoStack.pop_back();
    applySnapshot (target); committed = target; quietTicks = 0; return true;
}
bool SynthKeyProcessor::redo()
{
    if (redoStack.empty()) return false;
    undoStack.push_back (takeSnapshot()); const auto target = std::move (redoStack.back()); redoStack.pop_back();
    applySnapshot (target); committed = target; quietTicks = 0; return true;
}

void SynthKeyProcessor::timerCallback()
{
    AccessManager::get().checkExpiry(); updatePreviewState();
    if (engine && engine->latencySamples() != getLatencySamples()) setLatencySamples (engine->latencySamples());   // muda com a qualidade
}

// ------------------------------------------------------------------------------------------------
//  Presets
// ------------------------------------------------------------------------------------------------
void SynthKeyProcessor::refreshPresetList()
{
    auto fp = PackHub::get().factoryPresets(); auto up = PackHub::get().userPresets();
    // ordem: Núcleo primeiro (já vem assim), depois packs; presets do utilizador no fim
    fp.insert (fp.end(), up.begin(), up.end());
    const juce::String cur = presetName;
    presets = std::move (fp); currentPreset = -1;
    for (size_t i = 0; i < presets.size(); ++i) if (juce::String (presets[i].name) == cur) { currentPreset = (int) i; break; }
}

void SynthKeyProcessor::loadPreset (const Preset& p, int idx)
{
    loadingPreset = true;
    for (int i = 0; i < P_COUNT; ++i) { if (i == P_A_SMP || i == P_B_SMP) continue; setReal (i, p.get (i)); }
    presetName = U (p.name.c_str()); currentPreset = idx;
    apvts.state.setProperty ("presetName", presetName, nullptr);
    setSampleId (0, U (p.sampleA.c_str())); setSampleId (1, U (p.sampleB.c_str()));
    loadingPreset = false;
    updatePreviewState();
    if (getReal (P_A_TYPE) == 2.0f || getReal (P_B_TYPE) == 2.0f) previewLeftSamples.store ((int) (15.0 * sr));
    if (onPresetLoaded) onPresetLoaded();
}

void SynthKeyProcessor::loadPresetIndex (int i) { if (i >= 0 && i < (int) presets.size()) loadPreset (presets[(size_t) i], i); }

void SynthKeyProcessor::stepPreset (int dir)
{
    if (presets.empty()) return;
    const int n = (int) presets.size(); int i = currentPreset < 0 ? (dir > 0 ? 0 : n - 1) : (currentPreset + dir + n) % n;
    loadPresetIndex (i);
}

void SynthKeyProcessor::initPreset()
{
    Preset p; p.name = "Init"; p.pack = "nucleo"; loadPreset (p, -1); presetName = "Init";
}

Preset SynthKeyProcessor::capturePreset (const juce::String& name) const
{
    Preset p; p.name = name.toStdString(); p.author = "Utilizador"; p.pack = "user"; p.style = "user"; p.category = "Utilizador";
    for (int i = 0; i < P_COUNT; ++i)
    {
        if (i == P_A_SMP || i == P_B_SMP) continue;
        const float v = getReal (i); if (std::fabs (v - kParams[i].def) > 1e-5f) p.set (i, v);
    }
    p.sampleA = getSampleId (0).toStdString(); p.sampleB = getSampleId (1).toStdString();
    return p;
}

bool SynthKeyProcessor::saveUserPreset (const juce::String& name, juce::String& error, const juce::StringArray& tags)
{
    juce::String clean = name.trim().retainCharacters ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -_()");
    if (clean.isEmpty()) { error = juce::CharPointer_UTF8 ("Escolha um nome v\xC3\xA1lido (letras, n\xC3\xBAmeros, espa\xC3\xA7os)."); return false; }
    const auto f = PackHub::userPresetFolder().getChildFile (clean + ".skpreset");
    auto pr = capturePreset (clean); for (auto& t : tags) { const auto tt = t.trim().retainCharacters ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -_"); if (tt.isNotEmpty() && pr.tags.size() < 8) pr.tags.push_back (tt.toStdString()); }
    if (! f.replaceWithText (juce::String (presetToJson (pr)))) { error = juce::CharPointer_UTF8 ("N\xC3\xA3o foi poss\xC3\xADvel gravar o preset."); return false; }
    presetName = clean; apvts.state.setProperty ("presetName", presetName, nullptr);
    refreshPresetList(); return true;
}

bool SynthKeyProcessor::presetIsLocked (const Preset& p) const
{
    for (const auto& id : { p.sampleA, p.sampleB })
    {
        if (id.empty()) continue; const auto pack = PackHub::packOfInstrument (id); PackEntry pe;
        if (PackHub::get().entry (pack, pe) && ! AccessManager::get().packStatus (pack, pe.core).usable()) return true;
    }
    return false;
}

void SynthKeyProcessor::randomizePatch (float amount)
{
    static std::mt19937 rng { std::random_device{}() };
    std::uniform_real_distribution<float> u (0.0f, 1.0f), g (-1.0f, 1.0f);
    // só parâmetros "seguros" de timbre; nunca volume, polifonia, modo de voz nem estrutura de FX
    static const int ids[] = { P_A_UNIDET, P_A_PWM, P_A_FMAMT, P_A_WTPOS, P_A_WARP, P_A_FOLD, P_B_UNIDET, P_B_WTPOS, P_B_WARP, P_B_FOLD, P_F1_CUT, P_F1_RES, P_F1_DRIVE, P_F1_ENV,
                               P_AMP_A, P_AMP_D, P_AMP_S, P_AMP_R, P_ENV2_A, P_ENV2_D, P_ENV2_S, P_LFO1_RATE, P_LFO2_RATE, P_BLEND };
    for (int idx : ids)
    {
        auto* p = params[(size_t) idx]; if (! p) continue;
        const float cur = p->getValue(); const float nv = juce::jlimit (0.0f, 1.0f, cur + g (rng) * amount * 0.5f);
        p->setValueNotifyingHost (nv);
    }
}

// ------------------------------------------------------------------------------------------------
//  Estado (projeto do host)
// ------------------------------------------------------------------------------------------------
void SynthKeyProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto st = apvts.copyState();
    st.setProperty ("sampleA", getSampleId (0), nullptr); st.setProperty ("sampleB", getSampleId (1), nullptr); st.setProperty ("presetName", presetName, nullptr);
    st.setProperty ("userWT", encodeUserWavetable(), nullptr);
    if (auto xml = st.createXml()) copyXmlToBinary (*xml, dest);
}

// tabela do utilizador guardada dentro do projeto (kFrames × kSize amostras em int16 → base64)
juce::String SynthKeyProcessor::encodeUserWavetable() const
{
    const auto fr = engine->userWavetable().frames(); if (fr.empty()) return {};
    juce::MemoryBlock mb (fr.size() * 2);
    for (size_t i = 0; i < fr.size(); ++i) { const auto v = (juce::int16) std::lround (juce::jlimit (-1.0f, 1.0f, fr[i]) * 32767.0f); mb.copyFrom (&v, i * 2, 2); }
    return juce::Base64::toBase64 (mb.getData(), mb.getSize());
}
bool SynthKeyProcessor::decodeUserWavetable (const juce::String& b64)
{
    if (b64.isEmpty()) { engine->userWavetable().clear(); return false; }
    juce::MemoryOutputStream out; if (! juce::Base64::convertFromBase64 (out, b64)) return false;
    const size_t n = out.getDataSize() / 2; std::vector<float> fr (n); const auto* d = (const juce::int16*) out.getData();
    for (size_t i = 0; i < n; ++i) fr[i] = (float) d[i] / 32767.0f;
    return engine->userWavetable().setFrames (fr);
}
juce::String SynthKeyProcessor::importWavetable (const juce::File& f)
{
    sk::SampleData sd; const auto err = useraudio::decode (f, sd); if (err.isNotEmpty()) return err;
    const auto mono = useraudio::toMono (sd);
    if (! engine->userWavetable().setSignal (mono.data(), (int) mono.size())) return juce::CharPointer_UTF8 ("Ficheiro demasiado curto para uma wavetable.");
    userWtName = f.getFileNameWithoutExtension(); apvts.state.setProperty ("userWTName", userWtName, nullptr);
    return {};
}

void SynthKeyProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto vt = juce::ValueTree::fromXml (*xml);
            apvts.replaceState (vt);
            presetName = vt.getProperty ("presetName", "Init").toString();
            decodeUserWavetable (vt.getProperty ("userWT", "").toString()); userWtName = vt.getProperty ("userWTName", "").toString();
            setSampleId (0, vt.getProperty ("sampleA", "").toString()); setSampleId (1, vt.getProperty ("sampleB", "").toString());
            refreshPresetList();
            if (onPresetLoaded) onPresetLoaded();
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SynthKeyProcessor(); }
