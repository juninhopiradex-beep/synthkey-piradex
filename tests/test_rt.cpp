// Tempo real (auditoria 24/11/2026): o MidiFx (escala, acordes, arpejador) nao aloca memoria na thread de audio.
//   g++ -O2 -std=c++17 -ISource tests/test_rt.cpp -o test_rt && ./test_rt
#include "Core/MidiFx.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

static thread_local bool g_track = false; static thread_local long g_allocs = 0;
void* operator new (std::size_t n) { if (g_track) ++g_allocs; if (void* p = std::malloc (n ? n : 1)) return p; throw std::bad_alloc(); }
void* operator new[] (std::size_t n) { if (g_track) ++g_allocs; if (void* p = std::malloc (n ? n : 1)) return p; throw std::bad_alloc(); }
void operator delete (void* p) noexcept { std::free (p); }
void operator delete[] (void* p) noexcept { std::free (p); }
void operator delete (void* p, std::size_t) noexcept { std::free (p); }
void operator delete[] (void* p, std::size_t) noexcept { std::free (p); }

using namespace sk;
static float P[P_COUNT];
static int idOf (const char* id) { for (int i = 0; i < P_COUNT; ++i) if (std::strcmp (kParams[i].id, id) == 0) return i; return -1; }

int main()
{
    int fails = 0;
    for (int i = 0; i < P_COUNT; ++i) P[i] = kParams[i].def;
    MidiFx fx; fx.prepare (48000.0f); fx.setParams (P);
    std::vector<NoteEvent> out; out.reserve (512);
    auto play = [&] (int blocks, unsigned seed)
    {
        unsigned r = seed;
        for (int b = 0; b < blocks; ++b)
        {
            r = r * 1103515245u + 12345u;
            if ((r >> 16) % 3 == 0) fx.noteOn (48 + (int) ((r >> 8) % 24), 0.8f, (int) ((r >> 4) % 512));
            if ((r >> 12) % 4 == 0) fx.noteOff (48 + (int) ((r >> 20) % 24), (int) ((r >> 2) % 512));
            fx.process (512, 124.0, b * 512.0 * 124.0 / 60.0 / 48000.0, true, out);
        }
    };
    struct Case { const char* name; float arp, chord, mode, oct; };
    const Case cases[] = { { "acordes (7.a)", 0, 13, 0, 1 }, { "arpejador up 3 oitavas", 1, 0, 0, 3 }, { "arpejador acorde (modo 6)", 1, 0, 6, 2 }, { "arpejador aleatorio", 1, 0, 4, 4 } };
    for (const auto& c : cases)
    {
        P[P_ARP_ON] = c.arp; P[P_CHORD_TYPE] = c.chord; P[P_ARP_MODE] = c.mode; P[P_ARP_OCT] = c.oct;
        if (const int g = idOf ("arpRate"); g >= 0) P[g] = 12;      // passos rapidos
        play (400, 1);                                                // aquece (os vectores ja estao reservados)
        g_allocs = 0; g_track = true; play (2000, 7); g_track = false;
        std::printf ("%s  %-28s alocacoes em 2000 blocos: %ld\n", g_allocs == 0 ? "ok    " : "FALHOU", c.name, g_allocs);
        if (g_allocs != 0) ++fails;
        fx.allNotesOff(); fx.process (512, 124.0, 0.0, false, out);
    }
    std::printf ("%s (%d falhas)\n", fails ? "FALHOU" : "TUDO OK", fails);
    return fails ? 1 : 0;
}
