// =============================================================================
//  SYNTHKEY by Piradex — reverb de convolução (efeito 19)
//
//  • 6 respostas ao impulso (IR) geradas por procedimento (determinísticas, sem ficheiros): Quarto, Sala, Hall,
//    Placa, Catedral, Mola. Reflexões iniciais + cauda difusa com decaimento exponencial e abafamento progressivo
//    dos agudos; canal esquerdo e direito descorrelacionados.
//  • Convolução particionada uniforme no domínio da frequência (overlap-save, FFT de 2B): custo constante por
//    amostra, independente do tamanho do bloco do anfitrião. L e R vão juntos numa só FFT complexa.
//  • Latência intrínseca = B amostras (≈10,7 ms): é um pré-atraso do sinal molhado, o seco não é atrasado.
//  • As IR espectrais ficam num banco partilhado por taxa de amostragem (construído em prepare(), nunca no
//    fio de áudio). Não carrega IR de utilizador (decisão de âmbito).
// =============================================================================
#pragma once
#include "Dsp.h"
#include <map>
#include <memory>
#include <mutex>

namespace sk
{
inline constexpr int kConvNumIr = 6;
inline const char* const kConvIrNames[kConvNumIr] = { "Quarto", "Sala", "Hall", "Placa", "Catedral", "Mola" };

namespace convdetail
{
struct IrSpec { float len, rt60, fc0, fcMin, tauHf, onsetMs; int erCount; float erSpanMs, erAmp; bool spring; };
inline const IrSpec kSpecs[kConvNumIr] = {
    { 0.60f, 0.35f,  9000.0f, 1500.0f, 0.25f,  3.0f, 12, 22.0f, 0.9f, false },   // Quarto
    { 1.20f, 0.90f,  8000.0f, 1200.0f, 0.70f,  8.0f, 14, 40.0f, 0.7f, false },   // Sala
    { 2.60f, 2.20f,  7000.0f,  900.0f, 1.60f, 18.0f, 16, 60.0f, 0.5f, false },   // Hall
    { 2.00f, 1.70f, 16000.0f, 4000.0f, 3.00f,  1.5f,  0,  0.0f, 0.0f, false },   // Placa
    { 3.20f, 3.00f,  5500.0f,  600.0f, 2.60f, 35.0f, 10, 90.0f, 0.4f, false },   // Catedral (RT60 3 s, IR de 3,2 s)
    { 1.80f, 1.30f,  4200.0f, 1200.0f, 1.20f,  2.0f,  0,  0.0f, 0.0f, true  },   // Mola
};
struct Lcg { uint32_t s; float next() { s = s * 1664525u + 1013904223u; return (float) (s >> 8) * (2.0f / 16777216.0f) - 1.0f; } };
}

// resposta ao impulso no tempo (energia total normalizada), canal 0 = esquerdo, 1 = direito
inline std::vector<float> convIr (int idx, double sr, int ch)
{
    using namespace convdetail;
    const IrSpec& S = kSpecs[std::max (0, std::min (kConvNumIr - 1, idx))];
    const int n = (int) (S.len * sr);
    std::vector<float> h ((size_t) n, 0.0f);
    Lcg rng { 0x9E3779B9u * (uint32_t) (idx * 2 + ch + 1) };
    float lp = 0.0f, lp2 = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        const float t = (float) i / (float) sr;
        const float fc = std::max (S.fcMin, S.fc0 * std::exp (-t / S.tauHf));
        const float a = 1.0f - std::exp (-6.2831853f * fc / (float) sr);
        lp += a * (rng.next() - lp); lp2 += a * (lp - lp2);                      // 2 pólos: abafa progressivamente os agudos
        const float env = std::exp (-6.907755f * t / S.rt60);
        const float onset = 1.0f - std::exp (-t / (0.001f * S.onsetMs * 0.5f));
        h[(size_t) i] = lp2 * env * onset;
    }
    // o filtro de 2 pólos tira ganho: normaliza só a cauda; as reflexões iniciais somam-se depois
    for (int e = 0; e < S.erCount; ++e)
    {
        const float tms = 0.8f + (0.5f + 0.5f * rng.next()) * S.erSpanMs; const int pos = (int) (tms * 0.001 * sr);
        if (pos < n) h[(size_t) pos] += S.erAmp * 0.35f * rng.next() * std::exp (-tms / (S.erSpanMs * 1.3f));
    }
    if (S.spring)   // "boing" de mola: pulsos dispersivos (chirps descendentes) repetidos a cada ~35 ms
    {
        const float period = (0.034f + 0.003f * (float) ch) * (float) sr;
        for (int k = 0; k < 40; ++k)
        {
            const int p0 = (int) ((float) (k + 1) * period); if (p0 >= n) break;
            const float amp = 0.5f * std::pow (0.80f, (float) k); float ph = 0.0f;
            for (int j = 0; j < (int) (0.008 * sr) && p0 + j < n; ++j)
            {
                const float u = (float) j / (float) (0.008 * sr); const float f = 3200.0f * (1.0f - 0.85f * u);
                ph += 6.2831853f * f / (float) sr; h[(size_t) (p0 + j)] += amp * std::sin (ph) * (1.0f - u);
            }
        }
    }
    double e2 = 0; for (float v : h) e2 += (double) v * v;
    const float g = e2 > 0 ? (float) std::sqrt (0.35 / e2) : 1.0f;
    for (auto& v : h) v *= g;
    return h;
}

inline int convBlockFor (double sr) { return sr <= 52000.0 ? 512 : (sr <= 104000.0 ? 1024 : 2048); }

// banco partilhado de espectros de IR por taxa de amostragem
struct ConvBank
{
    int B = 512, N = 1024, bins = 513;
    struct Ir { int parts = 0; std::vector<float> re, im; };    // layout [parte][canal][bin]
    Ir ir[kConvNumIr];
    int maxParts = 0;

    static std::shared_ptr<const ConvBank> get (double sr)
    {
        static std::mutex m; static std::map<int, std::shared_ptr<const ConvBank>> cache;
        std::lock_guard<std::mutex> lk (m);
        auto& e = cache[(int) std::lround (sr)];
        if (! e) { auto b = std::make_shared<ConvBank>(); b->build (sr); e = b; }
        return e;
    }
    void build (double sr)
    {
        B = convBlockFor (sr); N = 2 * B; bins = B + 1;
        std::vector<Cpx> buf ((size_t) N);
        for (int idx = 0; idx < kConvNumIr; ++idx)
        {
            std::vector<float> h[2] = { convIr (idx, sr, 0), convIr (idx, sr, 1) };
            Ir& I = ir[idx]; I.parts = ((int) h[0].size() + B - 1) / B; maxParts = std::max (maxParts, I.parts);
            I.re.assign ((size_t) I.parts * 2 * bins, 0.0f); I.im.assign ((size_t) I.parts * 2 * bins, 0.0f);
            for (int k = 0; k < I.parts; ++k)
                for (int c = 0; c < 2; ++c)
                {
                    for (int i = 0; i < N; ++i)
                    {
                        const size_t s = (size_t) k * B + (size_t) i;
                        buf[(size_t) i] = { (i < B && s < h[c].size()) ? h[c][s] : 0.0f, 0.0f };
                    }
                    fft (buf, false);
                    const size_t o = ((size_t) k * 2 + (size_t) c) * bins;
                    for (int b = 0; b < bins; ++b) { I.re[o + (size_t) b] = buf[(size_t) b].re; I.im[o + (size_t) b] = buf[(size_t) b].im; }
                }
        }
    }
};

// estado de convolução por instância de efeito
class Convolver
{
public:
    void prepare (double sr)
    {
        bank = ConvBank::get (sr); B = bank->B; N = bank->N; bins = bank->bins;
        const size_t fdlSize = (size_t) bank->maxParts * 2 * (size_t) bins;
        if (fdRe.size() != fdlSize) { fdRe.assign (fdlSize, 0.0f); fdIm.assign (fdlSize, 0.0f); }
        work.assign ((size_t) N, Cpx { 0.0f, 0.0f }); accRe.assign ((size_t) bins * 2, 0.0f); accIm.assign ((size_t) bins * 2, 0.0f);
        reset();
    }
    void reset()
    {
        std::fill (fdRe.begin(), fdRe.end(), 0.0f); std::fill (fdIm.begin(), fdIm.end(), 0.0f);
        inBuf[0].assign ((size_t) 2 * B, 0.0f); inBuf[1].assign ((size_t) 2 * B, 0.0f); outBuf[0].assign ((size_t) B, 0.0f); outBuf[1].assign ((size_t) B, 0.0f);
        pos = 0; head = 0; curIr = -1;
    }
    int latency() const { return B; }
    // `lenFrac` 0..1: fração da IR usada (as últimas partições descem em rampa para não cortar a cauda)
    void process (int irIdx, float lenFrac, float* L, float* R, int n)
    {
        if (! bank) return;
        irIdx = std::max (0, std::min (kConvNumIr - 1, irIdx));
        if (irIdx != curIr) { std::fill (fdRe.begin(), fdRe.end(), 0.0f); std::fill (fdIm.begin(), fdIm.end(), 0.0f); curIr = irIdx; }
        const ConvBank::Ir& I = bank->ir[irIdx];
        kUse = std::max (2, std::min (I.parts, (int) std::lround ((float) I.parts * std::max (0.05f, std::min (1.0f, lenFrac)))));
        for (int i = 0; i < n; ++i)
        {
            const float xl = L[i], xr = R[i];
            L[i] = outBuf[0][(size_t) pos]; R[i] = outBuf[1][(size_t) pos];
            inBuf[0][(size_t) (B + pos)] = xl; inBuf[1][(size_t) (B + pos)] = xr;
            if (++pos >= B) { block (I); pos = 0; }
        }
    }

private:
    std::shared_ptr<const ConvBank> bank; int B = 512, N = 1024, bins = 513, pos = 0, head = 0, curIr = -1, kUse = 2;
    std::vector<float> fdRe, fdIm, accRe, accIm, inBuf[2], outBuf[2]; std::vector<Cpx> work;

    void block (const ConvBank::Ir& I)
    {
        // 1) FFT conjunta [anterior | atual] de L (real) e R (imaginário)
        for (int i = 0; i < N; ++i) work[(size_t) i] = { inBuf[0][(size_t) i], inBuf[1][(size_t) i] };
        fft (work, false);
        const int P = bank->maxParts;
        head = head == 0 ? P - 1 : head - 1;                        // a parte mais recente fica em `head`
        float* xr0 = &fdRe[((size_t) head * 2 + 0) * bins]; float* xi0 = &fdIm[((size_t) head * 2 + 0) * bins];
        float* xr1 = &fdRe[((size_t) head * 2 + 1) * bins]; float* xi1 = &fdIm[((size_t) head * 2 + 1) * bins];
        for (int b = 0; b < bins; ++b)
        {
            const Cpx a = work[(size_t) b], m = work[(size_t) ((N - b) % N)];           // X[b] e X[N-b]
            xr0[b] = 0.5f * (a.re + m.re); xi0[b] = 0.5f * (a.im - m.im);               // L = (X + conj X')/2
            xr1[b] = 0.5f * (a.im + m.im); xi1[b] = -0.5f * (a.re - m.re);              // R = (X - conj X')/(2i)
        }
        // 2) acumula Σ X[n-k]·H[k] (dois canais, 513 bins)
        std::fill (accRe.begin(), accRe.end(), 0.0f); std::fill (accIm.begin(), accIm.end(), 0.0f);
        const int kFull = std::max (1, kUse - 8);
        for (int k = 0; k < kUse; ++k)
        {
            const int pi = (head + k) % P;
            const float w = k < kFull ? 1.0f : 1.0f - (float) (k - kFull + 1) / (float) (kUse - kFull + 1);
            for (int c = 0; c < 2; ++c)
            {
                const float* xr = &fdRe[((size_t) pi * 2 + (size_t) c) * bins]; const float* xi = &fdIm[((size_t) pi * 2 + (size_t) c) * bins];
                const float* hr = &I.re[((size_t) k * 2 + (size_t) c) * bins]; const float* hi = &I.im[((size_t) k * 2 + (size_t) c) * bins];
                float* yr = &accRe[(size_t) c * bins]; float* yi = &accIm[(size_t) c * bins];
                if (w >= 1.0f) for (int b = 0; b < bins; ++b) { yr[b] += xr[b] * hr[b] - xi[b] * hi[b]; yi[b] += xr[b] * hi[b] + xi[b] * hr[b]; }
                else           for (int b = 0; b < bins; ++b) { yr[b] += w * (xr[b] * hr[b] - xi[b] * hi[b]); yi[b] += w * (xr[b] * hi[b] + xi[b] * hr[b]); }
            }
        }
        // 3) Y = YL + i·YR (ambos hermitianos) → uma IFFT dá yL + i·yR
        for (int b = 0; b < bins; ++b)
        {
            const float lr = accRe[(size_t) b], li = accIm[(size_t) b], rr = accRe[(size_t) bins + (size_t) b], ri = accIm[(size_t) bins + (size_t) b];
            work[(size_t) b] = { lr - ri, li + rr };
            if (b > 0 && b < N / 2) work[(size_t) (N - b)] = { lr + ri, -li + rr };
        }
        fft (work, true);
        for (int i = 0; i < B; ++i) { outBuf[0][(size_t) i] = work[(size_t) (B + i)].re; outBuf[1][(size_t) i] = work[(size_t) (B + i)].im; }
        for (int c = 0; c < 2; ++c) { std::memcpy (inBuf[c].data(), inBuf[c].data() + B, (size_t) B * sizeof (float)); }
    }
};
} // namespace sk
