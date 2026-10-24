// =============================================================================
//  SYNTHKEY by Piradex — biblioteca de grooves e progressões por estilo + reprodutor interno
//  Os padrões estão escritos em texto compacto (ver GroovesData.inc) e são todos em Dó (C);
//  a tonalidade é transposta ao tocar / arrastar para o DAW.
//    Bateria : "k:x...x..." um caminho por instrumento (k bombo, r rim, s caixa, c palmas, t tom, h hat,
//              m shaker, o hat aberto, g conga grave, j conga aguda, b bongo); x forte, o fraco, . vazio.
//    Notas   : "passo:nota+nota:duração:velocidade" (passo em semicolcheias; nota em semitons a partir de Dó).
// =============================================================================
#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

namespace sk
{
struct GNote { int step = 0; int note = 60; int len = 2; int vel = 100; };

struct Groove
{
    std::string style, kind, name;
    int bpm = 120, bars = 2; float swing = 0.0f;
    bool user = false; std::string file;      // grooves do utilizador (editados / importados de MIDI) vivem numa pasta própria
    std::vector<GNote> notes;                 // já com a altura absoluta (Dó), prontas a transpor
    int totalSteps() const { return bars * 16; }
};

namespace groovedata
{
struct Spec { const char* style; const char* kind; const char* name; int bpm; int bars; float swing; const char* dsl; };
#define TXT(s) s
inline const Spec kSpecs[] = {
#include "GroovesData.inc"
};
#undef TXT

inline int drumNote (char c)
{
    switch (c) { case 'k': return 36; case 'r': return 37; case 's': return 38; case 'c': return 39; case 't': return 41; case 'h': return 42;
                 case 'm': return 44; case 'o': return 46; case 'g': return 43; case 'j': return 45; case 'b': return 47; default: return -1; }
}
inline int baseFor (const std::string& kind) { return kind == "Baixo" ? 36 : kind == "Acordes" ? 60 : 72; }

inline Groove parse (const Spec& s)
{
    Groove g; g.style = s.style; g.kind = s.kind; g.name = s.name; g.bpm = s.bpm; g.bars = s.bars; g.swing = s.swing;
    const std::string d = s.dsl; size_t i = 0;
    while (i < d.size())
    {
        while (i < d.size() && d[i] == ' ') ++i;
        size_t j = d.find (' ', i); if (j == std::string::npos) j = d.size();
        const std::string tok = d.substr (i, j - i); i = j;
        if (tok.empty()) continue;
        if (g.kind == "Bateria")
        {
            const int nn = drumNote (tok[0]); if (nn < 0 || tok.size() < 3 || tok[1] != ':') continue;
            for (size_t k = 2; k < tok.size(); ++k)
            {
                const char c = tok[k]; if (c == '.') continue;
                GNote n; n.step = (int) (k - 2); n.note = nn; n.len = 1; n.vel = c == 'x' ? 105 : 62; g.notes.push_back (n);
            }
        }
        else
        {
            // passo:notas:dur:vel
            std::vector<std::string> f; size_t a = 0;
            while (true) { size_t b = tok.find (':', a); f.push_back (tok.substr (a, b == std::string::npos ? std::string::npos : b - a)); if (b == std::string::npos) break; a = b + 1; }
            if (f.size() < 3) continue;
            const int step = std::atoi (f[0].c_str()), len = std::max (1, std::atoi (f[2].c_str())), vel = f.size() > 3 ? std::atoi (f[3].c_str()) : 100;
            size_t p = 0; const std::string& ns = f[1];
            while (p <= ns.size())
            {
                size_t q = ns.find ('+', p); if (q == std::string::npos) q = ns.size();
                if (q > p) { GNote n; n.step = step; n.len = len; n.vel = vel; n.note = baseFor (g.kind) + std::atoi (ns.substr (p, q - p).c_str()); g.notes.push_back (n); }
                p = q + 1;
            }
        }
    }
    std::stable_sort (g.notes.begin(), g.notes.end(), [] (const GNote& x, const GNote& y) { return x.step < y.step; });
    return g;
}
}

inline const std::vector<Groove>& allGrooves()
{
    static const std::vector<Groove> all = [] { std::vector<Groove> v; for (auto& s : groovedata::kSpecs) v.push_back (groovedata::parse (s)); return v; }();
    return all;
}

// -------------------------------------------------------------------------------------------------
//  Reprodutor interno: gera eventos de nota (com offset em amostras) para o bloco de áudio.
//  Chamado pela thread de áudio; start/stop vêm da interface (protegidos por um pequeno lock).
// -------------------------------------------------------------------------------------------------
struct GMidiEvent { int offset; int note; int vel; bool on; };

class GroovePlayer
{
public:
    void start (const Groove& g, int transpose, double bpmOverride)
    {
        Lock l (flag); build (g, transpose);
        bpm = bpmOverride > 0 ? bpmOverride : g.bpm; pos = 0; cursor = 0; wantStart = true; running.store (true);
    }
    // altera o groove a tocar sem voltar ao início (edição ao vivo)
    void update (const Groove& g, int transpose)
    {
        Lock l (flag); if (! running.load()) return;
        build (g, transpose);
        if (pos >= (double) loopSteps) pos = 0.0;
        cursor = 0; while (cursor < evs.size() && evs[cursor].t < pos) ++cursor;
        wantStart = true;                                   // largou as notas que ficaram presas
    }
    void stop() { Lock l (flag); wantStop = true; running.store (false); }
    bool isRunning() const { return running.load(); }
    double stepPosition() const { return posShown.load(); }
    void setBpm (double b) { Lock l (flag); if (b > 20) bpm = b; }

    // devolve os eventos do bloco (n amostras a sr Hz)
    void process (int n, double sr, std::vector<GMidiEvent>& out)
    {
        if (flag.test_and_set (std::memory_order_acquire)) return;          // a UI está a mexer: salta este bloco
        if (wantStop) { for (int i = 0; i < 128; ++i) if (active[(size_t) i]) out.push_back ({ 0, i, 0, false }); active.fill (false); wantStop = false; flag.clear (std::memory_order_release); return; }
        if (! running.load() || evs.empty()) { flag.clear (std::memory_order_release); return; }
        if (wantStart) { for (int i = 0; i < 128; ++i) if (active[(size_t) i]) out.push_back ({ 0, i, 0, false }); active.fill (false); wantStart = false; }
        const double sps = bpm * 4.0 / (60.0 * sr);          // passos por amostra
        const double p0 = pos, p1 = pos + sps * n;
        auto emit = [&] (double a, double b, double shift)
        {
            while (cursor < evs.size() && evs[cursor].t < a) ++cursor;
            while (cursor < evs.size() && evs[cursor].t < b)
            {
                const Ev& e = evs[cursor++]; const int off = std::max (0, std::min (n - 1, (int) ((e.t + shift - p0) / sps)));
                out.push_back ({ off, e.note, e.vel, e.on }); active[(size_t) e.note] = e.on;
            }
        };
        if (p1 < (double) loopSteps) { emit (p0, p1, 0.0); pos = p1; }
        else
        {
            emit (p0, (double) loopSteps, 0.0);
            for (int i = 0; i < 128; ++i) if (active[(size_t) i]) { out.push_back ({ std::max (0, std::min (n - 1, (int) (((double) loopSteps - p0) / sps)) ), i, 0, false }); active[(size_t) i] = false; }
            cursor = 0; pos = p1 - (double) loopSteps; emit (0.0, pos, (double) loopSteps);
        }
        posShown.store (pos);
        flag.clear (std::memory_order_release);
    }
private:
    struct Ev { double t; int note; int vel; bool on; };
    void build (const Groove& g, int transpose)
    {
        std::vector<Ev> e; const int total = g.totalSteps();
        for (auto& n : g.notes)
        {
            const int note = juce_clamp (n.note + (g.kind == "Bateria" ? 0 : transpose), 0, 127);
            const double sw = (n.step % 2 == 1) ? g.swing : 0.0;
            e.push_back ({ n.step + sw, note, n.vel, true });
            e.push_back ({ std::min ((double) total - 0.01, (double) (n.step + n.len) - 0.02 + sw), note, 0, false });
        }
        std::stable_sort (e.begin(), e.end(), [] (const Ev& a, const Ev& b) { return a.t < b.t; });
        evs = std::move (e); loopSteps = total;
    }
    struct Lock { std::atomic_flag& f; explicit Lock (std::atomic_flag& x) : f (x) { while (f.test_and_set (std::memory_order_acquire)) {} } ~Lock() { f.clear (std::memory_order_release); } };
    static int juce_clamp (int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
    std::vector<Ev> evs; size_t cursor = 0; int loopSteps = 32; double bpm = 120.0, pos = 0.0;
    bool wantStart = false, wantStop = false; std::array<bool, 128> active {};
    std::atomic<bool> running { false }; std::atomic<double> posShown { 0.0 };
};
}
