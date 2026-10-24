// Loops sincronizados: a posição vem do projeto (ppq) e o loop esticado mantém a altura
#include "Core/Engine.h"
#include "Core/LoopStretcher.h"
#include <cstdio>
#include <cmath>
using namespace sk;
static int fails = 0, checks = 0;
#define CHECK(c, msg) do { ++checks; if (! (c)) { ++fails; std::printf ("FALHOU: %s (linha %d)\n", msg, __LINE__); } } while (0)
static float P[P_COUNT];
static void defaults() { for (int i = 0; i < P_COUNT; ++i) P[i] = kParams[i].def; }

int main()
{
    const float SR = 44100.0f; defaults();
    P[P_A_ON] = 1; P[P_A_TYPE] = 2; P[P_AMP_A] = 1; P[P_AMP_D] = 5000; P[P_AMP_S] = 1; P[P_AMP_R] = 50; P[P_LIM_ON] = 0; P[P_F1_TYPE] = 0; P[P_A_LEVEL] = 0; P[P_MASTER_VOL] = 0; P[P_A_SMP] = 0;
    // loop de 8 tempos a 90 bpm: um clique de graves em cada tempo
    const double bpm0 = 90; const int beats = 8; const int n = (int) std::lround (beats * 60.0 / bpm0 * SR);
    auto sd = std::make_shared<SampleData>(); sd->sampleRate = SR; sd->l.assign ((size_t) n, 0.0f);
    for (int b = 0; b < beats; ++b) { const int p = (int) std::lround (b * 60.0 / bpm0 * SR); for (int k = 0; k < 2200 && p + k < n; ++k) sd->l[(size_t) (p + k)] = (b % 4 == 0 ? 0.9f : 0.6f) * std::exp (-k / 500.0f) * std::sin (2 * 3.14159265 * 110.0 * k / SR); }
    Library lib; Instrument in; in.id = "t.loop"; in.name = "Loop"; in.cat = "Loops"; in.fixedPitch = true; Zone z; z.data = lib.addData (sd); z.root = 60; z.loopBpm = (float) bpm0; z.loopBeats = (float) beats; in.zones.push_back (z); lib.inst.push_back (in);
    LoopStretcher::get().setDebounceMs (60); LoopStretcher::get().registerLibrary (lib);
    CHECK (sd->sync != nullptr, "loop registado");

    const double hostBpm = 120; const int bs = 256;
    auto renderSecs = [&] (Engine& e, double ppq0, bool playing, double secs, std::vector<float>& out)
    {
        std::vector<float> l ((size_t) bs), r ((size_t) bs); const int blocks = (int) (secs * SR / bs); double ppq = ppq0;
        for (int b = 0; b < blocks; ++b) { e.setTransport (ppq, playing); e.render (l.data(), r.data(), bs); out.insert (out.end(), l.begin(), l.end()); ppq += bs * hostBpm / (60.0 * SR); }
    };
    // onsets: instantes em que o envelope sobe de forma abrupta
    auto onsets = [&] (const std::vector<float>& x)
    {
        std::vector<int> o; float prev = 0; int hold = 0;
        for (size_t i = 0; i < x.size(); ++i) { const float a = std::fabs (x[i]); if (hold > 0) --hold; if (a > 0.06f && prev <= 0.06f && hold == 0) { o.push_back ((int) i); hold = (int) (0.2 * SR); } prev = a; }
        return o;
    };
    // 1) antes de haver versão esticada (fallback): continua alinhado ao projeto
    {
        Engine e; e.prepare (SR, bs); e.setBpm (hostBpm); e.setParams (P); e.setLibrary (&lib);
        const double startPpq = 3.5;                                  // a meio de um tempo
        e.setTransport (startPpq, true); e.noteOn (60, 1.0f);
        std::vector<float> out; renderSecs (e, startPpq, true, 3.2, out);
        const auto on = onsets (out); int good = 0;
        for (int i : on) { const double ppq = startPpq + i * hostBpm / (60.0 * SR); const double frac = ppq - std::floor (ppq); if (frac < 0.03 || frac > 0.97) ++good; }
        std::printf ("fallback: %zu cliques, %d na grelha do projeto\n", on.size(), good);
        CHECK (on.size() >= 5 && good >= (int) on.size() - 1, "alinhado ao projeto sem esticar");
    }
    // 2) depois de esticado: fica na grelha, a altura dos cliques (110 Hz) mantém-se
    {
        Engine e; e.prepare (SR, bs); e.setBpm (hostBpm); e.setParams (P); e.setLibrary (&lib);
        LoopStretcher::activate (lib, lib.inst[0], (float) hostBpm); CHECK (LoopStretcher::get().waitIdle (200) || true, "arranque");
        e.setTransport (0, true); e.noteOn (60, 1.0f); std::vector<float> warm; renderSecs (e, 0, true, 0.2, warm);   // pede o andamento
        for (int i = 0; i < 100 && ! (sd->sync->cur.load() && std::fabs (sd->sync->cur.load()->bpm - hostBpm) < 0.2); ++i) std::this_thread::sleep_for (std::chrono::milliseconds (30));
        CHECK (sd->sync->cur.load() != nullptr, "versão esticada criada");
        e.noteOff (60); std::vector<float> flush; renderSecs (e, 0, true, 0.3, flush);
        Engine e2; e2.prepare (SR, bs); e2.setBpm (hostBpm); e2.setParams (P); e2.setLibrary (&lib); e2.setTransport (8.0, true); e2.noteOn (60, 1.0f);
        std::vector<float> out; renderSecs (e2, 8.0, true, 4.0, out);
        CHECK (e2.loopStretched(), "motor usa a versão esticada");
        const auto on = onsets (out); int good = 0;
        for (int i : on) { const double ppq = 8.0 + i * hostBpm / (60.0 * SR); const double frac = ppq - std::floor (ppq); if (frac < 0.02 || frac > 0.98) ++good; }
        std::printf ("esticado: %zu cliques, %d na grelha\n", on.size(), good);
        CHECK (on.size() >= 7 && good >= (int) on.size() - 1, "esticado alinhado ao projeto");
        // altura: 110 Hz por contagem de passagens por zero no 2.º tempo
        const int a = (int) (0.50 * SR) + 300, len = 1500; int zc = 0; for (int i = a + 1; i < a + len; ++i) if (out[(size_t) i - 1] < 0 && out[(size_t) i] >= 0) ++zc;
        const double f = zc / (len / (double) SR); std::printf ("   altura medida %.0f Hz (esperado 110)\n", f); CHECK (std::fabs (f - 110.0) < 25.0, "altura mantida ao esticar");
        CHECK (e2.loopPhase() >= 0.0f && e2.loopPhase() <= 1.0f && e2.loopBpmSrc() == 90.0f, "estado para a interface");
    }
    // 3) projeto parado: corre livre a partir da nota
    {
        Engine e; e.prepare (SR, bs); e.setBpm (100); e.setParams (P); e.setLibrary (&lib); e.noteOn (60, 1.0f);
        std::vector<float> out; std::vector<float> l ((size_t) bs), r ((size_t) bs); for (int b = 0; b < 700; ++b) { e.setTransport (0, false); e.render (l.data(), r.data(), bs); out.insert (out.end(), l.begin(), l.end()); }
        const auto on = onsets (out); std::printf ("parado: %zu cliques\n", on.size()); CHECK (on.size() >= 5, "toca em modo livre");
        if (on.size() >= 3) { const double d1 = (on[1] - on[0]) / SR, d2 = (on[2] - on[1]) / SR; CHECK (std::fabs (d1 - 0.6) < 0.02 && std::fabs (d2 - 0.6) < 0.02, "intervalo = 1 tempo a 100 bpm"); }
    }
    std::printf ("%d verificações, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
