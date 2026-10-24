// =============================================================================
//  SYNTHKEY by Piradex — loops sincronizados: thread de fundo que ajusta cada loop ao andamento do projeto
//
//  O motor toca o loop pela posição do projeto (compasso/tempo), por isso fica SEMPRE no sítio, mesmo
//  antes de haver versão esticada (nessa altura soa como um disco que acelera/abranda). Esta thread cria
//  a versão esticada sem mudar a altura (WSOLA) e o motor passa a usá-la assim que está pronta.
// =============================================================================
#pragma once
#include "Samples.h"
#include "TimeStretch.h"
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace sk
{
class LoopStretcher
{
public:
    static LoopStretcher& get() { static LoopStretcher s; return s; }

    // regista (idempotente) todos os loops de uma biblioteca; chamar depois de a biblioteca estar completa
    void registerLibrary (Library& lib)
    {
        std::vector<std::shared_ptr<SampleData>> found;
        for (auto& in : lib.inst)
            for (auto& z : in.zones)
            {
                if (z.loopBeats <= 0 || z.data < 0 || z.data >= (int) lib.data.size()) continue;
                auto& sd = lib.data[(size_t) z.data]; if (! sd) continue;
                if (! sd->sync) { sd->sync = std::make_shared<LoopSync>(); sd->sync->srcBpm = z.loopBpm; sd->sync->beats = z.loopBeats; }
                found.push_back (sd);
            }
        if (found.empty()) return;
        { std::lock_guard<std::mutex> g (m); for (auto& s : found) { bool have = false; for (auto& w : reg) if (w.lock() == s) { have = true; break; } if (! have) reg.push_back (s); } startLocked(); }
    }
    // marca os loops de um instrumento como "em uso" (esticam-se logo, antes da primeira nota)
    static void activate (const Library& lib, const Instrument& in, float hostBpm)
    {
        for (auto& z : in.zones) if (z.loopBeats > 0 && z.data >= 0 && z.data < (int) lib.data.size() && lib.data[(size_t) z.data] && lib.data[(size_t) z.data]->sync)
        { auto& sy = *lib.data[(size_t) z.data]->sync; sy.want.store (hostBpm, std::memory_order_relaxed); sy.active.store (true, std::memory_order_relaxed); }
    }
    // os loops do instrumento já estão esticados para 'bpm' (ou já estão nesse andamento)?
    static bool ready (const Library& lib, const Instrument& in, float bpm)
    {
        for (auto& z : in.zones) if (z.loopBeats > 0 && z.data >= 0 && z.data < (int) lib.data.size() && lib.data[(size_t) z.data] && lib.data[(size_t) z.data]->sync)
        {
            auto& sy = *lib.data[(size_t) z.data]->sync; const auto* c = sy.cur.load (std::memory_order_acquire);
            const float have = c ? c->bpm : sy.srcBpm; if (std::fabs (have - bpm) > 0.0015f * bpm) return false;
        }
        return true;
    }
    // exportação offline: pede o esticamento e espera (até 'ms') que fique pronto
    static bool waitReady (const Library& lib, const Instrument& in, float bpm, int ms)
    {
        activate (lib, in, bpm); const auto t0 = std::chrono::steady_clock::now();
        while (! ready (lib, in, bpm)) { if (std::chrono::steady_clock::now() - t0 > std::chrono::milliseconds (ms)) return false; std::this_thread::sleep_for (std::chrono::milliseconds (15)); }
        return true;
    }
    // para testes: espera até não haver trabalho pendente
    bool waitIdle (int ms)
    {
        const auto t0 = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - t0 < std::chrono::milliseconds (ms)) { if (idleFlag.load()) return true; std::this_thread::sleep_for (std::chrono::milliseconds (20)); }
        return idleFlag.load();
    }
    void setDebounceMs (int ms) { debounceMs = ms; }
    ~LoopStretcher() { { std::lock_guard<std::mutex> g (m); quit = true; } cv.notify_all(); if (th.joinable()) th.join(); }

private:
    LoopStretcher() = default;
    void startLocked() { if (! started) { started = true; th = std::thread ([this] { run(); }); } }
    struct Retired { std::unique_ptr<StretchedLoop> p; std::chrono::steady_clock::time_point at; };

    void run()
    {
        struct Seen { const SampleData* sd; float want; int stable; };
        std::vector<Seen> seen; std::vector<Retired> retired;
        for (;;)
        {
            std::vector<std::shared_ptr<SampleData>> live;
            {
                std::unique_lock<std::mutex> g (m);
                if (cv.wait_for (g, std::chrono::milliseconds (60), [this] { return quit; })) return;
                for (size_t i = 0; i < reg.size();) { auto s = reg[i].lock(); if (! s) { reg.erase (reg.begin() + (long) i); continue; } live.push_back (std::move (s)); ++i; }
            }
            const auto now = std::chrono::steady_clock::now();
            for (size_t i = 0; i < retired.size();) { if (now - retired[i].at > std::chrono::seconds (4)) retired.erase (retired.begin() + (long) i); else ++i; }   // o áudio já não usa
            bool busy = false;
            for (auto& sd : live)
            {
                auto& sy = *sd->sync; if (! sy.active.load (std::memory_order_relaxed)) continue;
                const float want = sy.want.load (std::memory_order_relaxed); if (want < 20.0f || sy.beats <= 0 || sd->frames() < 64) continue;
                const float have = sy.owned ? sy.owned->bpm : sy.srcBpm;
                if (std::fabs (want - have) <= 0.0015f * have) { continue; }
                // andamento estável durante o tempo de espera (automação de tempo não dispara trabalho a cada passo)
                Seen* sn = nullptr; for (auto& x : seen) if (x.sd == sd.get()) { sn = &x; break; }
                if (! sn) { seen.push_back ({ sd.get(), want, 0 }); sn = &seen.back(); }
                if (std::fabs (sn->want - want) > 0.0005f * want) { sn->want = want; sn->stable = 0; busy = true; continue; }
                if (++sn->stable * 60 < debounceMs) { busy = true; continue; }
                busy = true; sn->stable = 0;
                // ---- esticar para o andamento pedido
                const int n = sd->frames(); const double sr = sd->sampleRate;
                const int outLen = std::max (64, (int) std::lround ((double) sy.beats * 60.0 / (double) want * sr));
                std::vector<const float*> ch { sd->l.data() }; if (sd->stereo()) ch.push_back (sd->r.data());
                std::vector<std::vector<float>> out; tstretch::stretchCircular (ch, n, sr, outLen, out, (int) std::lround (sy.beats * 4.0f));
                auto fresh = std::make_unique<StretchedLoop>(); fresh->bpm = want; fresh->l = std::move (out[0]); if (out.size() > 1) fresh->r = std::move (out[1]);
                const StretchedLoop* raw = fresh.get();
                auto old = std::move (sy.owned); sy.owned = std::move (fresh); sy.cur.store (raw, std::memory_order_release);
                if (old) retired.push_back ({ std::move (old), std::chrono::steady_clock::now() });
            }
            idleFlag.store (! busy);
        }
    }

    std::mutex m; std::condition_variable cv; std::vector<std::weak_ptr<SampleData>> reg; std::thread th; bool started = false, quit = false;
    std::atomic<bool> idleFlag { true }; int debounceMs = 180;
};
}
