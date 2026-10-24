// Testes numéricos do núcleo (sem JUCE): g++ -O2 -std=c++17 -ISource tests/test_core.cpp -o test_core && ./test_core
#include "Core/Engine.h"
#include "Core/MidiFx.h"
#include "Core/PackSpecs.h"
#include "Core/Crypto.h"
#include "Core/Keys.h"
#include <chrono>
#include <cstdio>
#include <random>

using namespace sk;
static int failures = 0, checks = 0;
#define CHECK(cond, ...) do { ++checks; if (! (cond)) { ++failures; std::printf ("  FALHOU: "); std::printf (__VA_ARGS__); std::printf ("  [%s:%d]\n", __FILE__, __LINE__); } } while (0)

static float P[P_COUNT];
static void defaults() { for (int i = 0; i < P_COUNT; ++i) P[i] = kParams[i].def; }
static int idOf (const char* id) { for (int i = 0; i < P_COUNT; ++i) if (std::strcmp (kParams[i].id, id) == 0) return i; return -1; }

struct Rendered { std::vector<float> L, R; };
static Rendered run (Engine& e, int blocks, int bs = 512)
{
    Rendered r; std::vector<float> l ((size_t) bs), rr ((size_t) bs);
    for (int b = 0; b < blocks; ++b) { e.render (l.data(), rr.data(), bs); r.L.insert (r.L.end(), l.begin(), l.end()); r.R.insert (r.R.end(), rr.begin(), rr.end()); }
    return r;
}
static bool finite (const std::vector<float>& v) { for (float x : v) if (! std::isfinite (x)) return false; return true; }
static float peak (const std::vector<float>& v) { float p = 0; for (float x : v) p = std::max (p, std::fabs (x)); return p; }
static double rmsOf (const std::vector<float>& v, size_t a = 0, size_t b = 0) { if (! b) b = v.size(); double s = 0; for (size_t i = a; i < b; ++i) s += (double) v[i] * v[i]; return std::sqrt (s / (double) (b - a)); }

// estimativa da frequência (em Hz) por busca fina de máximo de DFT
static float measureFreq (const std::vector<float>& v, size_t start, size_t len, float fGuess, float sr)
{
    float best = 0, bf = fGuess;
    for (float c = -40; c <= 40; c += 0.25f)
    {
        const float f = fGuess * std::pow (2.0f, c / 1200.0f); double re = 0, im = 0;
        for (size_t i = 0; i < len; ++i) { const double ph = 6.283185307179586 * (double) f * (double) i / (double) sr; re += v[start + i] * std::cos (ph); im += v[start + i] * std::sin (ph); }
        const float m = (float) std::sqrt (re * re + im * im); if (m > best) { best = m; bf = f; }
    }
    return bf;
}

// energia fora das harmónicas (aliasing) em relação ao total, via FFT de 16384 pontos
static float aliasDb (const std::vector<float>& v, size_t start, float f0, float sr)
{
    const size_t N = 16384; std::vector<Cpx> a (N);
    for (size_t i = 0; i < N; ++i) { const float x = 6.2831853f * (float) i / (float) N, w = 0.35875f - 0.48829f * std::cos (x) + 0.14128f * std::cos (2 * x) - 0.01168f * std::cos (3 * x); a[i] = { v[start + i] * w, 0 }; }
    fft (a, false);
    double tot = 0, bad = 0; const float binHz = sr / (float) N;
    for (size_t k = 5; k < N / 2; ++k)
    {
        const double e = (double) a[k].re * a[k].re + (double) a[k].im * a[k].im; tot += e;
        const float f = (float) k * binHz, h = f / f0, dist = std::fabs (h - std::round (h)) * f0;   // distância à harmónica mais próxima
        if (dist > 6.0f * binHz) bad += e;
    }
    return (float) (10.0 * std::log10 (bad / std::max (tot, 1.0e-30) + 1.0e-30));
}

int main()
{
    const float SR = 48000.0f;
    std::printf ("SYNTHKEY núcleo — %d parâmetros\n", (int) P_COUNT);

    // ---------------------------------------------------------------- tabela de parâmetros
    {
        std::printf ("[1] tabela de parâmetros\n");
        for (int i = 0; i < P_COUNT; ++i)
        {
            const auto& p = kParams[i];
            CHECK (p.mn <= p.def && p.def <= p.mx, "%s: defeito %g fora de [%g,%g]", p.id, p.def, p.mn, p.mx);
            for (int j = i + 1; j < P_COUNT; ++j) CHECK (std::strcmp (p.id, kParams[j].id) != 0, "id repetido %s", p.id);
        }
    }

    // ---------------------------------------------------------------- wavetables
    {
        std::printf ("[2] wavetables\n");
        const auto& w = Wavetables::get();
        for (int t = 0; t < 16; ++t) for (int f = 0; f < 32; ++f) for (int m = 0; m < 8; ++m)
        {
            const float* c = w.cycle (t, f, m); float pk = 0, mean = 0; bool ok = true;
            for (int i = 0; i < 1024; ++i) { pk = std::max (pk, std::fabs (c[i])); mean += c[i]; ok = ok && std::isfinite (c[i]); }
            CHECK (ok && pk < 1.35f && std::fabs (mean / 1024.0f) < 0.02f && (m > 0 || pk > 0.85f), "tabela %d frame %d mip %d: pico %.2f dc %.3f", t, f, m, pk, mean / 1024.0f);
            CHECK (c[1024] == c[0], "guarda de interpolação");
        }
    }

    // ---------------------------------------------------------------- afinação e aliasing
    {
        std::printf ("[3] afinação e aliasing\n");
        for (int type = 0; type < 2; ++type)
        {
            for (int w = 0; w < 4; ++w)
            {
                if (type == 1 && w > 0) break;
                defaults(); P[P_F1_TYPE] = 0; P[P_A_TYPE] = (float) type; P[P_A_WAVE] = (float) w; P[P_AMP_A] = 1; P[P_AMP_S] = 1; P[P_AMP_D] = 5000; P[P_LIM_ON] = 0;
                if (type == 1) { P[P_A_WTTAB] = 0; P[P_A_WTPOS] = 0.6f; }   // quadrada/pulso
                Engine e; e.prepare (SR, 512); e.setParams (P);
                for (int note : { 45, 69, 93 })
                {
                    e.panic(); e.noteOn (note, 1.0f); auto r = run (e, 54);
                    const float f0 = midiToHz ((float) note), got = measureFreq (r.L, 8000, 8192, f0, SR);
                    const float cents = 1200.0f * std::log2 (got / f0);
                    CHECK (std::fabs (cents) < 2.0f, "tipo %d onda %d nota %d: erro %.2f cents", type, w, note, cents);
                    const float al = aliasDb (r.L, 8000, f0, SR);
                    if (note >= 69 && w != 0) CHECK (al < -45.0f, "tipo %d onda %d nota %d: aliasing %.1f dB", type, w, note, al);
                    if (note == 93) std::printf ("   tipo %d onda %d nota %d  afinação %+.2f ct  aliasing %.1f dB\n", type, w, note, cents, al);
                }
            }
        }
    }

    // ---------------------------------------------------------------- filtros, efeitos e unissono sem NaN
    {
        std::printf ("[4] filtros / motores / efeitos (estabilidade)\n");
        for (int ft = 0; ft <= 8; ++ft)
            for (float res : { 0.0f, 0.5f, 1.0f })
            {
                defaults(); P[P_F1_TYPE] = (float) ft; P[P_F1_CUT] = 800; P[P_F1_RES] = res; P[P_F1_DRIVE] = 0.5f; P[P_LIM_ON] = 1;
                Engine e; e.prepare (SR, 512); e.setParams (P); e.noteOn (48, 1.0f); e.noteOn (55, 1.0f);
                auto r = run (e, 30);
                CHECK (finite (r.L) && finite (r.R), "filtro %d res %.1f produziu NaN", ft, res);
                CHECK (peak (r.L) <= 1.0f, "filtro %d res %.1f: pico %.2f", ft, res, peak (r.L));
            }
        for (int fx = 1; fx <= 10; ++fx)
            for (float pv : { 0.0f, 0.5f, 1.0f })
            {
                defaults(); const int b = fxSlotBase (0); P[b] = (float) fx; for (int k = 1; k <= 5; ++k) P[b + k] = pv; P[b + 6] = 1.0f;
                Engine e; e.prepare (SR, 512); e.setParams (P); e.noteOn (60, 1.0f); e.noteOn (64, 1.0f);
                auto r = run (e, 60);
                CHECK (finite (r.L) && finite (r.R), "efeito %d param %.1f produziu NaN", fx, pv);
                CHECK (peak (r.L) <= 1.0f && peak (r.R) <= 1.0f, "efeito %d param %.1f: pico %.2f", fx, pv, peak (r.L));
            }
        defaults(); P[P_A_UNI] = 8; P[P_A_UNIDET] = 40; P[P_B_ON] = 1; P[P_B_TYPE] = 1; P[P_B_UNI] = 8; P[P_POLYPHONY] = 16;
        { Engine e; e.prepare (SR, 512); e.setParams (P); for (int n = 36; n < 52; ++n) e.noteOn (n, 0.8f);
          auto t0 = std::chrono::steady_clock::now(); auto r = run (e, 200); const double s = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
          std::printf ("   CPU: 16 vozes × 16 osciladores unisson: %.1f%% de um núcleo em tempo real\n", 100.0 * s / (200.0 * 512.0 / SR));
          CHECK (finite (r.L), "unissono NaN"); }
    }

    // ---------------------------------------------------------------- modulação
    {
        std::printf ("[5] matriz de modulação\n");
        defaults(); P[P_F1_TYPE] = 1; P[P_F1_CUT] = 500; P[P_A_WAVE] = 2;
        const int s0 = modSlotBase (0); P[s0] = MS_MACRO1; P[s0 + 1] = MD_CUT1; P[s0 + 2] = 1.0f;
        Engine e; e.prepare (SR, 512); e.setParams (P);
        auto hf = [] (const std::vector<float>& v) { std::vector<float> d; for (size_t i = 6001; i < v.size(); ++i) d.push_back (v[i] - v[i - 1]); return rmsOf (d); };
        P[P_MACRO1] = 0.0f; e.noteOn (48, 1.0f); auto r0 = run (e, 20); const double lo = hf (r0.L);
        e.panic(); P[P_MACRO1] = 1.0f; e.noteOn (48, 1.0f); auto r1 = run (e, 20); const double hi = hf (r1.L);
        CHECK (hi > lo * 2.0, "macro→corte: brilho %.4f → %.4f", lo, hi);
        // LFO → afinação (vibrato): frequência instantânea oscila
        defaults(); P[P_F1_TYPE] = 0; P[P_A_WAVE] = 0; P[P_LFO1_RATE] = 5; P[P_AMP_S] = 1; P[P_AMP_D] = 5000;
        P[s0] = MS_LFO1; P[s0 + 1] = MD_PITCH_ALL; P[s0 + 2] = 0.2f;
        Engine e2; e2.prepare (SR, 512); e2.setParams (P); e2.noteOn (69, 1.0f); auto r2 = run (e2, 60);
        const float fa = measureFreq (r2.L, 4000, 2048, 440.0f, SR), fb = measureFreq (r2.L, 4000 + 4800, 2048, 440.0f, SR);
        CHECK (std::fabs (fa - fb) > 1.0f, "LFO→afinação: %.1f vs %.1f Hz", fa, fb);
    }

    // ---------------------------------------------------------------- vozes
    {
        std::printf ("[6] vozes (polifonia, mono, legato, sustain)\n");
        defaults(); P[P_POLYPHONY] = 8; P[P_AMP_R] = 400;
        Engine e; e.prepare (SR, 512); e.setParams (P);
        for (int n = 40; n < 80; ++n) e.noteOn (n, 0.8f);
        run (e, 4); CHECK (e.activeVoices() <= 8, "polifonia 8 excedida: %d", e.activeVoices());
        e.allNotesOff(); run (e, 120); CHECK (e.activeVoices() == 0, "vozes presas: %d", e.activeVoices());
        P[P_VOICE_MODE] = 1; e.noteOn (60, 1); e.noteOn (64, 1); run (e, 2); CHECK (e.activeVoices() == 1, "mono: %d vozes", e.activeVoices());
        e.noteOff (64); run (e, 2); CHECK (e.activeVoices() == 1, "mono volta à nota anterior");
        e.noteOff (60); run (e, 120); CHECK (e.activeVoices() == 0, "mono libertou");
        P[P_VOICE_MODE] = 0; e.setSustain (true); e.noteOn (60, 1); e.noteOff (60); run (e, 100); CHECK (e.activeVoices() == 1, "sustain deve segurar a nota");
        e.setSustain (false); run (e, 150); CHECK (e.activeVoices() == 0, "sustain solto liberta");
    }

    // ---------------------------------------------------------------- escala, acordes, arpejador
    {
        std::printf ("[7] escala / acordes / arpejador\n");
        CHECK (MidiFx::snapToScale (61, 0, 1) == 62 || MidiFx::snapToScale (61, 0, 1) == 60, "snap Dó# em Dó maior");
        CHECK (MidiFx::snapToScale (66, 0, 1) == 67, "snap Fá# → Sol (empate sobe)");
        int out[8]; int n = MidiFx::chordNotes (3, 60, 0, 0, out); CHECK (n == 3 && out[1] == 64 && out[2] == 67, "acorde maior");
        n = MidiFx::chordNotes (13, 62, 0, 1, out); CHECK (n == 4 && out[0] == 62 && out[1] == 65 && out[2] == 69 && out[3] == 72, "Dm7 diatónico: %d %d %d %d", out[0], out[1], out[2], out[3]);
        defaults(); P[P_ARP_ON] = 1; P[P_ARP_RATE] = 12; /* 1/16 */ P[P_ARP_MODE] = 0; P[P_ARP_GATE] = 0.5f;
        MidiFx fx; fx.prepare (SR); fx.setParams (P);
        std::vector<NoteEvent> ev, all;
        fx.noteOn (60, 0.8f, 0); fx.noteOn (64, 0.8f, 0); fx.noteOn (67, 0.8f, 0);
        int64_t t = 0; for (int b = 0; b < 200; ++b) { fx.process (512, 120.0, 0, false, ev); for (auto e : ev) { e.offset += (int) (t); all.push_back (e); } t += 512; }
        std::vector<int> ons; std::vector<int> times; for (auto& e : all) if (e.on) { ons.push_back (e.note); times.push_back (e.offset); }
        CHECK (ons.size() >= 8, "arp gerou %zu notas", ons.size());
        if (ons.size() >= 6) { CHECK (ons[0] == 60 && ons[1] == 64 && ons[2] == 67 && ons[3] == 60, "ordem do arp: %d %d %d %d", ons[0], ons[1], ons[2], ons[3]); CHECK (std::abs ((times[1] - times[0]) - 6000) <= 1, "tempo entre passos: %d (esperado 6000)", times[1] - times[0]); }
        // todas as notas ligadas têm desligar
        int on = 0, off = 0; for (auto& e : all) (e.on ? on : off)++; CHECK (off >= on - 1, "arp: %d ligadas, %d desligadas", on, off);
        // padrões de estilo: cada um produz notas
        for (int pat = 0; pat < 8; ++pat)
        {
            defaults(); P[P_ARP_ON] = 1; P[P_ARP_PATTERN] = (float) pat; P[P_ARP_RATE] = 12; MidiFx f2; f2.prepare (SR); f2.setParams (P); f2.noteOn (60, 1, 0); f2.noteOn (64, 1, 0);
            int cnt = 0; for (int b = 0; b < 400; ++b) { f2.process (512, 120.0, 0, false, ev); for (auto& e : ev) if (e.on) ++cnt; }
            CHECK (cnt > 4, "padrão %d: só %d notas", pat, cnt);
        }
        // padrão próprio: 4 passos [cheio, pausa, cheio +8, pausa] com 1 nota
        {
            defaults(); P[P_ARP_ON] = 1; P[P_ARP_RATE] = 12; P[P_ARP_EDIT] = 1; P[P_ARP_LEN] = 4;
            for (int i = 0; i < 16; ++i) { P[P_ARP_V1 + 3 * i] = (i % 2 == 0) ? 1.0f : 0.0f; P[P_ARP_V1 + 3 * i + 1] = (i == 2) ? 1.0f : 0.0f; P[P_ARP_V1 + 3 * i + 2] = 0; }
            MidiFx f3; f3.prepare (SR); f3.setParams (P); f3.noteOn (60, 1, 0);
            std::vector<int> notes, offs; int64_t tt = 0;
            for (int b = 0; b < 400; ++b) { f3.process (512, 120.0, 0, false, ev); for (auto& e : ev) if (e.on) { notes.push_back (e.note); offs.push_back ((int) (tt + e.offset)); } tt += 512; }
            CHECK (notes.size() >= 8, "padrão próprio: %zu notas", notes.size());
            if (notes.size() >= 4) { CHECK (notes[0] == 60 && notes[1] == 72 && notes[2] == 60 && notes[3] == 72, "padrão próprio: %d %d %d %d", notes[0], notes[1], notes[2], notes[3]);
                                     CHECK (std::abs ((offs[1] - offs[0]) - 12000) <= 2, "padrão próprio: pausa entre notas = %d (esperado 12000)", offs[1] - offs[0]); }
        }
    }

    // ---------------------------------------------------------------- oversampling, formante, delay sincronizado
    {
        std::printf ("[7c] wavetable do utilizador (importada)\n");
        {
            // 1) ciclo único de 600 amostras (serra) → toda a tabela; 2) tabela estilo Serum de 4 ciclos × 2048; 3) som qualquer
            std::vector<float> one (600); for (int i = 0; i < 600; ++i) one[(size_t) i] = 2.0f * (float) i / 600.0f - 1.0f;
            std::vector<float> multi (4 * 2048); for (int c = 0; c < 4; ++c) for (int i = 0; i < 2048; ++i) multi[(size_t) (c * 2048 + i)] = std::sin (6.2831853f * (float) (c + 1) * (float) i / 2048.0f);
            std::vector<float> noiseish (30000); for (size_t i = 0; i < noiseish.size(); ++i) noiseish[i] = std::sin ((float) i * 0.013f) * std::sin ((float) i * 0.00041f);
            std::vector<float> tiny (10, 0.5f);
            std::vector<float> fr;
            CHECK (! Wavetables::framesFromSignal (tiny.data(), 10, fr), "sinal demasiado curto devia ser recusado");
            const std::vector<float>* sigs[3] = { &one, &multi, &noiseish };
            for (int k = 0; k < 3; ++k)
            {
                CHECK (Wavetables::framesFromSignal (sigs[k]->data(), (int) sigs[k]->size(), fr), "framesFromSignal %d", k);
                CHECK ((int) fr.size() == Wavetables::kFrames * Wavetables::kSize, "tamanho das frames %d", k);
                CHECK (finite (fr), "NaN nas frames %d", k);
                defaults(); P[P_F1_TYPE] = 0; P[P_A_TYPE] = 1; P[P_A_WTTAB] = (float) Wavetables::kUserTable; P[P_A_WTPOS] = 0.0f; P[P_SUB_LEVEL] = kParams[P_SUB_LEVEL].mn; P[P_B_ON] = 0; P[P_NOISE_LEVEL] = kParams[P_NOISE_LEVEL].mn; P[P_A_UNI] = 1; P[P_AMP_A] = 1; P[P_AMP_S] = 1; P[P_LIM_ON] = 0;
                Engine e; e.prepare (SR, 512); e.setParams (P);
                { e.noteOn (60, 1.0f); auto r0 = run (e, 20); CHECK (finite (r0.L), "tabela do utilizador por carregar devia ser segura (k=%d)", k); e.panic(); }
                CHECK (e.userWavetable().setSignal (sigs[k]->data(), (int) sigs[k]->size()), "setSignal %d", k);
                e.noteOn (60, 1.0f); auto r = run (e, 40);
                CHECK (finite (r.L), "NaN com tabela do utilizador %d", k);
                CHECK (rmsOf (r.L, 4000, r.L.size()) > 0.02, "tabela do utilizador %d sem som", k);
                if (k == 1)   // frame 0 = 1 ciclo de seno → fundamental em 261.6 Hz
                { const float f = measureFreq (r.L, 12000, 8192, 261.63f, (float) SR); CHECK (std::fabs (f / 261.63f - 1.0f) < 0.004f, "tabela do utilizador: frequência %.2f", f); }
                // recuperar as frames (projeto) e reconstruir dá o mesmo som
                auto saved = e.userWavetable().frames(); UserWavetable u2; CHECK (u2.setFrames (saved), "setFrames");
                CHECK (std::fabs (u2.cycle (5, 0)[100] - e.userWavetable().cycle (5, 0)[100]) < 1e-5f, "reconstrução diferente");
            }
        }
        std::printf ("[7d] novas fontes de modulação (step, LFO desenhado, env multi-ponto, pressão, MPE)\n");
        for (int sl = 0; sl < kNumModSlots; ++sl) { const int b = modSlotBase (sl); CHECK ((int) kParams[b].mx == MS_COUNT - 1, "intervalo da origem da matriz cobre todas as fontes"); CHECK ((int) kParams[b + 1].mx == MD_COUNT - 1, "intervalo do destino da matriz cobre todos os destinos"); }
        {
            auto base = [&] { defaults(); P[P_F1_TYPE] = 0; P[P_A_TYPE] = 0; P[P_A_WAVE] = 0; P[P_B_ON] = 0; P[P_A_UNI] = 1; P[P_SUB_LEVEL] = kParams[P_SUB_LEVEL].mn; P[P_NOISE_LEVEL] = kParams[P_NOISE_LEVEL].mn;
                              P[P_AMP_A] = 1; P[P_AMP_S] = 1; P[P_LIM_ON] = 0; };
            auto route = [&] (int slot, int src, int dst, float amt) { const int b = modSlotBase (slot); P[b] = (float) src; P[b + 1] = (float) dst; P[b + 2] = amt; };
            auto semis = [] (float st) { return 261.63f * std::pow (2.0f, st / 12.0f); };
            auto freqAt = [&] (const Rendered& r, double t0, double t1, float guess) { return measureFreq (r.L, (size_t) (t0 * SR), (size_t) ((t1 - t0) * SR), guess, (float) SR); };
            auto near = [] (float f, float want) { return std::fabs (f / want - 1.0f) < 0.006f; };
            // step sequencer: 2 passos (+1, -1) de 1/4 a 120 bpm → ±6 semitons (0.25 × 24)
            {
                base(); route (0, MS_STEP, MD_PITCH_ALL, 0.25f); P[P_STEP_LEN] = 2; P[P_STEP_DIV] = 6; P[P_STEP_SMOOTH] = 0; P[P_STEP_V1] = 1; P[P_STEP_V1 + 1] = -1;
                Engine e; e.prepare (SR, 512); e.setBpm (120.0f); e.setParams (P); e.noteOn (60, 1.0f); auto r = run (e, 160);
                const float f1 = freqAt (r, 0.15, 0.45, semis (6)), f2 = freqAt (r, 0.65, 0.95, semis (-6)), f3 = freqAt (r, 1.15, 1.45, semis (6));
                CHECK (near (f1, semis (6)), "step 1: %.1f Hz (esperado %.1f)", f1, semis (6));
                CHECK (near (f2, semis (-6)), "step 2: %.1f Hz (esperado %.1f)", f2, semis (-6));
                CHECK (near (f3, semis (6)), "step ao fim de 2 passos volta ao 1: %.1f Hz", f3);
                CHECK (finite (r.L), "NaN no step seq");
                // suavizado: continua finito e passa por valores intermédios
                P[P_STEP_SMOOTH] = 1.0f; Engine e2; e2.prepare (SR, 512); e2.setBpm (120.0f); e2.setParams (P); e2.noteOn (60, 1.0f); auto r2 = run (e2, 60); CHECK (finite (r2.L), "NaN no step suavizado");
            }
            // LFO desenhado: valor constante 0.5 → +6 st; e forma rampa a 2 Hz não dá NaN
            {
                base(); route (0, MS_DRAW, MD_PITCH_ALL, 0.5f); P[P_DRAW_SYNC] = 0; P[P_DRAW_RATE] = 2.0f; for (int i = 0; i < 32; ++i) P[P_DRAW_P1 + i] = 0.5f;
                Engine e; e.prepare (SR, 512); e.setParams (P); e.noteOn (60, 1.0f); auto r = run (e, 60);
                const float f = freqAt (r, 0.2, 0.5, semis (6)); CHECK (near (f, semis (6)), "LFO desenhado constante: %.1f Hz", f);
                for (int i = 0; i < 32; ++i) P[P_DRAW_P1 + i] = (i % 2) ? 1.0f : -1.0f;
                Engine e2; e2.prepare (SR, 512); e2.setParams (P); e2.noteOn (60, 1.0f); auto r2 = run (e2, 60); CHECK (finite (r2.L), "NaN no LFO desenhado");
            }
            // envelope multi-ponto: sustain no ponto 3 (nível 0.55) → +0.275×24 = 6.6 st
            {
                base(); route (0, MS_MENV, MD_PITCH_ALL, 0.5f); P[P_MENV_LEN] = 400; P[P_MENV_SUS] = 3; P[P_MENV_REL] = 200;
                Engine e; e.prepare (SR, 512); e.setParams (P); e.noteOn (60, 1.0f); auto r = run (e, 70);
                const float f = freqAt (r, 0.2, 0.5, semis (6.6f)); CHECK (near (f, semis (6.6f)), "env multi-ponto no sustain: %.1f Hz (esperado %.1f)", f, semis (6.6f));
                e.noteOff (60); auto r2 = run (e, 100); CHECK (finite (r2.L), "NaN na libertação da env multi-ponto");
                // sem sustain nem loop: o nível final do último ponto (0.35) mantém-se
                P[P_MENV_SUS] = 0; Engine e3; e3.prepare (SR, 512); e3.setParams (P); e3.noteOn (60, 1.0f); auto r3 = run (e3, 90);
                const float f3 = freqAt (r3, 0.5, 0.8, semis (0.175f * 24.0f)); CHECK (near (f3, semis (4.2f)), "env multi-ponto no fim: %.1f Hz (esperado %.1f)", f3, semis (4.2f));
            }
            // pressão por nota (aftertouch polifónico / MPE) e curvatura MPE
            {
                base(); route (0, MS_PRESSURE, MD_PITCH_ALL, 0.5f);
                Engine e; e.prepare (SR, 512); e.setParams (P); e.noteOn (60, 1.0f); e.setNotePressure (60, 0.5f); auto r = run (e, 40);
                const float f = freqAt (r, 0.2, 0.4, semis (6)); CHECK (near (f, semis (6)), "pressão por nota: %.1f Hz", f);
                Engine e2; e2.prepare (SR, 512); e2.setParams (P); e2.noteOn (60, 1.0f); e2.noteOn (67, 1.0f); e2.setNotePressure (67, 1.0f); auto r2 = run (e2, 40); CHECK (finite (r2.L), "NaN com 2 notas");
                base(); P[P_MPE_RANGE] = 48; Engine e3; e3.prepare (SR, 512); e3.setParams (P); e3.noteOn (60, 1.0f); e3.setNoteBend (60, 0.25f); auto r3 = run (e3, 40);
                const float f3 = freqAt (r3, 0.2, 0.4, semis (12)); CHECK (near (f3, semis (12)), "curvatura MPE: %.1f Hz", f3);
                // a nota vizinha não é afetada
                Engine e4; e4.prepare (SR, 512); e4.setParams (P); e4.noteOn (60, 1.0f); e4.noteOn (72, 1.0f); e4.setNoteBend (72, -0.25f); auto r4 = run (e4, 40); CHECK (finite (r4.L), "NaN com curvatura em 2 notas");
            }
        }
        std::printf ("[7b] oversampling / formante / delay tempo\n");
        double ref = 0;
        for (int q = 0; q < 3; ++q)
        {
            defaults(); P[P_F1_TYPE] = 0; P[P_A_TYPE] = 0; P[P_A_WAVE] = 0; P[P_AMP_A] = 1; P[P_AMP_S] = 1; P[P_AMP_D] = 5000; P[P_LIM_ON] = 0; P[P_QUALITY] = (float) q;
            Engine e; e.setQuality (q); e.prepare (SR, 512); e.setParams (P); e.noteOn (81, 1.0f);   // lá 880 Hz
            auto r = run (e, 40); CHECK (finite (r.L), "quality %d: NaN", q);
            const double rm = rmsOf (r.L, 8192, r.L.size()); if (q == 0) ref = rm;
            CHECK (std::fabs (20.0 * std::log10 (rm / ref)) < 0.6, "quality %d: nível %.2f dB vs normal", q, 20.0 * std::log10 (rm / ref));
            const float f = measureFreq (r.L, 12000, 8192, 880.0f, (float) SR); CHECK (std::fabs (f / 880.0f - 1.0f) < 0.003f, "quality %d: frequência %.2f", q, f);
            CHECK (e.quality() == q, "quality() = %d", e.quality());
            if (q > 0) CHECK (e.latencySamples() >= 72 && e.latencySamples() < 120, "latência com oversampling: %d", e.latencySamples());
            // blocos de tamanho diferente do preparado também têm de funcionar
            auto r2 = run (e, 30, 77); CHECK (finite (r2.L), "quality %d: NaN com bloco 77", q);
        }
        // oversampling reduz aliasing: serra-FM em nota aguda, energia acima de 18 kHz
        {
            double hi[3];
            for (int q = 0; q < 3; ++q)
            {
                defaults(); P[P_F1_TYPE] = 0; P[P_A_TYPE] = 0; P[P_A_WAVE] = 0; P[P_A_FMAMT] = 0.8f; P[P_A_FMRAT] = 3; P[P_AMP_A] = 1; P[P_AMP_S] = 1; P[P_LIM_ON] = 0; P[P_QUALITY] = (float) q;
                Engine e; e.setQuality (q); e.prepare (SR, 512); e.setParams (P); e.noteOn (96, 1.0f);
                auto r = run (e, 40); const size_t a = 12000, len = 8192; double en = 0;
                for (float fq = 18500.0f; fq < 22000.0f; fq += 90.0f) { double re = 0, im = 0; for (size_t i = 0; i < len; ++i) { const double ph = 6.283185307179586 * fq * (double) i / SR; re += r.L[a + i] * std::cos (ph); im += r.L[a + i] * std::sin (ph); } en += re * re + im * im; }
                hi[q] = en;
            }
            std::printf ("   energia >18 kHz: normal %.3g, 2x %.3g, 4x %.3g\n", hi[0], hi[1], hi[2]);
            CHECK (hi[2] <= hi[0] * 1.05, "4x não reduz aliasing");
        }
        // formante: ruído filtrado muda de espectro conforme a vogal
        {
            double c1[2];
            for (int k = 0; k < 2; ++k)
            {
                defaults(); P[P_A_ON] = 0; P[P_B_ON] = 0; P[P_NOISE_LEVEL] = -6; P[P_F1_TYPE] = 9; P[P_F1_CUT] = k ? 3000.0f : 30.0f; P[P_F1_RES] = 0.6f; P[P_AMP_A] = 1; P[P_AMP_S] = 1; P[P_LIM_ON] = 0;
                Engine e; e.prepare (SR, 512); e.setParams (P); e.noteOn (60, 1.0f);
                auto r = run (e, 30); CHECK (finite (r.L) && peak (r.L) > 0.001f, "formante %d: mudo ou NaN (%g)", k, peak (r.L));
                double num = 0, den = 0; for (size_t i = 8192; i + 1 < r.L.size(); ++i) { const double d = r.L[i + 1] - r.L[i]; num += d * d; den += (double) r.L[i] * r.L[i]; }
                c1[k] = num / std::max (den, 1e-12);
            }
            CHECK (std::fabs (c1[0] - c1[1]) > 0.05 * std::max (c1[0], c1[1]), "formante: vogais A e U soam iguais (%.3f vs %.3f)", c1[0], c1[1]);
        }
        // delay sincronizado: 1/8 a 120 bpm = 250 ms = 12000 amostras
        {
            FxUnit fx; fx.prepare ((float) SR, 512); std::vector<float> L (512 * 40, 0.0f), R (L.size(), 0.0f); L[0] = R[0] = 1.0f;
            float pp[5] = { 4.0f / 7.0f, 0.3f, 1.0f, 0.0f, 0.0f };
            for (int b = 0; b < 40; ++b) fx.process (FxUnit::DelaySync, pp, 1.0f, L.data() + b * 512, R.data() + b * 512, 512, 120.0f);
            int idx = -1; float best = 0; for (int i = 2000; i < 20000; ++i) if (std::fabs (L[(size_t) i]) > best) { best = std::fabs (L[(size_t) i]); idx = i; }
            CHECK (std::abs (idx - 12000) < 40, "delay 1/8 a 120 bpm: eco em %d (esperado 12000)", idx);
        }
        // reverb continua estável com a nova difusão
        {
            FxUnit fx; fx.prepare ((float) SR, 512); std::vector<float> L (512 * 200, 0.0f), R (L.size(), 0.0f); L[0] = R[0] = 1.0f; float pp[5] = { 0.6f, 0.8f, 0.3f, 0.1f, 1.0f };
            for (int b = 0; b < 200; ++b) fx.process (FxUnit::Reverb, pp, 1.0f, L.data() + b * 512, R.data() + b * 512, 512, 120.0f);
            CHECK (finite (L) && peak (L) < 2.0f, "reverb instável: pico %g", peak (L));
            double early = rmsOf (L, 100, 3000), late = rmsOf (L, 60000, 100000); CHECK (early > late, "reverb: cauda maior que o início?");
        }
    }

    // ---------------------------------------------------------------- samples e packs
    {
        std::printf ("[8] samples e packs\n");
        const auto defs = buildPackDefs(); CHECK (defs.size() == 8, "8 packs");
        Library lib; for (auto& d : defs) if (d.id == "nucleo" || d.id == "amapiano") buildPackLibrary (d, lib);
        std::printf ("   Núcleo + Amapiano: %zu instrumentos, %zu amostras\n", lib.inst.size(), lib.sampleCount());
        const int kit = lib.indexOf ("nucleo.kit"), ld = lib.indexOf ("amapiano.logdrum");
        CHECK (kit >= 0 && ld >= 0, "instrumentos em falta");
        for (int which = 0; which < 2; ++which)
        {
            defaults(); P[P_A_TYPE] = 2; P[P_A_SMP] = (float) (which ? ld : kit); P[P_F1_TYPE] = 0; P[P_A_RR] = 1; P[P_AMP_D] = 3000;
            Engine e; e.prepare (SR, 512); e.setParams (P); e.setLibrary (&lib);
            e.noteOn (which ? 48 : 36, 1.0f); auto r = run (e, 40);
            CHECK (finite (r.L) && peak (r.L) > 0.05f, "sample %s: pico %.3f", which ? "log drum" : "bombo", peak (r.L));
            if (which)   // afinação do log drum: nota 48 → 130.8 Hz
            { const float f0 = midiToHz (48), got = measureFreq (r.L, 12000, 8192, f0, SR); CHECK (std::fabs (1200.0f * std::log2 (got / f0)) < 30.0f, "afinação do log drum: %.1f Hz", got); }
        }
        // zonas cobrem todo o teclado sem buracos
        for (auto& I : lib.inst) { if (I.fixedPitch) continue; for (int k = 0; k < 128; ++k) { bool ok = false; for (auto& z : I.zones) if (k >= z.loKey && k <= z.hiKey) ok = true; CHECK (ok, "%s: tecla %d sem zona", I.id.c_str(), k); if (! ok) break; } }
        // WAV: escrever e ler dá o mesmo resultado (24 bit)
        const auto& d0 = *lib.data[0]; auto wav = writeWav24 (d0); SampleData back; CHECK (readWav (wav.data(), wav.size(), back), "ler wav");
        double err = 0; for (int i = 0; i < d0.frames() && i < back.frames(); ++i) err = std::max (err, (double) std::fabs (d0.l[(size_t) i] - back.l[(size_t) i])); CHECK (err < 2.0e-7 && back.frames() == d0.frames(), "ida e volta wav: erro %.2e", err);
    }

    // ---------------------------------------------------------------- criptografia e chaves
    {
        std::printf ("[9] criptografia e chaves de packs\n");
        CHECK (crypto::sha256Hex ("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", "SHA-256 'abc'");
        CHECK (crypto::toHex (crypto::hmacSha256 ("key", "The quick brown fox jumps over the lazy dog")) == "f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8", "HMAC-SHA256");
        CHECK (crypto::toHex (crypto::pbkdf2Sha256 ("password", "salt", 2, 32)) == "ae4d0c95af6b46d32d0adff928f06dd02a303f8ef3c251dfd6e2d85a95474c43", "PBKDF2 vetor RFC 7914");
        std::string data = "SYNTHKEY samples de teste 1234567890"; std::string enc = data; crypto::streamXor (enc, "chave", "pack:amapiano"); CHECK (enc != data, "cifra altera");
        crypto::streamXor (enc, "chave", "pack:amapiano"); CHECK (enc == data, "cifra reversível");
        keys::Payload pl; pl.permanent = false; pl.expiryDay = 400; pl.seats = 2; pl.serial = 123456; pl.relative = false;
        const std::string key = keys::makeKey ("secretoXYZ", "trap", pl);
        std::printf ("   chave de exemplo: %s\n", key.c_str());
        keys::Payload out; CHECK (keys::parseKey ("secretoXYZ", "trap", key, out) && out.serial == 123456 && out.seats == 2 && ! out.permanent && out.expiryDay == 400, "chave válida");
        CHECK (! keys::parseKey ("secretoXYZ", "drill", key, out), "chave de Trap não abre Drill");
        CHECK (! keys::parseKey ("outro", "trap", key, out), "segredo diferente recusado");
        std::string bad = key; bad[7] = bad[7] == 'A' ? 'B' : 'A'; CHECK (! keys::parseKey ("secretoXYZ", "trap", bad, out), "chave adulterada recusada");
        CHECK (keys::parseKey ("secretoXYZ", "trap", " trap-" + key.substr (5) + " ", out), "tolera espaços e minúsculas");
        int days = keys::daysSince2026 (2026, 11, 1); CHECK (days == 304, "dias até 01/11/2026: %d", days);
    }

    // ---------------------------------------------------------------- fuzz
    {
        std::printf ("[10] fuzz de parâmetros (300 patches aleatórios)\n");
        std::mt19937 rng (42); std::uniform_real_distribution<float> U (0, 1);
        Engine e; e.prepare (SR, 256);
        Library lib; for (auto& d : buildPackDefs()) if (d.id == "nucleo") buildPackLibrary (d, lib);
        e.setLibrary (&lib); int bad = 0; float worst = 0;
        for (int it = 0; it < 300; ++it)
        {
            for (int i = 0; i < P_COUNT; ++i)
            {
                const auto& p = kParams[i]; float v = p.mn + (p.mx - p.mn) * U (rng);
                if (p.kind == PK_LOG) v = p.mn * std::pow (p.mx / p.mn, U (rng));
                if (p.kind == PK_INT || p.kind == PK_CHOICE) v = std::round (v);
                if (p.kind == PK_BOOL) v = U (rng) > 0.5f ? 1.0f : 0.0f;
                P[i] = v;
            }
            P[P_A_SMP] = std::floor (U (rng) * (float) lib.inst.size()); P[P_B_SMP] = std::floor (U (rng) * (float) lib.inst.size());
            P[P_LIM_ON] = 1; P[P_LIM_CEIL] = -0.3f; P[P_MASTER_VOL] = 0;
            e.setParams (P); e.panic(); e.reset();
            for (int k = 0; k < 4; ++k) e.noteOn (30 + (int) (U (rng) * 60), U (rng));
            auto r = run (e, 12, 256);
            const float pk = std::max (peak (r.L), peak (r.R));
            if (! finite (r.L) || ! finite (r.R)) { ++bad; std::printf ("   NaN no patch %d\n", it); }
            worst = std::max (worst, pk);
            if (pk > 0.9999f) { ++bad; std::printf ("   pico %.3f no patch %d\n", pk, it); }
        }
        CHECK (bad == 0, "fuzz: %d patches com problemas", bad);
        std::printf ("   pico máximo observado: %.4f (teto -0.3 dB = 0.9661)\n", worst);
    }

    std::printf ("\n%d verificações, %d falhas\n", checks, failures);
    return failures ? 1 : 0;
}
