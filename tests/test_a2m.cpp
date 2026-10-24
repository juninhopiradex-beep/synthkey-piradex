// Áudio -> MIDI: bateria, melodia, baixo e acordes sintéticos com a verdade conhecida
#include "Core/AudioToMidi.h"
#include "Core/SampleSynth.h"
#include <cstdio>
#include <map>
#include <set>
using namespace sk;
static int fails = 0, checks = 0;
#define CHECK(c, msg) do { ++checks; if (! (c)) { ++fails; std::printf ("FALHOU: %s (linha %d)\n", msg, __LINE__); } } while (0)
static const double SR = 44100.0;
// serra de banda limitada (sem aliasing), como num sintetizador a sério
static float saw (double ph, double f) { float s = 0; for (int h = 1; h * f < 8000.0 && h <= 60; ++h) s += (float) std::sin (2.0 * 3.14159265358979 * h * ph) / (float) h; return s * 0.64f; }
static void mixIn (std::vector<float>& out, const std::vector<float>& s, double at, float g) { const size_t a = (size_t) std::lround (at * SR); if (out.size() < a + s.size()) out.resize (a + s.size(), 0.0f); for (size_t i = 0; i < s.size(); ++i) out[a + i] += s[i] * g; }

int main()
{
    const double bpm = 100, grid = 60.0 / bpm / 4;
    // ---- bateria "difícil": bombo com varrimento de altura, caixa com corpo tonal (vaza para os graves) e hats de ruído
    {
        const double g2 = 60.0 / 100 / 4; std::vector<float> x; unsigned rs = 7; auto rnd = [&] { rs = rs * 1664525u + 1013904223u; return (float) (rs >> 8) / 8388608.0f - 1.0f; };
        std::vector<float> kk, sn, ht; { double ph = 0; for (int i = 0; i < (int) (SR * 0.3); ++i) { const double t = i / SR; ph += 2 * 3.14159265358979 * (50 + 100 * std::exp (-t * 30)) / SR; kk.push_back ((float) (std::sin (ph) * std::exp (-t * 12))); } }
        for (int i = 0; i < (int) (SR * 0.2); ++i) { const double t = i / SR; sn.push_back ((float) (rnd() * std::exp (-t * 25) * 0.7 + std::sin (2 * 3.14159265358979 * 190 * t) * std::exp (-t * 30) * 0.5)); }
        { float prev = 0; for (int i = 0; i < (int) (SR * 0.06); ++i) { const float v = rnd(); ht.push_back ((v - prev) * (float) std::exp (-(i / SR) * 70) * 0.5f); prev = v; } }
        for (int s = 0; s < 64; ++s)
        {
            const double t = 0.05 + s * g2; const int m = s % 16;
            if (m == 0 || m == 6 || m == 10) mixIn (x, kk, t, 0.9f); if (m == 4 || m == 12) mixIn (x, sn, t, 0.8f); if (m % 2 == 0) mixIn (x, ht, t, 0.4f);
        }
        a2m::Options o; o.style = "trap"; o.name = "dificil"; auto r = a2m::convert (x, SR, o); CHECK (r.ok, "bateria dificil converte");
        if (r.ok)
        {
            int k = 0, sOk = 0, bad = 0; for (auto& n : r.groove.notes) { const int m = n.step % 16; if (n.note == 36) { if (m == 0 || m == 6 || m == 10) ++k; else ++bad; } else if (n.note == 38) { if (m == 4 || m == 12) ++sOk; else ++bad; } }
            std::printf("bateria dificil: bpm=%.1f bombos certos %d/12, caixas certas %d/8, trocadas/extra %d\n", r.bpm, k, sOk, bad);
            CHECK (k >= 11 && sOk >= 7 && bad == 0, "bombo e caixa nao se confundem");
        }
    }
    // ---- bateria
    {
        std::vector<float> x; const auto kick = synth::kick (150, 48, 0.02f, 0.3f, 0.5f, 0.3f, 0.5f, 1), snare = synth::snare (195, 0.85f, 0.12f, 0.9f, 2), hat = synth::hat (0.035f, 1.0f, true, 3), ohat = synth::hat (0.28f, 1.0f, true, 4);
        const std::set<int> K = { 0, 6, 10, 16, 22, 26 }, S = { 4, 12, 20, 28 }, O = { 14, 30 };
        for (int s = 0; s < 32; ++s)
        {
            const double t = 0.05 + s * grid;
            if (K.count (s)) mixIn (x, kick, t, 0.9f); if (S.count (s)) mixIn (x, snare, t, 0.8f);
            if (O.count (s)) mixIn (x, ohat, t, 0.35f); else if (s % 2 == 0) mixIn (x, hat, t, 0.35f);
        }
        { // análise de loop: 2 compassos a 100 bpm = 8 tempos
            std::vector<float> lp (x.begin(), x.begin() + (size_t) std::lround (32 * grid * SR)); auto li = a2m::analyseLoop (lp, SR);
            std::printf ("loop: bpm=%.2f tempos=%d conf=%.2f\n", li.bpm, li.beats, li.conf);
            CHECK (li.beats == 8 && std::fabs (li.bpm - 100.0) < 0.5, "analyseLoop: 8 tempos a 100 bpm");
            auto lh = a2m::analyseLoop (lp, SR, 100.0); CHECK (lh.beats == 8 && lh.fromName, "analyseLoop com BPM do nome");
        }
        a2m::Options o; o.style = "trap"; o.name = "teste"; auto r = a2m::convert (x, SR, o);
        if (! r.ok) std::printf ("   erro: %s\n", r.error.c_str()); CHECK (r.ok, "bateria converte"); if (r.ok)
        {
            std::printf ("bateria: tipo=%s bpm=%.1f (real 100) notas=%d compassos=%d\n", r.detectedKind.c_str(), r.bpm, r.noteCount, r.groove.bars);
            CHECK (r.detectedKind == "Bateria", "deteta bateria"); CHECK (std::fabs (r.bpm - 100.0) < 3.0, "andamento ±3 bpm"); CHECK (r.groove.bars == 2, "2 compassos");
            auto has = [&] (int note, int step) { for (auto& n : r.groove.notes) if (n.note == note && n.step == step) return true; return false; };
            int hitK = 0, hitS = 0, hitH = 0, hitO = 0, extraK = 0, extraS = 0; for (int s : K) hitK += has (36, s); for (int s : S) hitS += has (38, s) || has (39, s);
            for (int s = 0; s < 32; s += 2) if (! O.count (s)) hitH += has (42, s); for (int s : O) hitO += has (46, s);
            for (auto& n : r.groove.notes) { if (n.note == 36 && ! K.count (n.step)) ++extraK; if ((n.note == 38 || n.note == 39) && ! S.count (n.step)) ++extraS; }
            { std::printf ("   hats em falta:"); for (int s = 0; s < 32; s += 2) if (! O.count (s) && ! has (42, s)) std::printf (" %d", s); std::printf ("\n"); }
            std::printf ("   bombos %d/6 (extra %d)  caixas %d/4 (extra %d)  hats %d/14  abertos %d/2\n", hitK, extraK, hitS, extraS, hitH, hitO);
            CHECK (hitK >= 5 && extraK <= 1, "bombos"); CHECK (hitS >= 4 && extraS <= 1, "caixas"); CHECK (hitH >= 11, "hats fechados"); CHECK (hitO >= 1, "hat aberto");
        }
    }
    // ---- melodia (serra com envelope) e baixo
    for (int pass = 0; pass < 2; ++pass)
    {
        const int base = pass ? 36 : 64; const int seq[8] = { 0, 3, 5, 7, 5, 3, 7, 10 }; const int stepAt[8] = { 0, 2, 4, 6, 8, 10, 12, 14 }; const int lens[8] = { 2, 2, 2, 2, 2, 2, 2, 2 };
        std::vector<float> x; double ph = 0;
        for (int i = 0; i < 8; ++i)
        {
            const double f = 440.0 * std::pow (2.0, (base + seq[i] - 69) / 12.0); const double t0 = 0.1 + stepAt[i] * grid, d = lens[i] * grid * 0.9; const size_t a = (size_t) (t0 * SR), n = (size_t) (d * SR);
            if (x.size() < a + n + 1000) x.resize (a + n + 1000, 0.0f);
            for (size_t k = 0; k < n; ++k) { ph += f / SR; ph -= std::floor (ph); const float env = std::min (1.0f, (float) k / 200.0f) * std::min (1.0f, (float) (n - k) / 300.0f); x[a + k] += saw (ph, f) * 0.5f * env; }
        }
        a2m::Options o; o.style = "kizomba"; o.name = "m"; o.bpm = 0; auto r = a2m::convert (x, SR, o);
        CHECK (r.ok, "melodia converte"); if (r.ok)
        {
            int ok = 0; for (int i = 0; i < 8; ++i) { bool f = false; for (auto& n : r.groove.notes) if (n.note == base + seq[i] && std::abs (n.step - stepAt[i]) <= 1 && std::abs (n.len - lens[i]) <= 1) { ++ok; f = true; break; } if (! f) { std::printf ("   falta nota %d no passo %d; obtidas:", base + seq[i], stepAt[i]); for (auto& n : r.groove.notes) if (std::abs (n.step - stepAt[i]) <= 2) std::printf (" (%d@%d len %d)", n.note, n.step, n.len); std::printf ("\n"); } }
            std::printf ("%s: tipo=%s bpm=%.0f (real 100) notas=%d, certas %d/8\n", pass ? "baixo" : "melodia", r.detectedKind.c_str(), r.bpm, r.noteCount, ok);
            CHECK (ok >= 7, "notas corretas (altura+casa+duração)"); CHECK (r.detectedKind == (pass ? "Baixo" : "Melodia"), "tipo");
        }
    }
    // ---- acordes: C, Am, F, G7 — um por compasso, 100 bpm
    {
        const int roots[4] = { 0, 9, 5, 7 }; const int types[4] = { 0, 1, 0, 2 }; const std::vector<std::vector<int>> iv = { { 0, 4, 7 }, { 0, 3, 7 }, { 0, 4, 7, 10 } };
        std::vector<float> x; const double barSec = 4 * 60.0 / bpm; x.assign ((size_t) (4 * barSec * SR) + 5000, 0.0f);
        for (int c = 0; c < 4; ++c)
            for (int k : iv[(size_t) types[c]])
            {
                const double f = 261.63 * std::pow (2.0, (roots[c] + k) / 12.0); double ph = 0; const size_t a = (size_t) ((0.05 + c * barSec) * SR), n = (size_t) (barSec * 0.95 * SR);
                for (size_t i = 0; i < n; ++i) { ph += f / SR; ph -= std::floor (ph); const float env = std::min (1.0f, (float) i / 800.0f) * std::min (1.0f, (float) (n - i) / 1500.0f); x[a + i] += saw (ph, f) * 0.25f * env; }
            }
        a2m::Options o; o.style = "kizomba"; o.name = "c"; o.bpm = 100; o.kind = "Acordes"; auto r = a2m::convert (x, SR, o);
        CHECK (r.ok, "acordes convertem"); if (r.ok)
        {
            std::printf ("acordes: notas=%d compassos=%d\n", r.noteCount, r.groove.bars); int ok = 0;
            for (int c = 0; c < 4; ++c)
            {
                std::set<int> pcs; for (auto& n : r.groove.notes) if (n.step >= c * 16 && n.step < c * 16 + 16) pcs.insert (n.note % 12);
                std::set<int> want; for (int k : iv[(size_t) types[c]]) want.insert ((roots[c] + k) % 12); bool good = true; for (int p : want) good = good && pcs.count (p); if (types[c] == 2 && pcs.size() > 4) good = false; if (good) ++ok; else { std::printf ("   compasso %d: obtido:", c); for (int p : pcs) std::printf (" %d", p); std::printf ("  esperado:"); for (int p : want) std::printf (" %d", p); std::printf ("\n"); }
            }
            std::printf ("   compassos com o acorde certo: %d/4\n", ok); CHECK (ok >= 3, "acordes reconhecidos"); CHECK (r.groove.bars == 4, "4 compassos");
        }
    }
    // ---- auto: a deteção de tipo para acordes
    {
        std::vector<float> x ((size_t) (6 * SR), 0.0f); const int ch[3] = { 0, 4, 7 };
        for (int k : ch) { const double f = 220.0 * std::pow (2.0, k / 12.0); double ph = 0; for (size_t i = 0; i < x.size(); ++i) { ph += f / SR; ph -= std::floor (ph); x[i] += saw (ph, f) * 0.3f; } }
        a2m::Options o; o.bpm = 90; auto r = a2m::convert (x, SR, o); std::printf ("auto acorde sustentado: %s\n", r.ok ? r.detectedKind.c_str() : r.error.c_str()); CHECK (r.ok && r.detectedKind == "Acordes", "auto: acordes");
    }
    // ---- silêncio / curto
    { std::vector<float> x (44100 * 2, 0.0f); a2m::Options o; auto r = a2m::convert (x, SR, o); CHECK (! r.ok && ! r.error.empty(), "silêncio dá erro"); }
    std::printf ("%d verificações, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
