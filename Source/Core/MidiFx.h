// =============================================================================
//  SYNTHKEY by Piradex — processamento de MIDI antes das vozes:
//  escala (forçar notas), acordes e arpejador com padrões por estilo.
// =============================================================================
#pragma once
#include "ParamTable.h"
#include "Dsp.h"
#include <algorithm>
#include <cstdint>
#include <vector>

namespace sk
{
struct NoteEvent { int offset; bool on; int note; float vel; };

inline const int kScaleIntervals[11][12] = {
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 },       // Desligada (cromática)
    { 0, 2, 4, 5, 7, 9, 11, -1, -1, -1, -1, -1 },    // Maior
    { 0, 2, 3, 5, 7, 8, 10, -1, -1, -1, -1, -1 },    // Menor natural
    { 0, 2, 3, 5, 7, 9, 10, -1, -1, -1, -1, -1 },    // Dórico
    { 0, 1, 3, 5, 7, 8, 10, -1, -1, -1, -1, -1 },    // Frígio
    { 0, 2, 4, 6, 7, 9, 11, -1, -1, -1, -1, -1 },    // Lídio
    { 0, 2, 4, 5, 7, 9, 10, -1, -1, -1, -1, -1 },    // Mixolídio
    { 0, 2, 3, 5, 7, 8, 11, -1, -1, -1, -1, -1 },    // Menor harmónica
    { 0, 2, 4, 7, 9, -1, -1, -1, -1, -1, -1, -1 },   // Pentatónica maior
    { 0, 3, 5, 7, 10, -1, -1, -1, -1, -1, -1, -1 },  // Pentatónica menor
    { 0, 3, 5, 6, 7, 10, -1, -1, -1, -1, -1, -1 } }; // Blues

struct ArpStep { int8_t oct; float vel; float gate; uint8_t flags; };   // flags: 1 pausa, 2 ligação, 4 ratchet×2, 8 ratchet×3
inline constexpr uint8_t AF_REST = 1, AF_TIE = 2, AF_R2 = 4, AF_R3 = 8;

struct ArpPattern { int len; ArpStep s[16]; };

// padrões inspirados nos estilos dos packs (índice = valor de ARP_PATTERN)
inline const ArpPattern kArpPatterns[8] = {
    { 1, { { 0, 1.0f, 1.0f, 0 } } },                                                                                           // Liso
    { 16, { { 0, 1.0f, .4f, 0 }, { 0, .5f, .3f, 0 }, { 0, .7f, .3f, 0 }, { 0, .45f, .3f, 0 }, { 0, .95f, .4f, 0 }, { 0, .5f, .3f, 0 }, { 0, .65f, .3f, AF_R3 }, { 0, .5f, .3f, 0 },
            { 0, 1.0f, .4f, 0 }, { 0, .5f, .3f, 0 }, { 0, .7f, .3f, 0 }, { 0, .45f, .3f, AF_R2 }, { 0, .95f, .4f, 0 }, { 0, .5f, .3f, 0 }, { 0, .65f, .3f, 0 }, { 0, .55f, .3f, AF_R3 } } },    // Trap Hi-Hat
    { 16, { { 0, 1.0f, .7f, 0 }, { 0, 0, 0, AF_REST }, { 0, .6f, .35f, 0 }, { 0, 0, 0, AF_REST }, { 0, .8f, .5f, 0 }, { 0, .5f, .3f, 0 }, { 0, 0, 0, AF_REST }, { 1, .7f, .4f, 0 },
            { 0, .95f, .7f, 0 }, { 0, 0, 0, AF_REST }, { 0, .6f, .35f, 0 }, { 0, .5f, .3f, 0 }, { 0, 0, 0, AF_REST }, { 0, .8f, .5f, 0 }, { 0, .5f, .3f, 0 }, { 0, 0, 0, AF_REST } } },        // R&B Quebrado
    { 16, { { 0, 1.0f, .6f, 0 }, { 0, .4f, .3f, 0 }, { 0, 0, 0, AF_REST }, { 0, .9f, .5f, 0 }, { 0, 0, 0, AF_REST }, { 0, .5f, .3f, 0 }, { 0, .95f, .5f, 0 }, { 0, 0, 0, AF_REST },
            { 1, .7f, .4f, 0 }, { 0, 0, 0, AF_REST }, { 0, .9f, .5f, 0 }, { 0, .4f, .3f, 0 }, { 0, .95f, .6f, 0 }, { 0, 0, 0, AF_REST }, { 0, .5f, .3f, 0 }, { 0, .6f, .3f, 0 } } },           // Afrobeat Sincopado
    { 16, { { 0, 1.0f, 1.0f, 0 }, { 0, 0, 0, AF_TIE }, { 0, 0, 0, AF_REST }, { 0, .5f, .35f, 0 }, { 0, 0, 0, AF_REST }, { 0, 0, 0, AF_REST }, { 0, .85f, .6f, 0 }, { 0, 0, 0, AF_TIE },
            { 0, .95f, 1.0f, 0 }, { 0, 0, 0, AF_REST }, { 0, 0, 0, AF_REST }, { 0, .5f, .35f, 0 }, { 0, 0, 0, AF_REST }, { 0, 0, 0, AF_REST }, { -1, .8f, .6f, 0 }, { 0, 0, 0, AF_REST } } },    // Amapiano Groove
    { 16, { { 0, 1.0f, 1.0f, 0 }, { 0, 0, 0, AF_TIE }, { 0, .6f, .5f, 0 }, { 0, 0, 0, AF_REST }, { 0, .9f, .8f, 0 }, { 0, 0, 0, AF_TIE }, { 0, 0, 0, AF_REST }, { 0, .6f, .5f, 0 },
            { 0, 1.0f, 1.0f, 0 }, { 0, 0, 0, AF_TIE }, { 0, .6f, .5f, 0 }, { 0, 0, 0, AF_REST }, { 0, .9f, .8f, 0 }, { 0, 0, 0, AF_TIE }, { 0, 0, 0, AF_REST }, { 0, 0, 0, AF_REST } } },          // Drill Slides
    { 8, { { 0, .9f, 1.0f, 0 }, { 0, .5f, .9f, 0 }, { 0, .65f, .9f, 0 }, { 0, .5f, .9f, 0 }, { 0, .85f, 1.0f, 0 }, { 0, .5f, .9f, 0 }, { 0, .65f, .9f, 0 }, { 0, .5f, .9f, 0 } } },                    // Kizomba Zouk
    { 16, { { 0, 1.0f, .5f, 0 }, { 0, .6f, .4f, 0 }, { 0, .8f, .4f, 0 }, { 0, .6f, .4f, 0 }, { 0, .95f, .5f, 0 }, { 0, .6f, .4f, 0 }, { 0, .8f, .4f, 0 }, { 1, .7f, .4f, 0 },
            { 0, 1.0f, .5f, 0 }, { 0, .6f, .4f, 0 }, { 0, .8f, .4f, 0 }, { 0, .6f, .4f, 0 }, { 0, .95f, .5f, 0 }, { 0, .6f, .4f, 0 }, { 1, .85f, .4f, 0 }, { 1, .7f, .4f, AF_R2 } } }           // Kuduro
};

class MidiFx
{
public:
    void prepare (float sampleRate) { sr = sampleRate; reset(); }
    void reset()
    {
        held.clear(); sounding.clear(); pending.clear(); chordMap.clear();
        physicalDown = 0; arpActive = false; lastStepIdx = -1; internalBeat = 0; stepCounter = 0; noteCounter = 0; pingDir = 1; pingIdx = 0; absPos = 0;
        rng.seed (0xBEEF);
    }
    void setParams (const float* p) { prm = p; }

    // chamadas de entrada, pela ordem em que chegam
    void noteOn (int note, float vel, int offset) { inq.push_back ({ offset, true, note, vel }); }
    void noteOff (int note, int offset) { inq.push_back ({ offset, false, note, 0.0f }); }
    void allNotesOff() { inq.push_back ({ 0, false, -1, 0.0f }); }

    // processa um bloco: consome a fila de entrada e escreve os eventos de saída (ordenados)
    void process (int n, double bpm, double hostPpq, bool hostPlaying, std::vector<NoteEvent>& out)
    {
        out.clear();
        const bool arpOn = prm[P_ARP_ON] > 0.5f;
        if (arpOn != arpActive)
        {
            flushSounding (out, 0);
            arpActive = arpOn; lastStepIdx = -1; stepCounter = 0; noteCounter = 0; internalBeat = 0;
            pending.clear();
            if (arpOn) { /* as notas seguradas já estão em 'held' */ }
        }

        for (const auto& e : inq)
        {
            if (e.note < 0) { flushSounding (out, e.offset); held.clear(); physicalDown = 0; pending.clear(); continue; }
            if (e.on) handleOn (e, out); else handleOff (e, out);
        }
        inq.clear();

        if (arpActive) runArp (n, bpm, hostPpq, hostPlaying);

        // eventos do arpejador agendados para este bloco
        std::sort (pending.begin(), pending.end(), [] (const Pend& a, const Pend& b) { return a.at < b.at || (a.at == b.at && ! a.on && b.on); });
        size_t k = 0;
        for (; k < pending.size() && pending[k].at < absPos + n; ++k)
        {
            const int off = (int) std::max<int64_t> (0, pending[k].at - absPos);
            out.push_back ({ off, pending[k].on, pending[k].note, pending[k].vel });
            if (pending[k].on) sounding.push_back (pending[k].note);
            else { auto it = std::find (sounding.begin(), sounding.end(), pending[k].note); if (it != sounding.end()) sounding.erase (it); }
        }
        pending.erase (pending.begin(), pending.begin() + (long) k);
        absPos += n;
        std::stable_sort (out.begin(), out.end(), [] (const NoteEvent& a, const NoteEvent& b) { return a.offset < b.offset; });
    }

    // utilitários (também usados em testes)
    static int snapToScale (int note, int key, int scale)
    {
        if (scale <= 0) return note;
        int best = note, bd = 99;
        for (int d = -6; d <= 6; ++d)
        {
            const int cand = note + d, pc = ((cand - key) % 12 + 12) % 12;
            bool inScale = false;
            for (int i = 0; i < 12 && kScaleIntervals[scale][i] >= 0; ++i) if (kScaleIntervals[scale][i] == pc) inScale = true;
            if (inScale && (std::abs (d) < bd || (std::abs (d) == bd && d > 0))) { bd = std::abs (d); best = cand; }
        }
        return best;
    }
    static int chordNotes (int type, int root, int key, int scale, int out[8])
    {
        static const int fixedI[12][5] = { { 0 }, { 0, 12 }, { 0, 7 }, { 0, 4, 7 }, { 0, 3, 7 }, { 0, 4, 7, 11 }, { 0, 3, 7, 10 }, { 0, 4, 7, 10 }, { 0, 4, 7, 11, 14 }, { 0, 3, 7, 10, 14 }, { 0, 2, 7 }, { 0, 5, 7 } };
        static const int fixedN[12] = { 1, 2, 2, 3, 3, 4, 4, 4, 5, 5, 3, 3 };
        if (type <= 0) { out[0] = root; return 1; }
        if (type <= 11) { for (int i = 0; i < fixedN[type]; ++i) out[i] = root + fixedI[type][i]; return fixedN[type]; }
        // diatónico: empilha terças na escala (se desligada, usa a maior)
        const int sc = scale <= 0 ? 1 : scale;
        int degs[12], nd = 0;
        for (int i = 0; i < 12 && kScaleIntervals[sc][i] >= 0; ++i) degs[nd++] = kScaleIntervals[sc][i];
        const int rootPc = ((root - key) % 12 + 12) % 12;
        int idx = 0, bd = 99;
        for (int i = 0; i < nd; ++i) { const int d = std::abs (degs[i] - rootPc); if (d < bd) { bd = d; idx = i; } }
        const int count = type == 12 ? 3 : (type == 13 ? 4 : 5);
        for (int i = 0; i < count; ++i)
        {
            const int di = idx + i * 2;
            out[i] = i == 0 ? root : root + (((di / nd) * 12 + degs[di % nd]) - degs[idx]);
        }
        return count;
    }

private:
    struct Pend { int64_t at; bool on; int note; float vel; };
    struct Held { int note; float vel; };
    float sr = 44100.0f; const float* prm = nullptr;
    std::vector<NoteEvent> inq;
    std::vector<Held> held;
    std::vector<int> sounding;
    std::vector<Pend> pending;
    struct ChordEntry { int root; std::vector<int> notes; };
    std::vector<ChordEntry> chordMap;
    int physicalDown = 0; bool arpActive = false;
    int64_t lastStepIdx = -1, absPos = 0; double internalBeat = 0; int stepCounter = 0, noteCounter = 0, pingDir = 1, pingIdx = 0;
    Rng rng;

    int prepNote (int note) const
    {
        note += (int) prm[P_TRANSPOSE] * 0;   // a transposição é feita no motor
        if (prm[P_SCALE_SNAP] > 0.5f) note = snapToScale (note, (int) prm[P_SCALE_KEY], (int) prm[P_SCALE_TYPE]);
        return std::min (127, std::max (0, note));
    }
    void flushSounding (std::vector<NoteEvent>& out, int offset)
    {
        for (int n : sounding) out.push_back ({ offset, false, n, 0.0f });
        sounding.clear(); chordMap.clear();
    }
    void handleOn (const NoteEvent& e, std::vector<NoteEvent>& out)
    {
        ++physicalDown;
        const int root = prepNote (e.note);
        if (arpActive)
        {
            if (prm[P_ARP_LATCH] > 0.5f && physicalDown == 1) held.clear();
            held.push_back ({ root, e.vel });
            return;
        }
        int ns[8]; const int cnt = chordNotes ((int) prm[P_CHORD_TYPE], root, (int) prm[P_SCALE_KEY], (int) prm[P_SCALE_TYPE], ns);
        ChordEntry ce { e.note, {} };
        for (int i = 0; i < cnt; ++i)
        {
            const int nn = std::min (127, std::max (0, ns[i]));
            ce.notes.push_back (nn);
            out.push_back ({ e.offset, true, nn, e.vel });
            sounding.push_back (nn);
        }
        chordMap.push_back (std::move (ce));
    }
    void handleOff (const NoteEvent& e, std::vector<NoteEvent>& out)
    {
        if (physicalDown > 0) --physicalDown;
        if (arpActive)
        {
            if (prm[P_ARP_LATCH] > 0.5f) return;
            const int root = prepNote (e.note);
            auto it = std::find_if (held.begin(), held.end(), [root] (const Held& h) { return h.note == root; });
            if (it != held.end()) held.erase (it);
            return;
        }
        for (auto it = chordMap.begin(); it != chordMap.end(); ++it)
            if (it->root == e.note)
            {
                for (int nn : it->notes)
                {
                    out.push_back ({ e.offset, false, nn, 0.0f });
                    auto s = std::find (sounding.begin(), sounding.end(), nn); if (s != sounding.end()) sounding.erase (s);
                }
                chordMap.erase (it);
                break;
            }
    }

    // ---- arpejador ----------------------------------------------------------------------
    void buildList (std::vector<Held>& list) const
    {
        list = held;
        const int mode = (int) prm[P_ARP_MODE];
        if (mode != 5) std::sort (list.begin(), list.end(), [] (const Held& a, const Held& b) { return a.note < b.note; });
        const int octs = std::max (1, (int) prm[P_ARP_OCT]);
        std::vector<Held> all;
        for (int o = 0; o < octs; ++o) for (auto h : list) { h.note += 12 * o; if (h.note <= 127) all.push_back (h); }
        list = all;
    }
    int pickIndex (int mode, int size)
    {
        if (size <= 0) return 0;
        switch (mode)
        {
            case 0: return noteCounter % size;
            case 1: return size - 1 - (noteCounter % size);
            case 2: case 3:
            {
                if (size == 1) return 0;
                const int period = 2 * (size - 1); const int k = noteCounter % period;
                const int up = k < size ? k : period - k;
                return mode == 2 ? up : size - 1 - up;
            }
            case 4: return (int) (rng.uni() * (float) size) % size;
            default: return noteCounter % size;   // ordem tocada
        }
    }
    void schedule (int64_t at, int note, float vel, int64_t len)
    {
        pending.push_back ({ at, true, note, vel });
        pending.push_back ({ std::max<int64_t> (at + 1, at + len), false, note, 0.0f });
    }
    void runArp (int n, double bpm, double hostPpq, bool hostPlaying)
    {
        if (held.empty()) { internalBeat = 0; lastStepIdx = -1; return; }
        const double bps = bpm / 60.0, beatsPerSample = bps / (double) sr;
        const double stepBeats = (double) kDivBeats[std::min (15, std::max (0, (int) prm[P_ARP_RATE]))];
        const double ppq0 = hostPlaying ? hostPpq : internalBeat;
        if (hostPlaying) { const int64_t cur = (int64_t) std::floor (ppq0 / stepBeats); if (lastStepIdx > cur) lastStepIdx = cur - 1; }
        const double stepLen = stepBeats / beatsPerSample;
        const int pat = std::min (7, std::max (0, (int) prm[P_ARP_PATTERN]));
        const ArpPattern& ap = kArpPatterns[pat];
        const int mode = (int) prm[P_ARP_MODE];
        const float gate = prm[P_ARP_GATE], swing = prm[P_ARP_SWING];
        std::vector<Held> list; buildList (list);
        if (list.empty()) return;

        for (;;)
        {
            const int64_t idx = lastStepIdx + 1;
            const double boundary = (double) idx * stepBeats;
            const double offD = (boundary - ppq0) / beatsPerSample;
            if (offD >= (double) n) break;
            lastStepIdx = idx;
            int64_t at = absPos + (int64_t) std::ceil (std::max (0.0, offD));
            if (idx & 1) at += (int64_t) (swing * 0.66 * stepLen);
            const ArpStep& st = ap.s[stepCounter % ap.len];
            ++stepCounter;
            if (st.flags & AF_REST) continue;
            if (st.flags & AF_TIE)
            {
                for (auto& p : pending) if (! p.on && p.at > at - 1 && p.at < at + (int64_t) stepLen * 2) p.at += (int64_t) stepLen;   // prolonga a nota anterior
                continue;
            }
            const int reps = (st.flags & AF_R3) ? 3 : ((st.flags & AF_R2) ? 2 : 1);
            const int64_t subLen = (int64_t) (stepLen / (double) reps);
            std::vector<int> picks;
            if (mode == 6) { for (size_t i = 0; i < list.size() && i < 4; ++i) picks.push_back ((int) i); }
            else picks.push_back (pickIndex (mode, (int) list.size()));
            for (int r = 0; r < reps; ++r)
                for (int pi : picks)
                {
                    const Held& h = list[(size_t) pi];
                    const int note = std::min (127, std::max (0, h.note + 12 * st.oct));
                    schedule (at + (int64_t) r * subLen, note, clampf (h.vel * st.vel, 0.05f, 1.0f), std::max<int64_t> (2, (int64_t) ((double) subLen * (double) gate * (double) st.gate)));
                }
            ++noteCounter;
        }
        if (! hostPlaying) internalBeat += (double) n * beatsPerSample;
    }
};
} // namespace sk
