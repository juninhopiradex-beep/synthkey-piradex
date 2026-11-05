// =============================================================================
//  SYNTHKEY by Piradex — atualizações automáticas dos packs instalados
//
//  • Ao abrir o plugin (no máximo 1×/24 h) e quando se carrega em "VERIFICAR", descarrega o manifesto (Core/PackUpdate.h).
//  • Só propõe packs JÁ instalados com versão mais recente; packs novos aparecem como novidade mas nunca se instalam sozinhos.
//  • Instalação (manual ou automática, se ativada): download para um ficheiro temporário, verificação do SHA-256 do manifesto
//    e depois PackHub::install(), que ainda valida a assinatura (HMAC) do pack. Falha de rede/validação = nada é alterado.
//  • O endereço do manifesto é configurável (ficheiro atualizacoes.txt na pasta do utilizador, linha url=…).
//  • Sem ligação, o plugin funciona normalmente: nenhuma funcionalidade depende desta verificação.
// =============================================================================
#pragma once
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <atomic>
#include <functional>
#include "AppPaths.h"
#include "PackHub.h"
#include "Core/PackUpdate.h"
#include "Core/Crypto.h"

class PackUpdater : public juce::ChangeBroadcaster
{
public:
    static PackUpdater& get() { static PackUpdater u; return u; }
    static constexpr const char* kDefaultManifestUrl = "https://raw.githubusercontent.com/juninhopiradex-beep/synthkey-piradex/main/docs/packs-manifest.json";

    struct Status { enum State { Idle, Checking, Ready, Downloading, Error } state = Idle; std::string message; std::vector<sk::update::Pending> pending; int percent = 0; std::string busyId; };
    Status status() const { const juce::ScopedLock sl (lock); return st; }

    std::string manifestUrl() const { const auto v = readSetting ("url"); return v.empty() ? std::string (kDefaultManifestUrl) : v; }
    bool autoInstall() const { return readSetting ("auto") == "1"; }
    void setAutoInstall (bool on) { writeSetting ("auto", on ? "1" : "0"); }
    bool autoCheck() const { return readSetting ("check") != "0"; }
    void setAutoCheck (bool on) { writeSetting ("check", on ? "1" : "0"); }

    // chamar uma vez ao abrir a interface: verifica se passaram 24 h desde a última verificação
    void startupCheck()
    {
        if (started.exchange (true) || ! autoCheck()) return;
        const auto last = (juce::int64) std::atoll (readSetting ("last").c_str()); const auto now = juce::Time::currentTimeMillis();
        if (now - last < 24LL * 3600 * 1000) return;
        checkNow (true);
    }
    void checkNow (bool silent)
    {
        { const juce::ScopedLock sl (lock); if (st.state == Status::Checking || st.state == Status::Downloading) return; st.state = Status::Checking; st.message = "A verificar…"; }
        notify();
        const std::string url = manifestUrl();
        juce::Thread::launch ([this, url, silent]
        {
            std::string text, err; Status out; out.state = Status::Error;
            if (! httpGet (url, text, err)) out.message = err;
            else
            {
                sk::update::Manifest m; int skipped = 0;
                if (! sk::update::parseManifest (text, m, &err, &skipped)) out.message = err;
                else
                {
                    std::map<std::string, std::string> inst; for (auto& e : PackHub::get().entries()) if (e.installed && ! e.core) inst[e.id] = e.version;
                    out.pending = sk::update::pending (m, inst); out.state = Status::Ready;
                    int upd = 0, news = 0; for (auto& p : out.pending) (p.isNew ? news : upd)++;
                    out.message = upd == 0 ? (news ? std::to_string (news) + " pack(s) novo(s) disponível(is)" : "Tudo atualizado") : std::to_string (upd) + " atualização(ões) disponível(is)";
                }
            }
            if (silent && out.state == Status::Error) { out.state = Status::Idle; out.message.clear(); }      // verificação automática falhada: sem alarme vermelho
            if (out.state == Status::Ready) writeSetting ("last", std::to_string ((long long) juce::Time::currentTimeMillis()));
            { const juce::ScopedLock sl (lock); st = out; }
            notify();
            if (out.state == Status::Ready && autoInstall()) for (auto& p : out.pending) if (! p.isNew) { installPending (p); break; }   // as seguintes entram em cadeia
        });
    }
    void installPending (const sk::update::Pending& p)
    {
        { const juce::ScopedLock sl (lock); if (st.state == Status::Downloading) return; st.state = Status::Downloading; st.busyId = p.remote.id; st.percent = 0; st.message = "A descarregar " + p.remote.name + "…"; }
        notify();
        const auto remote = p.remote;
        juce::Thread::launch ([this, remote]
        {
            std::string err; const auto tmp = juce::File::createTempFile (".bfpack");
            const bool ok = download (remote, tmp, err);
            juce::MessageManager::callAsync ([this, remote, tmp, ok, err]
            {
                std::string msg = err;
                if (ok) { const auto e = PackHub::get().install (tmp); if (e.isNotEmpty()) msg = e.toStdString(); else msg.clear(); }
                tmp.deleteFile();
                { const juce::ScopedLock sl (lock);
                  if (msg.empty()) { st.pending.erase (std::remove_if (st.pending.begin(), st.pending.end(), [&] (const sk::update::Pending& q) { return q.remote.id == remote.id; }), st.pending.end()); st.message = remote.name + " atualizado para " + remote.version; }
                  else st.message = "Falhou: " + msg;
                  st.state = Status::Ready; st.busyId.clear(); st.percent = 0; }
                notify();
                if (msg.empty() && autoInstall()) { Status s = status(); for (auto& q : s.pending) if (! q.isNew) { installPending (q); break; } }
            });
        });
    }
    void cancel() { cancelFlag = true; }

private:
    PackUpdater() = default;
    mutable juce::CriticalSection lock; Status st; std::atomic<bool> started { false }, cancelFlag { false };
    void notify() { juce::MessageManager::callAsync ([this] { sendChangeMessage(); }); }
    static juce::File settingsPath() { return skpaths::userDir().getChildFile ("atualizacoes.txt"); }
    std::string readSetting (const std::string& key) const
    {
        juce::StringArray l; l.addLines (settingsPath().loadFileAsString());
        for (auto& s : l) if (s.startsWith (juce::String (key) + "=")) return s.fromFirstOccurrenceOf ("=", false, false).trim().toStdString();
        return {};
    }
    void writeSetting (const std::string& key, const std::string& val)
    {
        const juce::ScopedLock sl (lock);
        juce::StringArray l, out; l.addLines (settingsPath().loadFileAsString()); bool done = false;
        for (auto& s : l) { if (s.startsWith (juce::String (key) + "=")) { out.add (juce::String (key) + "=" + val); done = true; } else if (s.trim().isNotEmpty()) out.add (s); }
        if (! done) out.add (juce::String (key) + "=" + val);
        settingsPath().replaceWithText (out.joinIntoString ("\n") + "\n");
    }
    static std::unique_ptr<juce::InputStream> open (const std::string& url)
    {
        juce::URL u { juce::String (url) };
        return u.createInputStream (juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress).withConnectionTimeoutMs (10000).withNumRedirectsToFollow (5));
    }
    static bool httpGet (const std::string& url, std::string& text, std::string& err)
    {
        if (! sk::update::isHttps (url)) { err = "O endereço do manifesto tem de ser https."; return false; }
        auto s = open (url); if (! s) { err = "Sem ligação ao servidor de atualizações."; return false; }
        char buf[8192]; while (! s->isExhausted()) { const int n = s->read (buf, sizeof buf); if (n <= 0) break; text.append (buf, (size_t) n); if (text.size() > 4u * 1024 * 1024) { err = "Manifesto demasiado grande."; return false; } }
        if (text.empty()) { err = "Resposta vazia do servidor."; return false; }
        return true;
    }
    bool download (const sk::update::Remote& r, const juce::File& dst, std::string& err)
    {
        cancelFlag = false;
        auto s = open (r.url); if (! s) { err = "Sem ligação para descarregar o pack."; return false; }
        const juce::int64 total = s->getTotalLength() > 0 ? s->getTotalLength() : (r.size > 0 ? r.size : 0);
        if (total > 2LL * 1024 * 1024 * 1024) { err = "Pack demasiado grande."; return false; }
        juce::FileOutputStream out (dst); if (out.failedToOpen()) { err = "Não foi possível criar o ficheiro temporário."; return false; }
        out.setPosition (0); out.truncate();
        sk::crypto::Sha256 h; juce::int64 got = 0; std::vector<char> buf (1 << 16); int lastPct = -1;
        while (! s->isExhausted())
        {
            if (cancelFlag) { err = "Cancelado."; return false; }
            const int n = s->read (buf.data(), (int) buf.size()); if (n < 0) { err = "A ligação foi interrompida."; return false; } if (n == 0) break;
            h.update (buf.data(), (size_t) n); if (! out.write (buf.data(), (size_t) n)) { err = "Sem espaço em disco."; return false; }
            got += n; if (got > 2LL * 1024 * 1024 * 1024) { err = "Pack demasiado grande."; return false; }
            if (total > 0) { const int pct = (int) (100 * got / total); if (pct != lastPct) { lastPct = pct; { const juce::ScopedLock sl (lock); st.percent = pct; } notify(); } }
        }
        out.flush();
        if (r.size > 0 && got != r.size) { err = "Tamanho do ficheiro diferente do manifesto."; return false; }
        if (sk::crypto::toHex (h.finish()) != r.sha256) { err = "SHA-256 não confere: ficheiro rejeitado."; return false; }
        return true;
    }
};
