// =============================================================================
//  SYNTHKEY by Piradex — motor Espectral (síntese aditiva por tabela)
//
//  O espectro (amplitude de cada harmónica) é descrito por poucos controlos — inclinação, formante,
//  pente, par/ímpar, dispersão de fase — e varrido por 32 frames: a posição do formante anda de
//  SP_FORM até SP_FORM·4^SWEEP. Os frames passam por buildMips (mesmas tabelas band-limited do motor
//  Wavetable), por isso o aliasing e o custo por voz são os de uma wavetable; a POSIÇÃO (A_WTPOS,
//  modulável por voz) percorre o espectro.
//
//  A tabela reconstrói-se de forma INCREMENTAL (poucos frames por bloco) num buffer de reserva e só
//  troca quando está completa — a thread de áudio nunca espera nem aloca.
// =============================================================================
#pragma once
#include "Wavetables.h"
#include <cstring>

namespace sk
{
struct SpectralParams
{
    float tilt = 1.0f, form = 8.0f, fq = 0.4f, comb = 0.0f, odd = 0.0f, sweep = 0.5f, smear = 0.0f;
    uint32_t hash() const
    {
        const float v[7] = { tilt, form, fq, comb, odd, sweep, smear };
        uint32_t h = 2166136261u;
        for (float x : v) { uint32_t b; std::memcpy (&b, &x, 4); h = (h ^ b) * 16777619u; }
        return h ? h : 1u;
    }
};

class SpectralBank
{
public:
    static constexpr int kHarm = 256;          // harmónicas geradas (o resto fica a zero; chega para qualquer nota útil)

    SpectralBank() { for (auto& b : buf) b.assign ((size_t) Wavetables::kFrames * Wavetables::kMips * Wavetables::kStride, 0.0f);  cbuf.resize ((size_t) Wavetables::kSize); }

    bool ready() const { return front >= 0; }
    const float* cycle (int frame, int mip) const
    {
        if (front < 0) return nullptr;
        return buf[(size_t) front].data() + (((size_t) frame * Wavetables::kMips + (size_t) mip) * Wavetables::kStride);
    }

    // espectro de um frame (0..kFrames-1)
    static void frameSpectrum (const SpectralParams& p, int frame, float* amp, float* phs)
    {
        for (int k = 0; k <= kHarm; ++k) { amp[k] = 0.0f; phs[k] = 0.0f; }
        const float m = (float) frame / (float) (Wavetables::kFrames - 1);
        const float centre = std::max (1.0f, p.form * std::exp2 (p.sweep * m * 4.0f));          // sweep 1 = 4 oitavas de harmónicas
        const float w = 1.3f - 1.18f * p.fq;                                                   // largura do formante (em ln k)
        const float lnc = std::log (centre);
        const float period = std::max (2.0f, centre * 0.5f);
        const int kmax = kHarm;
        for (int k = 1; k <= kmax; ++k)
        {
            float a = std::pow ((float) k, -p.tilt);
            const float d = (std::log ((float) k) - lnc) / w;
            a *= 0.1f + 0.9f * std::exp (-0.5f * d * d);
            if (p.comb > 0.001f) a *= 1.0f - p.comb * (0.5f - 0.5f * std::cos (2.0f * kPi * (float) k / period));
            if (p.odd > 0.0f) { if ((k & 1) == 0) a *= 1.0f - p.odd; }
            else if (p.odd < 0.0f) { if ((k & 1) == 1 && k > 1) a *= 1.0f + p.odd; }
            const float ph = p.smear * kPi * (float) k * (float) k * (1.0f / 160.0f);
            amp[k] = a; phs[k] = ph;
        }
    }

    // chamar uma vez por bloco: avança a reconstrução em `budget` frames; devolve true se trocou de tabela
    bool update (const SpectralParams& p, int budget)
    {
        const uint32_t h = p.hash();
        if (h == builtHash && front >= 0) return false;
        if (h != buildHash) { buildHash = h; nextFrame = 0; back = front >= 0 ? 1 - front : 0; }       // novo alvo: recomeça
        if (front < 0) budget = std::max (budget, 8);                                              // primeira vez: mais depressa (4 blocos)
        const int end = std::min (Wavetables::kFrames, nextFrame + std::max (1, budget));
        for (; nextFrame < end; ++nextFrame)
        {
            frameSpectrum (p, nextFrame, ampB, phB);
            Wavetables::buildFrameFromHarmonics (ampB, phB, kHarm, buf[(size_t) back].data() + ((size_t) nextFrame * Wavetables::kMips) * Wavetables::kStride, cbuf);
        }
        if (nextFrame >= Wavetables::kFrames) { front = back; builtHash = buildHash; return true; }
        return false;
    }
    void rebuildNow (const SpectralParams& p) { builtHash = 0; buildHash = 0; front = -1; update (p, Wavetables::kFrames); while (front < 0) update (p, Wavetables::kFrames); }

private:
    std::vector<float> buf[2]; std::vector<Cpx> cbuf; float ampB[kHarm + 1] = {}, phB[kHarm + 1] = {};
    int front = -1, back = 0, nextFrame = 0; uint32_t builtHash = 0, buildHash = 0;
};
} // namespace sk
