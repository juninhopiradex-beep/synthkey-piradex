// =============================================================================
//  SYNTHKEY by Piradex — biblioteca de samples (instrumentos, zonas) e WAV mínimo
// =============================================================================
#pragma once
#include <cstdint>
#include <cstring>
#include <cmath>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>

namespace sk
{
struct SampleData
{
    std::vector<float> l, r;      // r vazio = mono
    float sampleRate = 44100.0f;
    int frames() const { return (int) l.size(); }
    bool stereo() const { return ! r.empty(); }
};

struct Zone
{
    int data = -1;                 // índice em Library::data
    int root = 60, loKey = 0, hiKey = 127, loVel = 1, hiVel = 127;
    int loopStart = -1, loopEnd = -1;   // em amostras (−1 = sem loop)
    float gain = 1.0f;
    int rr = 0;                    // grupo de round-robin (zonas com a mesma gama alternam)
    std::string file;              // nome de ficheiro (para packs)
};

struct Instrument
{
    std::string id, name, cat, pack;
    bool fixedPitch = false;       // one-shots: não acompanham a tecla
    std::vector<Zone> zones;
};

struct Library
{
    std::vector<std::shared_ptr<SampleData>> data;   // partilhados entre bibliotecas (instâncias do plugin)
    std::vector<Instrument> inst;

    int indexOf (const std::string& id) const
    {
        for (size_t i = 0; i < inst.size(); ++i) if (inst[i].id == id) return (int) i;
        return -1;
    }
    int addData (std::shared_ptr<SampleData> d) { data.push_back (std::move (d)); return (int) data.size() - 1; }
    // junta outra biblioteca PARTILHANDO os buffers (barato: não copia áudio)
    void appendShared (const Library& o)
    {
        const int base = (int) data.size();
        for (auto& d : o.data) data.push_back (d);
        for (auto i : o.inst) { for (auto& z : i.zones) if (z.data >= 0) z.data += base; inst.push_back (std::move (i)); }
    }
    size_t sampleCount() const { size_t n = 0; for (auto& i : inst) n += i.zones.size(); return n; }
    // junta outra biblioteca (move os dados e ajusta os índices)
    void absorb (Library&& o)
    {
        const int base = (int) data.size();
        for (auto& d : o.data) data.push_back (std::move (d));
        for (auto& i : o.inst) { for (auto& z : i.zones) if (z.data >= 0) z.data += base; inst.push_back (std::move (i)); }
        o.data.clear(); o.inst.clear();
    }
};

// ---------------------------------------------------------------------------------
//  WAV: leitura (PCM 16/24/32 e float32) e escrita (PCM 24-bit)
// ---------------------------------------------------------------------------------
inline bool readWav (const uint8_t* d, size_t n, SampleData& out)
{
    if (n < 44 || std::memcmp (d, "RIFF", 4) != 0 || std::memcmp (d + 8, "WAVE", 4) != 0) return false;
    auto u16 = [&] (size_t o) { return (uint32_t) d[o] | ((uint32_t) d[o + 1] << 8); };
    auto u32 = [&] (size_t o) { return u16 (o) | (u16 (o + 2) << 16); };
    size_t pos = 12; int fmt = 0, ch = 0, bits = 0; uint32_t sr = 0; const uint8_t* pcm = nullptr; size_t pcmSize = 0;
    while (pos + 8 <= n)
    {
        const uint32_t sz = u32 (pos + 4);
        if (std::memcmp (d + pos, "fmt ", 4) == 0 && pos + 8 + 16 <= n) { fmt = (int) u16 (pos + 8); ch = (int) u16 (pos + 10); sr = u32 (pos + 12); bits = (int) u16 (pos + 22); if (fmt == 0xFFFE && sz >= 26) fmt = (int) u16 (pos + 32); }
        else if (std::memcmp (d + pos, "data", 4) == 0) { pcm = d + pos + 8; pcmSize = std::min<size_t> (sz, n - pos - 8); break; }
        pos += 8 + (size_t) sz + (sz & 1);
    }
    if (! pcm || ch < 1 || ch > 2 || sr == 0) return false;
    const int bytes = bits / 8; if (bytes < 2 || bytes > 4) return false;
    const size_t frames = pcmSize / (size_t) (bytes * ch);
    out.l.assign (frames, 0.0f); out.r.clear(); if (ch == 2) out.r.assign (frames, 0.0f);
    out.sampleRate = (float) sr;
    for (size_t i = 0; i < frames; ++i)
        for (int c = 0; c < ch; ++c)
        {
            const uint8_t* p = pcm + (i * (size_t) ch + (size_t) c) * (size_t) bytes; float v = 0;
            if (fmt == 3 && bytes == 4) { float f; std::memcpy (&f, p, 4); v = f; }
            else if (bytes == 2) v = (float) (int16_t) (p[0] | (p[1] << 8)) / 32768.0f;
            else if (bytes == 3) { int32_t s = (int32_t) (p[0] | (p[1] << 8) | (p[2] << 16)); if (s & 0x800000) s |= ~0xFFFFFF; v = (float) s / 8388608.0f; }
            else { int32_t s; std::memcpy (&s, p, 4); v = (float) ((double) s / 2147483648.0); }
            (c ? out.r : out.l)[i] = v;
        }
    return true;
}

inline std::vector<uint8_t> writeWav24 (const SampleData& s)
{
    const int ch = s.stereo() ? 2 : 1; const uint32_t frames = (uint32_t) s.frames(), dataBytes = frames * (uint32_t) ch * 3;
    std::vector<uint8_t> o; o.reserve (44 + dataBytes);
    auto w32 = [&] (uint32_t v) { for (int i = 0; i < 4; ++i) o.push_back ((uint8_t) (v >> (8 * i))); };
    auto w16 = [&] (uint32_t v) { o.push_back ((uint8_t) v); o.push_back ((uint8_t) (v >> 8)); };
    o.insert (o.end(), { 'R', 'I', 'F', 'F' }); w32 (36 + dataBytes); o.insert (o.end(), { 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ' });
    w32 (16); w16 (1); w16 ((uint32_t) ch); w32 ((uint32_t) s.sampleRate); w32 ((uint32_t) s.sampleRate * (uint32_t) ch * 3); w16 ((uint32_t) ch * 3); w16 (24);
    o.insert (o.end(), { 'd', 'a', 't', 'a' }); w32 (dataBytes);
    for (uint32_t i = 0; i < frames; ++i)
        for (int c = 0; c < ch; ++c)
        {
            const float f = (c ? s.r : s.l)[i];
            int32_t v = (int32_t) std::lrint (std::max (-1.0f, std::min (1.0f, f)) * 8388607.0f);
            o.push_back ((uint8_t) v); o.push_back ((uint8_t) (v >> 8)); o.push_back ((uint8_t) (v >> 16));
        }
    return o;
}
} // namespace sk
