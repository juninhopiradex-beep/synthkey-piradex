#include <cstdio>
#include "Core/Grooves.h"
using namespace sk;
static int fails = 0;
#define CHECK(c, m) do { if (!(c)) { std::printf ("FALHOU: %s\n", m); ++fails; } } while (0)
int main()
{
    const auto& all = allGrooves(); std::printf ("%zu grooves\n", all.size());
    CHECK (all.size() >= 50, "pelo menos 50 grooves");
    for (auto& g : all)
    {
        CHECK (! g.notes.empty(), (g.name + ": vazio").c_str());
        for (auto& n : g.notes)
        {
            CHECK (n.note >= 0 && n.note < 128, (g.name + ": nota fora do intervalo").c_str());
            CHECK (n.step >= 0 && n.step < g.totalSteps(), (g.name + ": passo fora do compasso").c_str());
            CHECK (n.step + n.len <= g.totalSteps() + 1, (g.name + ": nota passa do fim").c_str());
            CHECK (n.vel > 0 && n.vel < 128, (g.name + ": velocidade").c_str());
        }
    }
    // reprodutor: toca 4 compassos a 120 bpm em blocos de 512 e confere que há tantos note-off como note-on
    for (auto& g : all)
    {
        GroovePlayer pl; pl.start (g, 0, 0); std::vector<GMidiEvent> ev; int on = 0, off = 0;
        const double sr = 48000.0; const int blocks = (int) (sr * 60.0 / g.bpm * 4.0 * g.bars * 2 / 512) + 4;
        for (int b = 0; b < blocks; ++b) { ev.clear(); pl.process (512, sr, ev); for (auto& e : ev) { CHECK (e.offset >= 0 && e.offset < 512, "offset"); (e.on ? on : off)++; } }
        pl.stop(); ev.clear(); pl.process (512, sr, ev); for (auto& e : ev) if (! e.on) ++off;
        CHECK (on > 0, (g.name + ": não tocou nada").c_str());
        CHECK (on == off, (g.name + ": notas por desligar").c_str());
    }
    std::printf (fails ? "%d falhas\n" : "tudo OK\n", fails); return fails ? 1 : 0;
}
