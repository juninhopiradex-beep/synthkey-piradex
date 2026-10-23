// =============================================================================
//  SYNTHKEY by Piradex — blocos básicos de DSP (C++17, sem dependências)
//  Tudo aqui é "realtime-safe": sem alocações dentro dos métodos process.
// =============================================================================
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace sk
{
constexpr float kPi    = 3.14159265358979323846f;
constexpr float kTwoPi = 6.28318530717958647692f;

inline float clampf (float x, float a, float b) { return x < a ? a : (x > b ? b : x); }
inline float dbToGain (float db) { return std::exp (db * 0.11512925464970229f); }
inline float gainToDb (float g) { return 8.685889638065035f * std::log (std::max (g, 1.0e-9f)); }
inline float midiToHz (float note) { return 440.0f * std::exp2 ((note - 69.0f) * (1.0f / 12.0f)); }
inline float flushDenorm (float x) { return std::fabs (x) < 1.0e-20f ? 0.0f : x; }
inline float lerpf (float a, float b, float t) { return a + (b - a) * t; }

// tanh racional (erro < 2e-3 em |x|<3, satura depois)
inline float fastTanh (float x)
{
    if (x > 3.0f) return 1.0f;
    if (x < -3.0f) return -1.0f;
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

// ----------------------------------------------------------------------------
//  Gerador de números aleatórios (xorshift) — determinístico e sem locks
// ----------------------------------------------------------------------------
struct Rng
{
    uint32_t s = 0x1234567u;
    void seed (uint32_t v) { s = v ? v : 0x1234567u; }
    inline uint32_t next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    inline float uni()  { return (float) (next() >> 8) * (1.0f / 16777216.0f); }      // [0,1)
    inline float bi()   { return uni() * 2.0f - 1.0f; }                                // [-1,1)
};

// ----------------------------------------------------------------------------
//  Suavizador de um polo (para parâmetros)
// ----------------------------------------------------------------------------
struct Smoother
{
    float cur = 0.0f, tgt = 0.0f, coef = 0.0f;
    void setTime (float sr, float ms) { coef = 1.0f - std::exp (-1.0f / (0.001f * ms * sr + 1.0e-6f)); }
    void snap (float v) { cur = tgt = v; }
    inline float tick() { cur += (tgt - cur) * coef; return cur; }
};

// ----------------------------------------------------------------------------
//  Biquad (TDF-II) + fórmulas RBJ + resposta em frequência (para a GUI)
// ----------------------------------------------------------------------------
struct BiquadCoeffs
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;

    static BiquadCoeffs make (float b0, float b1, float b2, float a0, float a1, float a2)
    {
        BiquadCoeffs c; const float i = 1.0f / a0;
        c.b0 = b0 * i; c.b1 = b1 * i; c.b2 = b2 * i; c.a1 = a1 * i; c.a2 = a2 * i; return c;
    }
    static BiquadCoeffs lowpass (float f, float q, float sr)
    {
        const float w = kTwoPi * clampf (f, 10.0f, sr * 0.49f) / sr, c = std::cos (w), al = std::sin (w) / (2.0f * q);
        return make ((1 - c) * 0.5f, 1 - c, (1 - c) * 0.5f, 1 + al, -2 * c, 1 - al);
    }
    static BiquadCoeffs highpass (float f, float q, float sr)
    {
        const float w = kTwoPi * clampf (f, 10.0f, sr * 0.49f) / sr, c = std::cos (w), al = std::sin (w) / (2.0f * q);
        return make ((1 + c) * 0.5f, -(1 + c), (1 + c) * 0.5f, 1 + al, -2 * c, 1 - al);
    }
    static BiquadCoeffs bandpass (float f, float q, float sr)
    {
        const float w = kTwoPi * clampf (f, 10.0f, sr * 0.49f) / sr, c = std::cos (w), al = std::sin (w) / (2.0f * q);
        return make (al, 0, -al, 1 + al, -2 * c, 1 - al);
    }
    static BiquadCoeffs notch (float f, float q, float sr)
    {
        const float w = kTwoPi * clampf (f, 10.0f, sr * 0.49f) / sr, c = std::cos (w), al = std::sin (w) / (2.0f * q);
        return make (1, -2 * c, 1, 1 + al, -2 * c, 1 - al);
    }
    static BiquadCoeffs allpass (float f, float q, float sr)
    {
        const float w = kTwoPi * clampf (f, 10.0f, sr * 0.49f) / sr, c = std::cos (w), al = std::sin (w) / (2.0f * q);
        return make (1 - al, -2 * c, 1 + al, 1 + al, -2 * c, 1 - al);
    }
    static BiquadCoeffs peak (float f, float q, float gainDb, float sr)
    {
        const float A = std::exp (gainDb * 0.05756462732f);   // 10^(dB/40)
        const float w = kTwoPi * clampf (f, 10.0f, sr * 0.49f) / sr, c = std::cos (w), al = std::sin (w) / (2.0f * q);
        return make (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }
    static BiquadCoeffs lowShelf (float f, float gainDb, float sr)
    {
        const float A = std::exp (gainDb * 0.05756462732f);
        const float w = kTwoPi * clampf (f, 10.0f, sr * 0.49f) / sr, c = std::cos (w), s = std::sin (w);
        const float be = std::sqrt (A) * s;   // S = 1
        return make (A * ((A + 1) - (A - 1) * c + be), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - be),
                     (A + 1) + (A - 1) * c + be, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - be);
    }
    static BiquadCoeffs highShelf (float f, float gainDb, float sr)
    {
        const float A = std::exp (gainDb * 0.05756462732f);
        const float w = kTwoPi * clampf (f, 10.0f, sr * 0.49f) / sr, c = std::cos (w), s = std::sin (w);
        const float be = std::sqrt (A) * s;
        return make (A * ((A + 1) + (A - 1) * c + be), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - be),
                     (A + 1) - (A - 1) * c + be, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - be);
    }
    // |H(e^jw)| em amplitude linear
    float magnitude (float f, float sr) const
    {
        const float w = kTwoPi * f / sr, c1 = std::cos (w), s1 = std::sin (w), c2 = std::cos (2 * w), s2 = std::sin (2 * w);
        const float nr = b0 + b1 * c1 + b2 * c2, ni = -(b1 * s1 + b2 * s2);
        const float dr = 1 + a1 * c1 + a2 * c2, di = -(a1 * s1 + a2 * s2);
        return std::sqrt ((nr * nr + ni * ni) / (dr * dr + di * di + 1.0e-20f));
    }
};

struct Biquad
{
    BiquadCoeffs c; float z1 = 0, z2 = 0;
    void reset() { z1 = z2 = 0; }
    inline float tick (float x)
    {
        const float y = c.b0 * x + z1;
        z1 = c.b1 * x - c.a1 * y + z2;
        z2 = c.b2 * x - c.a2 * y;
        return y;
    }
};

// ----------------------------------------------------------------------------
//  Filtro de variável de estado (TPT/ZDF, estável sob modulação)
// ----------------------------------------------------------------------------
struct SvfCoef
{
    float g = 0, k = 1, a1 = 1, a2 = 0, a3 = 0;
    void set (float cutoffHz, float q, float sr)
    {
        g  = std::tan (kPi * clampf (cutoffHz, 10.0f, sr * 0.49f) / sr);
        k  = 1.0f / std::max (q, 0.1f);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
};

struct SvfState
{
    float ic1 = 0, ic2 = 0;
    inline void tick (float v0, const SvfCoef& c, float& lp, float& bp, float& hp)
    {
        const float v3 = v0 - ic2;
        const float v1 = c.a1 * ic1 + c.a2 * v3;
        const float v2 = ic2 + c.a2 * ic1 + c.a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        lp = v2; bp = v1; hp = v0 - c.k * v1 - v2;
    }
    void reset() { ic1 = ic2 = 0; }
};

// ----------------------------------------------------------------------------
//  Filtro de voz estéreo: 9 tipos (Desligado, LP12, LP24, HP12, HP24, BP, Notch, Ladder, Pente)
// ----------------------------------------------------------------------------
class VoiceFilter
{
public:
    enum Type { Off = 0, LP12, LP24, HP12, HP24, BP, Notch, Ladder, Comb };

    void prepare (float sampleRate)
    {
        sr = sampleRate;
        combLen = (int) (sr / 20.0f) + 8;
        for (auto& ch : comb) ch.assign ((size_t) combLen, 0.0f);
        reset();
    }
    void reset()
    {
        for (auto& a : svf) for (auto& s : a) s.reset();
        for (auto& l : ladder) for (auto& v : l) v = 0.0f;
        for (auto& ch : comb) std::fill (ch.begin(), ch.end(), 0.0f);
        combW = 0; for (auto& v : combLast) v = 0.0f;
    }

    // processa n amostras in-place. cutoff constante durante a chamada (sub-bloco curto)
    void process (int type, float* L, float* R, int n, float cutoffHz, float res, float drive)
    {
        if (type == Off || n <= 0) return;
        float* ch[2] = { L, R };
        const float gain = 1.0f + drive * 15.0f, post = 1.0f / (1.0f + 0.25f * (gain - 1.0f));
        const bool dr = drive > 0.001f;
        const float fc = clampf (cutoffHz, 20.0f, std::min (20000.0f, sr * 0.45f));

        if (type == Ladder)
        {
            const float g = std::min (0.95f, 1.0f - std::exp (-kTwoPi * fc / sr));
            const float k = res * 4.0f * 0.98f;
            for (int c = 0; c < 2; ++c)
            {
                float* s = ladder[c]; float* x = ch[c];
                for (int i = 0; i < n; ++i)
                {
                    float in = x[i]; if (dr) in = fastTanh (in * gain) * post * 1.3f;
                    in = fastTanh (in - k * s[3]);
                    s[0] += g * (in - fastTanh (s[0]));
                    s[1] += g * (fastTanh (s[0]) - fastTanh (s[1]));
                    s[2] += g * (fastTanh (s[1]) - fastTanh (s[2]));
                    s[3] += g * (fastTanh (s[2]) - fastTanh (s[3]));
                    x[i] = flushDenorm (s[3]) * (1.0f + 0.9f * res);   // compensa a perda de graves com res alta
                }
            }
            return;
        }
        if (type == Comb) { processComb (L, R, n, cutoffHz, res, drive); return; }

        // SVF: LP12 / LP24 / HP12 / HP24 / BP / Notch
        const float q1 = 0.55f + res * res * 18.0f;
        SvfCoef c1, c2; c1.set (fc, q1, sr); c2.set (fc, 0.7071f, sr);
        const bool two = (type == LP24 || type == HP24);
        for (int c = 0; c < 2; ++c)
        {
            float* x = ch[c];
            for (int i = 0; i < n; ++i)
            {
                float in = x[i]; if (dr) in = fastTanh (in * gain) * post * 1.3f;
                float lp, bp, hp;
                svf[c][0].tick (in, c1, lp, bp, hp);
                float y;
                switch (type)
                {
                    case LP12: y = lp; break;
                    case HP12: y = hp; break;
                    case BP:   y = bp * (1.0f / q1) * 2.0f; break;
                    case Notch: y = lp + hp; break;
                    case LP24: { float l2, b2, h2; svf[c][1].tick (lp, c2, l2, b2, h2); y = l2; } break;
                    default:   { float l2, b2, h2; svf[c][1].tick (hp, c2, l2, b2, h2); y = h2; } break;
                }
                x[i] = y;
            }
        }
    }

    // O Pente precisa de um índice de escrita partilhado — implementação separada e simples:
    void processComb (float* L, float* R, int n, float cutoffHz, float res, float drive)
    {
        const float fc = clampf (cutoffHz, 20.0f, std::min (20000.0f, sr * 0.45f));
        const float d = clampf (sr / fc, 2.0f, (float) (combLen - 4));
        const float fb = res * 0.95f, outG = 1.0f - 0.5f * fb;
        const float gain = 1.0f + drive * 15.0f, post = 1.0f / (1.0f + 0.25f * (gain - 1.0f));
        const bool dr = drive > 0.001f;
        float* ch[2] = { L, R };
        for (int i = 0; i < n; ++i)
        {
            float rp = (float) combW - d; if (rp < 0) rp += (float) combLen;
            const int i0 = (int) rp, i1 = (i0 + 1) % combLen; const float fr = rp - (float) i0;
            for (int c = 0; c < 2; ++c)
            {
                auto& b = comb[c];
                const float dl = b[(size_t) i0] + (b[(size_t) i1] - b[(size_t) i0]) * fr;
                float in = ch[c][i]; if (dr) in = fastTanh (in * gain) * post * 1.3f;
                const float y = flushDenorm (in + fb * dl);
                b[(size_t) combW] = clampf (y, -8.0f, 8.0f);
                ch[c][i] = y * outG;
            }
            if (++combW >= combLen) combW = 0;
        }
    }

private:
    float sr = 44100.0f;
    SvfState svf[2][2];
    float ladder[2][4] = {};
    std::vector<float> comb[2];
    int combLen = 2200, combW = 0;
    float combLast[2] = {};
};

// ----------------------------------------------------------------------------
//  Envelope ADSR analógico (exponencial). Tempos em segundos.
//  Ataque: aproxima-se de 1.3 (cobre 1.0 no tempo A); decay/release: -60 dB no tempo D/R.
// ----------------------------------------------------------------------------
class Adsr
{
public:
    enum Stage { Idle = 0, Attack, Decay, Sustain, Release };

    void setSampleRate (float s) { sr = s; }
    void set (float a, float d, float s, float r)
    {
        sus = clampf (s, 0.0f, 1.0f);
        aC = std::pow (0.3f / 1.3f, 1.0f / std::max (a * sr, 1.0f));
        dC = std::pow (0.001f, 1.0f / std::max (d * sr, 1.0f));
        rC = std::pow (0.001f, 1.0f / std::max (r * sr, 1.0f));
    }
    void gateOn()  { stage = Attack; }
    void gateOff() { if (stage != Idle) stage = Release; }
    void kill()    { stage = Idle; level = 0.0f; }
    void fastRelease (float seconds) { rC = std::pow (0.001f, 1.0f / std::max (seconds * sr, 1.0f)); stage = Release; }
    bool active() const { return stage != Idle; }
    Stage getStage() const { return stage; }
    float getLevel() const { return level; }

    inline float tick()
    {
        switch (stage)
        {
            case Attack:
                level = 1.3f - (1.3f - level) * aC;
                if (level >= 1.0f) { level = 1.0f; stage = Decay; }
                break;
            case Decay:
                level = sus + (level - sus) * dC;
                if (level - sus < 1.0e-5f) { level = sus; stage = Sustain; }
                break;
            case Sustain: level = sus; break;
            case Release:
                level *= rC;
                if (level < 1.0e-5f) { level = 0.0f; stage = Idle; }
                break;
            default: level = 0.0f; break;
        }
        return level;
    }
private:
    float sr = 44100.0f, aC = 0.99f, dC = 0.99f, rC = 0.99f, sus = 1.0f, level = 0.0f;
    Stage stage = Idle;
};

// ----------------------------------------------------------------------------
//  LFO (7 formas). Fase em [0,1). Valor bipolar [-1,1].
// ----------------------------------------------------------------------------
struct Lfo
{
    float phase = 0.0f, held = 0.0f, prev = 0.0f, next = 0.0f;
    Rng rng;

    void reset (float startPhase, uint32_t seed)
    {
        phase = startPhase - std::floor (startPhase); rng.seed (seed);
        held = rng.bi(); prev = rng.bi(); next = rng.bi();
    }
    // avança 'dt' em fração de ciclo e devolve o valor
    float advance (int wave, float cyclesPerSample, int samples)
    {
        phase += cyclesPerSample * (float) samples;
        while (phase >= 1.0f)
        {
            phase -= 1.0f; held = rng.bi(); prev = next; next = rng.bi();
        }
        return value (wave);
    }
    float value (int wave) const
    {
        switch (wave)
        {
            case 0: return std::sin (kTwoPi * phase);
            case 1: return 4.0f * std::fabs (phase - 0.5f) - 1.0f;
            case 2: return 2.0f * phase - 1.0f;
            case 3: return 1.0f - 2.0f * phase;
            case 4: return phase < 0.5f ? 1.0f : -1.0f;
            case 5: return held;
            default: { const float t = phase * phase * (3.0f - 2.0f * phase); return prev + (next - prev) * t; }
        }
    }
};

// ----------------------------------------------------------------------------
//  polyBLEP
// ----------------------------------------------------------------------------
inline float polyBlep (float t, float dt)
{
    if (t < dt) { t /= dt; return t + t - t * t - 1.0f; }
    if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
    return 0.0f;
}

// ----------------------------------------------------------------------------
//  Ruído branco / rosa / castanho misturável (cor 0..1)
// ----------------------------------------------------------------------------
struct NoiseGen
{
    Rng rng; float b0 = 0, b1 = 0, b2 = 0, brown = 0;
    void seed (uint32_t s) { rng.seed (s); }
    inline float tick (float color)
    {
        const float w = rng.bi();
        b0 = 0.99765f * b0 + w * 0.0990460f; b1 = 0.96300f * b1 + w * 0.2965164f; b2 = 0.57000f * b2 + w * 1.0526913f;
        const float pink = (b0 + b1 + b2 + w * 0.1848f) * 0.25f;
        brown = (brown + 0.02f * w) / 1.02f; const float br = brown * 3.5f;
        if (color < 0.5f) return lerpf (w, pink, color * 2.0f);
        return lerpf (pink, br, (color - 0.5f) * 2.0f);
    }
};

// ----------------------------------------------------------------------------
//  Interpolação Hermite (4 pontos) sobre um buffer
// ----------------------------------------------------------------------------
inline float hermite (float xm1, float x0, float x1, float x2, float t)
{
    const float c0 = x0, c1 = 0.5f * (x1 - xm1);
    const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
    const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
    return ((c3 * t + c2) * t + c1) * t + c0;
}

// ----------------------------------------------------------------------------
//  FFT radix-2 (usada só na geração das wavetables, fora da thread de áudio)
// ----------------------------------------------------------------------------
struct Cpx { float re, im; };

inline void fft (std::vector<Cpx>& a, bool inverse)
{
    const size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i)
    {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap (a[i], a[j]);
    }
    for (size_t len = 2; len <= n; len <<= 1)
    {
        const double ang = 2.0 * 3.14159265358979323846 / (double) len * (inverse ? 1.0 : -1.0);
        const double wr = std::cos (ang), wi = std::sin (ang);
        for (size_t i = 0; i < n; i += len)
        {
            double cr = 1.0, ci = 0.0;
            for (size_t k = 0; k < len / 2; ++k)
            {
                const Cpx u = a[i + k], v = a[i + k + len / 2];
                const double vr = v.re * cr - v.im * ci, vi = v.re * ci + v.im * cr;
                a[i + k]           = { (float) (u.re + vr), (float) (u.im + vi) };
                a[i + k + len / 2] = { (float) (u.re - vr), (float) (u.im - vi) };
                const double ncr = cr * wr - ci * wi; ci = cr * wi + ci * wr; cr = ncr;
            }
        }
    }
    if (inverse) for (auto& c : a) { c.re /= (float) n; c.im /= (float) n; }
}

} // namespace sk
