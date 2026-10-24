// =============================================================================
//  SYNTHKEY by Piradex — áudio → MIDI (WAV/loop → groove editável)
//
//  Funciona sobre áudio mono em memória (sem JUCE):
//    · andamento (BPM) por autocorrelação do envelope de ataques + fase da grelha;
//    · bateria: ataques por banda (grave = bombo, médios/agudos = caixa/palmas, muito agudos = hi-hat fechado/aberto);
//    · melodia / baixo: altura por YIN (autocorrelação via FFT), notas segmentadas e quantizadas;
//    · acordes: croma por tempo e comparação com modelos (maior, menor, sétima…).
//  O resultado é um Groove (semicolcheias, até 16 compassos) que se edita no piano-roll.
//  Não é mágica: áudio com muitos instrumentos misturados dá resultados aproximados — por isso há sempre
//  edição depois e a escolha manual do tipo (bateria / baixo / melodia / acordes) e do BPM.
// =============================================================================
#pragma once
#include "Grooves.h"
#include <algorithm>
#include <cmath>
#include <complex>
#include <numeric>
#include <string>
#include <vector>

namespace sk { namespace a2m
{
constexpr double kPi = 3.14159265358979323846;

// ---------------------------------------------------------------- FFT (radix-2)
struct Fft
{
    int n = 0; std::vector<std::complex<float>> tw; std::vector<int> rev;
    explicit Fft (int size) : n (size)
    {
        tw.resize ((size_t) n / 2); for (int i = 0; i < n / 2; ++i) tw[(size_t) i] = std::polar (1.0f, (float) (-2.0 * kPi * i / n));
        rev.resize ((size_t) n); int lg = 0; while ((1 << lg) < n) ++lg;
        for (int i = 0; i < n; ++i) { int r = 0; for (int b = 0; b < lg; ++b) if (i & (1 << b)) r |= 1 << (lg - 1 - b); rev[(size_t) i] = r; }
    }
    void run (std::vector<std::complex<float>>& a, bool inverse = false) const
    {
        for (int i = 0; i < n; ++i) if (i < rev[(size_t) i]) std::swap (a[(size_t) i], a[(size_t) rev[(size_t) i]]);
        for (int len = 2; len <= n; len <<= 1)
        {
            const int step = n / len;
            for (int i = 0; i < n; i += len)
                for (int j = 0; j < len / 2; ++j)
                {
                    auto w = tw[(size_t) (j * step)]; if (inverse) w = std::conj (w);
                    const auto u = a[(size_t) (i + j)], v = a[(size_t) (i + j + len / 2)] * w; a[(size_t) (i + j)] = u + v; a[(size_t) (i + j + len / 2)] = u - v;
                }
        }
        if (inverse) for (auto& x : a) x /= (float) n;
    }
};
inline int pow2AtLeast (double v) { int p = 256; while (p < v) p <<= 1; return p; }

// ---------------------------------------------------------------- bandas e ataques
struct Bands
{
    double hop = 0, sr = 0; int frames = 0;
    std::vector<float> low, body, mid, high, total;     // energia (amplitude comprimida) por quadro
};
inline Bands analyseBands (const std::vector<float>& x, double sr)
{
    Bands b; b.sr = sr; const int N = pow2AtLeast (0.0232 * sr), H = N / 4; b.hop = (double) H / sr;
    Fft fft (N); std::vector<float> win ((size_t) N); for (int i = 0; i < N; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos (2.0 * kPi * i / N);
    const int n = (int) x.size(); b.frames = n > N ? (n - N) / H + 1 : 0;
    for (auto* v : { &b.low, &b.body, &b.mid, &b.high, &b.total }) v->assign ((size_t) b.frames, 0.0f);
    std::vector<std::complex<float>> buf ((size_t) N); const double binHz = sr / N;
    auto bin = [&] (double hz) { return std::max (1, std::min (N / 2 - 1, (int) std::lround (hz / binHz))); };
    const int l0 = bin (30), l1 = bin (150), b0 = bin (150), b1 = bin (420), m0 = bin (1500), m1 = bin (6000), h0 = bin (7000), h1 = bin (std::min (16000.0, sr * 0.45));
    for (int f = 0; f < b.frames; ++f)
    {
        for (int i = 0; i < N; ++i) buf[(size_t) i] = x[(size_t) (f * H + i)] * win[(size_t) i];
        fft.run (buf);
        auto sum = [&] (int a, int c) { double s = 0; for (int k = a; k <= c; ++k) s += std::abs (buf[(size_t) k]); return (float) (s / std::max (1, c - a + 1)); };
        b.low[(size_t) f] = sum (l0, l1); b.body[(size_t) f] = sum (b0, b1); b.mid[(size_t) f] = sum (m0, m1); b.high[(size_t) f] = sum (h0, h1); b.total[(size_t) f] = sum (1, N / 2 - 1);
    }
    return b;
}
// fluxo positivo (aumento de energia comprimida) de uma banda
inline std::vector<float> flux (const std::vector<float>& e)
{
    std::vector<float> f (e.size(), 0.0f); std::vector<float> c (e.size()); for (size_t i = 0; i < e.size(); ++i) c[i] = std::pow (e[i], 0.5f);
    for (size_t i = 2; i < e.size(); ++i) f[i] = std::max (0.0f, c[i] - std::max (c[i - 1], c[i - 2]) * 0.98f);
    return f;
}
struct Peak { int frame; float strength; };
inline std::vector<Peak> pickPeaks (const std::vector<float>& f, double hop, float relMin, double minGapSec)
{
    std::vector<Peak> out; if (f.size() < 8) return out;
    float mx = 0; double mean = 0; for (float v : f) { mx = std::max (mx, v); mean += v; } mean /= (double) f.size();
    double var = 0; for (float v : f) var += (v - mean) * (v - mean); const double sd = std::sqrt (var / (double) f.size());
    const float thr = std::max (relMin * mx, (float) (mean + 1.0 * sd)); const int gap = std::max (1, (int) std::lround (minGapSec / hop));
    for (int i = 3; i + 3 < (int) f.size(); ++i)
    {
        if (f[(size_t) i] < thr) continue; bool isMax = true; for (int k = -3; k <= 3; ++k) if (f[(size_t) (i + k)] > f[(size_t) i]) { isMax = false; break; }
        if (! isMax) continue;
        if (! out.empty() && i - out.back().frame < gap) { if (f[(size_t) i] > out.back().strength) out.back() = { i, f[(size_t) i] }; continue; }
        out.push_back ({ i, f[(size_t) i] });
    }
    return out;
}

// ---------------------------------------------------------------- andamento
struct Tempo { double bpm = 0, conf = 0, phaseSec = 0; };
// 'env' = envelope de ataques (qualquer curva ≥ 0) com passo 'hop' segundos
inline Tempo estimateTempo (const std::vector<float>& env, double hop, double minBpm = 70, double maxBpm = 180)
{
    Tempo t; const int n = (int) env.size(); if (n < 64) return t;
    std::vector<double> e (env.begin(), env.end()); double mean = 0; for (double v : e) mean += v; mean /= n; for (double& v : e) v -= mean;
    const int lagMin = std::max (2, (int) std::floor (60.0 / maxBpm / hop)), lagMax = std::min (n / 2, (int) std::ceil (60.0 / minBpm / hop * 2.05));
    if (lagMax <= lagMin + 2) return t;
    std::vector<double> ac ((size_t) lagMax + 2, 0.0); double e0 = 1e-12; for (double v : e) e0 += v * v;
    for (int lag = lagMin - 1; lag <= lagMax + 1 && lag < n; ++lag) { double s = 0; for (int i = 0; i + lag < n; ++i) s += e[(size_t) i] * e[(size_t) (i + lag)]; ac[(size_t) lag] = s / e0 * ((double) n / (double) (n - lag)); }
    // pontuação por andamento: autocorrelação nos lags de 1, 2 e 4 tempos reforça o andamento certo; prior suave em torno de 105 bpm
    double bestScore = -1e30, bestBpm = 0; double secondScore = -1e30;
    for (double bpm = minBpm; bpm <= maxBpm; bpm += 0.25)
    {
        const double lagF = 60.0 / bpm / hop; double s = 0; int cnt = 0;
        for (double mult : { 1.0, 2.0, 4.0 })
        {
            const double lf = lagF * mult; const int l0 = (int) lf; if (l0 + 1 >= (int) ac.size() || l0 < 1) continue;
            const double v = ac[(size_t) l0] * (1.0 - (lf - l0)) + ac[(size_t) l0 + 1] * (lf - l0); s += v / mult * (mult == 1.0 ? 1.0 : 1.4); ++cnt;
        }
        if (! cnt) continue;
        const double prior = std::exp (-0.5 * std::pow (std::log2 (bpm / 105.0) / 0.75, 2.0)); s *= (0.55 + 0.45 * prior);
        if (s > bestScore) { secondScore = bestScore; bestScore = s; bestBpm = bpm; }
    }
    if (bestBpm <= 0) return t;
    t.bpm = bestBpm; t.conf = std::max (0.0, std::min (1.0, bestScore * 2.0));
    (void) secondScore; return t;
}
// fase da grelha: desvio (s) do início da música à 1.ª semicolcheia, que maximiza o alinhamento dos ataques
inline double gridPhase (const std::vector<Peak>& peaks, double hop, double bpm)
{
    const double g = 60.0 / bpm / 4.0; double best = -1, bestPh = 0;
    for (double ph = 0; ph < g; ph += g / 48.0)
    {
        double s = 0; for (auto& p : peaks) { const double t = p.frame * hop; const double d = std::fmod (t - ph + g * 100.0, g); const double e = std::min (d, g - d); s += p.strength * std::exp (-0.5 * std::pow (e / (g * 0.12), 2.0)); }
        if (s > best) { best = s; bestPh = ph; }
    }
    return bestPh;
}
// ---------------------------------------------------------------- altura (YIN com autocorrelação por FFT)
struct Pitch { float f0 = 0, conf = 0, rms = 0; };
inline std::vector<Pitch> trackPitch (const std::vector<float>& x, double sr, double& hopSec, double fmin = 38.0, double fmax = 1500.0)
{
    const int W = pow2AtLeast (0.075 * sr), H = std::max (64, W / 8); hopSec = (double) H / sr; const int F = W * 2; Fft fft (F);
    const int tauMin = std::max (2, (int) (sr / fmax)), tauMax = std::min (W - 2, (int) (sr / fmin));
    const int n = (int) x.size(); const int frames = n > W ? (n - W) / H + 1 : 0; std::vector<Pitch> out ((size_t) frames);
    std::vector<std::complex<float>> a ((size_t) F); std::vector<double> d ((size_t) tauMax + 2), cm ((size_t) tauMax + 2), cs ((size_t) W + 1);
    const int win = W / 2;                                  // janela de integração
    for (int f = 0; f < frames; ++f)
    {
        const float* p = x.data() + (size_t) f * H; double e = 0; for (int i = 0; i < W; ++i) e += (double) p[i] * p[i]; out[(size_t) f].rms = (float) std::sqrt (e / W);
        if (out[(size_t) f].rms < 1.0e-4f) continue;
        for (int i = 0; i < F; ++i) a[(size_t) i] = i < W ? std::complex<float> (p[i], 0) : std::complex<float> (0, 0);
        fft.run (a); for (auto& v : a) v = std::complex<float> (std::norm (v), 0); fft.run (a, true);
        cs[0] = 0; for (int i = 0; i < W; ++i) cs[(size_t) i + 1] = cs[(size_t) i] + (double) p[i] * p[i];
        // d(τ) = Σ_{j<win} (x_j − x_{j+τ})² = e(0..win) + e(τ..τ+win) − 2 r(τ)   (r aproximada pela autocorrelação do quadro inteiro, normalizada)
        d[0] = 0; double run = 0;
        for (int tau = 1; tau <= tauMax; ++tau)
        {
            const double r = a[(size_t) tau].real() * ((double) win / (double) (W - tau));
            const double e1 = cs[(size_t) win] - cs[0], e2 = cs[(size_t) std::min (W, tau + win)] - cs[(size_t) tau];
            d[(size_t) tau] = std::max (0.0, e1 + e2 - 2.0 * r); run += d[(size_t) tau]; cm[(size_t) tau] = run > 0 ? d[(size_t) tau] * tau / run : 1.0;
        }
        int best = -1; for (int tau = tauMin; tau < tauMax; ++tau) if (cm[(size_t) tau] < 0.16) { while (tau + 1 < tauMax && cm[(size_t) tau + 1] < cm[(size_t) tau]) ++tau; best = tau; break; }
        if (best < 0) { double m = 1e9; for (int tau = tauMin; tau < tauMax; ++tau) if (cm[(size_t) tau] < m) { m = cm[(size_t) tau]; best = tau; } if (m > 0.35) continue; }
        double off = 0; if (best > 1 && best < tauMax) { const double s0 = cm[(size_t) best - 1], s1 = cm[(size_t) best], s2 = cm[(size_t) best + 1], den = s0 - 2 * s1 + s2; if (std::fabs (den) > 1e-12) off = 0.5 * (s0 - s2) / den; }
        out[(size_t) f].f0 = (float) (sr / (best + off)); out[(size_t) f].conf = (float) std::max (0.0, 1.0 - cm[(size_t) best]);
    }
    return out;
}

// ---------------------------------------------------------------- resultado
struct Options { double bpm = 0; std::string style = "trap"; std::string name = "Audio"; std::string kind = "auto"; double maxSeconds = 180.0; };
struct Result { bool ok = false; std::string error, detectedKind; Groove groove; double bpm = 0, confidence = 0; int noteCount = 0; };

inline int stepsOf (double t, double start, double grid) { return (int) std::lround ((t - start) / grid); }

// ---------------------------------------------------------------- bateria
inline void drumsToNotes (const std::vector<float>& x, double sr, double& bpm, double& conf, std::vector<GNote>& notes, double& startSec)
{
    const auto B = analyseBands (x, sr); if (B.frames < 32) return;
    const auto fl = flux (B.low), fb = flux (B.body), fm = flux (B.mid), fh = flux (B.high);
    std::vector<float> all (fl.size()); for (size_t i = 0; i < all.size(); ++i) all[i] = fl[i] + fb[i] + fm[i] + fh[i];
    const auto pk = pickPeaks (all, B.hop, 0.08f, 0.045);
    if (bpm <= 0) { const auto t = estimateTempo (all, B.hop); bpm = t.bpm; conf = t.conf; } else conf = 1.0;
    if (bpm <= 0) return;
    const double grid = 60.0 / bpm / 4.0;
    // eventos por instrumento
    struct Ev { double t; int note; float strength; };
    std::vector<Ev> evs;
    const auto pkL = pickPeaks (fl, B.hop, 0.30f, 0.06), pkB = pickPeaks (fb, B.hop, 0.30f, 0.06), pkM = pickPeaks (fm, B.hop, 0.40f, 0.06), pkH = pickPeaks (fh, B.hop, 0.12f, 0.035);
    float mxL = 1e-9f, mxM = 1e-9f, mxH = 1e-9f; for (auto& p : pkL) mxL = std::max (mxL, p.strength); for (auto& p : pkM) mxM = std::max (mxM, p.strength); for (auto& p : pkH) mxH = std::max (mxH, p.strength);
    auto near = [&] (const std::vector<Peak>& v, int fr, int tol) { for (auto& p : v) if (std::abs (p.frame - fr) <= tol) return &p; return (const Peak*) nullptr; };
    std::vector<char> usedH (pkH.size(), 0);
    auto flNear = [&] (int fr) { float m = 0; for (int k = -2; k <= 2; ++k) if (fr + k >= 0 && fr + k < (int) fl.size()) m = std::max (m, fl[(size_t) (fr + k)]); return m; };
    auto fmNear = [&] (int fr) { float m = 0; for (int k = -2; k <= 2; ++k) if (fr + k >= 0 && fr + k < (int) fm.size()) m = std::max (m, fm[(size_t) (fr + k)]); return m; };
    for (auto& p : pkL)                                                                                                          // bombo (a caixa também "vaza" para os graves: se os médios dominam claramente, é só caixa)
    {
        const float lowN = p.strength / mxL, midN = fmNear (p.frame) / mxM;
        if (midN > 1.4f * lowN) continue;
        evs.push_back ({ p.frame * B.hop, 36, lowN });
    }
    auto fhNear = [&] (int fr) { float m = 0; for (int k = -2; k <= 2; ++k) if (fr + k >= 0 && fr + k < (int) fh.size()) m = std::max (m, fh[(size_t) (fr + k)]); return m; };
    std::vector<int> coincide;
    for (auto& p : pkM)                                                                                                          // caixa ou palmas
    {
        if (fhNear (p.frame) > 1.0f * fmNear (p.frame) && ! near (pkB, p.frame, 2)) continue;                                  // dominado pelos agudos e sem corpo: é hi-hat
        if (flNear (p.frame) / mxL > 1.6f * (p.strength / mxM)) continue;                                                       // os graves dominam claramente: é só o bombo (o "click" do bombo não é caixa)
        const Peak* body = near (pkB, p.frame, 2); const bool snare = body != nullptr || fb[(size_t) p.frame] > 0.35f * fm[(size_t) p.frame];
        evs.push_back ({ p.frame * B.hop, snare ? 38 : 39, p.strength / mxM });
        coincide.push_back (p.frame);
    }
    {   // agudos que coincidem com uma caixa: só são hi-hat se forem tão fortes como os hi-hats normais (senão são o espectro da própria caixa)
        std::vector<float> alone; for (auto& h : pkH) { bool c = false; for (int fr : coincide) if (std::abs (fr - h.frame) <= 2) c = true; if (! c) alone.push_back (h.strength); }
        std::sort (alone.begin(), alone.end()); const float med = alone.empty() ? mxH * 0.5f : alone[alone.size() / 2];
        for (size_t i = 0; i < pkH.size(); ++i) { bool c = false; for (int fr : coincide) if (std::abs (fr - pkH[i].frame) <= 2) c = true; if (c && pkH[i].strength < 0.6f * med) usedH[i] = 1; }
    }
    for (size_t i = 0; i < pkH.size(); ++i)
    {
        if (usedH[i]) continue; const auto& p = pkH[i];
        // aberto: a energia dos agudos fica alta durante >110 ms
        const float pe = B.high[(size_t) p.frame]; int hold = 0; for (int k = p.frame + 1; k < B.frames && k < p.frame + (int) (0.35 / B.hop); ++k) { if (B.high[(size_t) k] > 0.38f * pe) ++hold; else break; }
        bool withSnare = false; for (int fr : coincide) if (std::abs (fr - p.frame) <= 2) withSnare = true;
        const bool open = ! withSnare && hold * B.hop > 0.11;           // junto a uma caixa a cauda é da caixa: fica fechado
        evs.push_back ({ p.frame * B.hop, open ? 46 : 42, p.strength / mxH });
    }
    // grelha
    std::vector<Peak> grid_pk; for (auto& e : evs) grid_pk.push_back ({ (int) std::lround (e.t / B.hop), e.strength });
    double ph = gridPhase (grid_pk, B.hop, bpm);
    // início do compasso 1: o 1.º ataque (de qualquer instrumento) cai na 1.ª semicolcheia
    double first = 1e9; for (auto& e : evs) first = std::min (first, e.t);
    double start = ph; while (start > first + grid * 0.4) start -= grid; while (start + grid <= first - grid * 0.4) start += grid;
    startSec = start;
    for (auto& e : evs)
    {
        const int st = stepsOf (e.t, start, grid); if (st < 0 || st >= 16 * 16) continue;
        GNote n; n.step = st; n.note = e.note; n.len = 1; n.vel = (int) std::lround (48.0 + 79.0 * std::pow (std::max (0.0f, std::min (1.0f, e.strength)), 0.6f)); notes.push_back (n);
    }
    // duplicados na mesma casa (mesmo instrumento): fica o mais forte
    std::sort (notes.begin(), notes.end(), [] (const GNote& a, const GNote& b) { return a.step != b.step ? a.step < b.step : (a.note != b.note ? a.note < b.note : a.vel > b.vel); });
    notes.erase (std::unique (notes.begin(), notes.end(), [] (const GNote& a, const GNote& b) { return a.step == b.step && a.note == b.note; }), notes.end());
    (void) pk;
}

// ---------------------------------------------------------------- melodia / baixo
inline void pitchedToNotes (const std::vector<float>& x, double sr, double& bpm, double& conf, std::vector<GNote>& notes, double& startSec, bool& isBass)
{
    double hop = 0; const auto P = trackPitch (x, sr, hop); if (P.size() < 16) return;
    float maxRms = 0; for (auto& p : P) maxRms = std::max (maxRms, p.rms); if (maxRms < 1e-4f) return;
    std::vector<int> mid (P.size(), -1);
    for (size_t i = 0; i < P.size(); ++i) if (P[i].f0 > 0 && P[i].conf > 0.6f && P[i].rms > 0.04f * maxRms) mid[i] = (int) std::lround (69.0 + 12.0 * std::log2 (P[i].f0 / 440.0));
    // filtro de mediana (5)
    std::vector<int> sm = mid; for (size_t i = 2; i + 2 < mid.size(); ++i) { int w[5] = { mid[i - 2], mid[i - 1], mid[i], mid[i + 1], mid[i + 2] }; std::sort (w, w + 5); if (mid[i] >= 0) sm[i] = w[2] >= 0 ? w[2] : mid[i]; }
    // ataques: envelope de energia para tempo + separação de notas repetidas
    std::vector<float> envE (P.size()); for (size_t i = 0; i < P.size(); ++i) envE[i] = P[i].rms;
    std::vector<float> fe = flux (envE);
    if (bpm <= 0) { const auto t = estimateTempo (fe, hop); bpm = t.bpm; conf = t.conf; } else conf = 1.0;
    if (bpm <= 0) return;
    struct N { double t0, t1; int note; float vel; }; std::vector<N> ns;
    int cur = -1; double t0 = 0; float vmax = 0; int missing = 0;
    auto close = [&] (size_t endIdx) { if (cur >= 0) { const double t1 = (double) endIdx * hop; if (t1 - t0 >= 0.045) ns.push_back ({ t0, t1, cur, vmax }); } cur = -1; };
    for (size_t i = 0; i < sm.size(); ++i)
    {
        const int m = sm[i]; const bool onset = i > 2 && fe[i] > 0.35f * *std::max_element (fe.begin(), fe.end()) && fe[i] > 0.02f;
        if (m < 0) { if (cur >= 0 && ++missing >= 3) { close (i - 2); missing = 0; } continue; }
        missing = 0;
        if (cur < 0) { cur = m; t0 = (double) i * hop; vmax = P[i].rms; continue; }
        if (m != cur && std::abs (m - cur) <= 12) { // mudança de nota: confirma com 2 quadros seguidos
            if (i + 1 < sm.size() && sm[i + 1] == m) { close (i); cur = m; t0 = (double) i * hop; vmax = P[i].rms; continue; } }
        else if (onset && m == cur && (double) i * hop - t0 > 0.09) { close (i); cur = m; t0 = (double) i * hop; vmax = P[i].rms; continue; }     // mesma nota repetida
        vmax = std::max (vmax, P[i].rms);
    }
    close (sm.size());
    if (ns.empty()) return;
    const double grid = 60.0 / bpm / 4.0; std::vector<Peak> pks; for (auto& n : ns) pks.push_back ({ (int) std::lround (n.t0 / hop), 1.0f });
    double ph = gridPhase (pks, hop, bpm); double first = ns.front().t0; double start = ph; while (start > first + grid * 0.4) start -= grid; while (start + grid <= first - grid * 0.4) start += grid; startSec = start;
    double avg = 0; for (auto& n : ns) avg += n.note; avg /= (double) ns.size(); isBass = avg < 52.0;
    for (auto& n : ns)
    {
        const int s0 = stepsOf (n.t0, start, grid); int s1 = stepsOf (n.t1, start, grid); if (s0 < 0 || s0 >= 256) continue; s1 = std::max (s0 + 1, s1);
        GNote g; g.step = s0; g.len = std::min (64, s1 - s0); g.note = std::max (0, std::min (127, n.note)); g.vel = (int) std::lround (55.0 + 70.0 * std::min (1.0f, n.vel / maxRms)); notes.push_back (g);
    }
}

// ---------------------------------------------------------------- acordes
inline void chordsToNotes (const std::vector<float>& x, double sr, double& bpm, double& conf, std::vector<GNote>& notes, double& startSec)
{
    // andamento a partir do envelope de energia total
    const auto B = analyseBands (x, sr); if (B.frames < 32) return;
    if (bpm <= 0) { const auto t = estimateTempo (flux (B.total), B.hop); bpm = t.bpm; conf = t.conf; } else conf = 1.0;
    if (bpm <= 0) return;
    const int N = pow2AtLeast (0.19 * sr), H = N / 4; Fft fft (N); const int n = (int) x.size(); if (n < N * 2) return;
    std::vector<float> win ((size_t) N); for (int i = 0; i < N; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos (2.0 * kPi * i / N);
    const int frames = (n - N) / H + 1; const double hopSec = (double) H / sr; const double binHz = sr / N;
    std::vector<std::array<float, 12>> chroma ((size_t) frames); std::vector<float> energy ((size_t) frames, 0.0f); std::vector<std::complex<float>> buf ((size_t) N);
    for (int f = 0; f < frames; ++f)
    {
        for (int i = 0; i < N; ++i) buf[(size_t) i] = x[(size_t) (f * H + i)] * win[(size_t) i];
        fft.run (buf); auto& c = chroma[(size_t) f]; c.fill (0.0f);
        for (int k = 1; k < N / 2; ++k)
        {
            const double hz = k * binHz; if (hz < 70 || hz > 2200) continue; const float mag = std::abs (buf[(size_t) k]);
            const double midi = 69.0 + 12.0 * std::log2 (hz / 440.0); const int pc = ((int) std::lround (midi) % 12 + 12) % 12; const double dev = std::fabs (midi - std::lround (midi));
            if (dev > 0.35) continue; c[(size_t) pc] += mag * mag / (1.0f + (float) (hz / 900.0)); energy[(size_t) f] += mag * mag;
        }
    }
    // início: 1.º quadro com energia
    float mxE = 0; for (float e : energy) mxE = std::max (mxE, e); if (mxE <= 0) return;
    int firstFrame = 0; while (firstFrame < frames && energy[(size_t) firstFrame] < 0.1f * mxE) ++firstFrame;
    const double beat = 60.0 / bpm; const double start = firstFrame * hopSec; startSec = start;
    const int nBeats = (int) std::floor ((frames * hopSec - start) / beat); if (nBeats < 1) return;
    struct T { const char* name; std::vector<int> iv; }; static const std::vector<T> tpl = { { "maj", { 0, 4, 7 } }, { "min", { 0, 3, 7 } }, { "dom7", { 0, 4, 7, 10 } }, { "min7", { 0, 3, 7, 10 } }, { "maj7", { 0, 4, 7, 11 } } };
    struct Ch { int root = -1, type = 0; };
    std::vector<Ch> perBeat ((size_t) nBeats);
    for (int b = 0; b < nBeats; ++b)
    {
        std::array<double, 12> c {}; double tot = 0; const int f0 = (int) ((start + b * beat) / hopSec), f1 = std::min (frames, (int) ((start + (b + 1) * beat) / hopSec));
        for (int f = f0; f < f1; ++f) for (int k = 0; k < 12; ++k) c[(size_t) k] += std::sqrt (chroma[(size_t) f][(size_t) k]);
        for (double v : c) tot += v * v; if (tot < 1e-9) continue;
        double norm = std::sqrt (tot); for (double& v : c) v /= norm;
        double best = -1; int bestRoot = -1, bestType = 0;
        for (int r = 0; r < 12; ++r) for (size_t ti = 0; ti < tpl.size(); ++ti)
        {
            double s = 0, tn = 0; std::array<double, 12> m {}; for (int iv : tpl[ti].iv) m[(size_t) ((r + iv) % 12)] = (iv == 0 ? 1.15 : 1.0);
            for (int k = 0; k < 12; ++k) { s += c[(size_t) k] * m[(size_t) k]; tn += m[(size_t) k] * m[(size_t) k]; } s /= std::sqrt (tn);
            if (ti >= 2) s -= 0.045;                                                  // as sétimas só ganham se forem claramente suportadas
            if (s > best) { best = s; bestRoot = r; bestType = (int) ti; }
        }
        if (best > 0.72) { perBeat[(size_t) b].root = bestRoot; perBeat[(size_t) b].type = bestType; }
    }
    // alisar: um acorde isolado de 1 tempo que difere dos vizinhos iguais é ruído
    for (int b = 1; b + 1 < nBeats; ++b) if (perBeat[(size_t) b - 1].root == perBeat[(size_t) b + 1].root && perBeat[(size_t) b - 1].type == perBeat[(size_t) b + 1].type && (perBeat[(size_t) b].root != perBeat[(size_t) b - 1].root || perBeat[(size_t) b].type != perBeat[(size_t) b - 1].type)) perBeat[(size_t) b] = perBeat[(size_t) b - 1];
    int b = 0;
    while (b < nBeats)
    {
        if (perBeat[(size_t) b].root < 0) { ++b; continue; }
        int e = b + 1; while (e < nBeats && perBeat[(size_t) e].root == perBeat[(size_t) b].root && perBeat[(size_t) e].type == perBeat[(size_t) b].type) ++e;
        const int s0 = b * 4, len = (e - b) * 4; const int root = perBeat[(size_t) b].root; const auto& iv = tpl[(size_t) perBeat[(size_t) b].type].iv;
        const int base = 48 + root;                                                       // fundamental no 3.º registo (Dó3..Si3), resto por cima, baixo uma oitava abaixo
        for (size_t k = 0; k < iv.size(); ++k) { GNote g; g.step = s0; g.len = len; g.note = base + iv[k]; g.vel = 92; notes.push_back (g); }
        { GNote g; g.step = s0; g.len = len; g.note = base - 12; g.vel = 80; notes.push_back (g); }
        b = e;
    }
}


// ---------------------------------------------------------------- tipo automático
// ruído nos agudos (hi-hats, pratos, caixas) em muitos quadros, ou pouca altura estável → bateria; vários sons simultâneos → acordes
inline std::string detectKind (const std::vector<float>& xin, double sr)
{
    const std::vector<float> x (xin.begin(), xin.begin() + (long) std::min ((double) xin.size(), 30.0 * sr));
    // 1) ruído nos agudos
    {
        const int N = (int) pow2AtLeast (0.0464 * sr), H = N / 2; Fft fft (N); std::vector<std::complex<float>> buf ((size_t) N); const double binHz = sr / N;
        const int h0 = std::max (1, (int) (5000.0 / binHz)), h1 = std::min (N / 2 - 1, (int) (std::min (16000.0, sr * 0.45) / binHz));
        std::vector<float> hiE, flat; double maxHi = 0;
        for (size_t s = 0; s + (size_t) N < x.size(); s += (size_t) H)
        {
            for (int i = 0; i < N; ++i) buf[(size_t) i] = x[s + (size_t) i] * (0.5f - 0.5f * std::cos (2.0 * kPi * i / N)); fft.run (buf);
            double lg = 0, ar = 0; int c = 0; for (int k = h0; k <= h1; ++k) { const double m = std::abs (buf[(size_t) k]) + 1e-9; lg += std::log (m); ar += m; ++c; }
            const double e = ar / std::max (1, c), fl = std::exp (lg / std::max (1, c)) / std::max (ar / std::max (1, c), 1e-12);
            hiE.push_back ((float) e); flat.push_back ((float) fl); maxHi = std::max (maxHi, e);
        }
        int noisy = 0, bursts = 0; for (size_t i = 0; i < hiE.size(); ++i) if (hiE[i] > 0.12 * maxHi && flat[i] > 0.45f) { ++noisy; if (i > 0 && hiE[i - 1] < 0.6f * hiE[i]) ++bursts; }
        const double secs = (double) hiE.size() * H / sr;       // só conta se os agudos ruidosos chegam em ataques (hi-hats, caixas), não ruído constante
        if (! hiE.empty() && (double) noisy / (double) hiE.size() >= 0.04 && (double) bursts / std::max (1.0, secs) >= 1.0) return "Bateria";
    }
    // 2) altura estável?
    double hop = 0; const auto P = trackPitch (x, sr, hop); float mx = 0; for (auto& p : P) mx = std::max (mx, p.rms);
    int act = 0, voiced = 0; for (auto& p : P) if (p.rms > 0.1f * mx) { ++act; if (p.f0 > 0 && p.conf > 0.8f) ++voiced; }
    const double vf = act ? (double) voiced / act : 0.0; double activeFrac = P.empty() ? 0.0 : (double) act / (double) P.size();
    if (vf < 0.30 || (activeFrac < 0.22 && vf < 0.9)) return "Bateria";
    // 3) quantas alturas fundamentais ao mesmo tempo (harmónicos retirados)?
    const int N = pow2AtLeast (0.19 * sr); Fft fft (N); std::vector<std::complex<float>> buf ((size_t) N); const double binHz = sr / N; std::vector<int> counts;
    for (size_t s = 0; s + (size_t) N < x.size() && counts.size() < 80; s += (size_t) (sr * 0.25))
    {
        double rms = 0; for (int i = 0; i < N; ++i) { buf[(size_t) i] = x[s + (size_t) i] * (0.5f - 0.5f * std::cos (2.0 * kPi * i / N)); rms += (double) x[s + (size_t) i] * x[s + (size_t) i]; }
        if (std::sqrt (rms / N) < 0.04) continue; fft.run (buf);
        const int k0 = std::max (2, (int) (60.0 / binHz)), k1 = std::min (N / 2 - 2, (int) (1500.0 / binHz)); std::vector<float> mag ((size_t) N / 2); float mxm = 0;
        for (int k = 0; k < N / 2; ++k) { mag[(size_t) k] = std::abs (buf[(size_t) k]); if (k >= k0 && k <= k1) mxm = std::max (mxm, mag[(size_t) k]); }
        std::vector<double> funds; 
        for (int k = k0; k <= k1; ++k)
        {
            if (mag[(size_t) k] < 0.25f * mxm || mag[(size_t) k] < mag[(size_t) k - 1] || mag[(size_t) k] < mag[(size_t) k + 1]) continue;
            const double a = mag[(size_t) k - 1], b = mag[(size_t) k], c = mag[(size_t) k + 1], den = a - 2 * b + c; const double f = (k + (std::fabs (den) > 1e-12 ? 0.5 * (a - c) / den : 0.0)) * binHz;
            bool harm = false; for (double fk : funds) { const double r = f / fk; const double nn = std::round (r); if (nn >= 2 && std::fabs (r - nn) / nn < 0.03) { harm = true; break; } }
            if (! harm) funds.push_back (f);
        }
        counts.push_back ((int) funds.size());
    }
    if (counts.empty()) return "Melodia"; std::sort (counts.begin(), counts.end()); const int med = counts[counts.size() / 2];
    return med >= 3 ? "Acordes" : "Melodia";
}

// ---------------------------------------------------------------- principal
inline Result convert (const std::vector<float>& monoIn, double sr, const Options& opt)
{
    Result R; if (monoIn.size() < (size_t) (sr * 0.5)) { R.error = "O \xC3\xA1udio \xC3\xA9 demasiado curto (menos de meio segundo)."; return R; }
    std::vector<float> x (monoIn.begin(), monoIn.begin() + (long) std::min ((double) monoIn.size(), opt.maxSeconds * sr));
    float pk = 0; for (float v : x) pk = std::max (pk, std::fabs (v)); if (pk < 1.0e-4f) { R.error = "O \xC3\xA1udio est\xC3\xA1 em sil\xC3\xAAncio."; return R; }
    for (float& v : x) v /= pk;
    std::string kind = opt.kind;
    if (kind == "auto") kind = detectKind (x, sr);
    double bpm = opt.bpm, conf = 0, startSec = 0; std::vector<GNote> notes; bool bass = false;
    if (kind == "Bateria") drumsToNotes (x, sr, bpm, conf, notes, startSec);
    else if (kind == "Acordes") chordsToNotes (x, sr, bpm, conf, notes, startSec);
    else { pitchedToNotes (x, sr, bpm, conf, notes, startSec, bass); if (opt.kind == "Baixo" || opt.kind == "Melodia") bass = (opt.kind == "Baixo"); kind = bass ? "Baixo" : "Melodia"; }
    if (notes.empty() || bpm <= 0) { R.error = "N\xC3\xA3o consegui encontrar notas neste \xC3\xA1udio. Tenta escolher o tipo manualmente (bateria, baixo, melodia ou acordes) ou indicar o BPM."; return R; }
    int maxStep = 0; for (auto& n : notes) maxStep = std::max (maxStep, n.step + n.len);
    Groove g; g.style = opt.style; g.name = opt.name; g.kind = kind; g.bpm = (int) std::lround (std::max (40.0, std::min (240.0, bpm))); g.bars = std::max (1, std::min (16, (maxStep + 15) / 16)); if (g.bars == 3) g.bars = 4;
    g.notes = notes; g.user = true;
    const int total = g.totalSteps(); g.notes.erase (std::remove_if (g.notes.begin(), g.notes.end(), [total] (const GNote& n) { return n.step >= total; }), g.notes.end());
    for (auto& n : g.notes) n.len = std::max (1, std::min (total - n.step, n.len));
    std::stable_sort (g.notes.begin(), g.notes.end(), [] (const GNote& a, const GNote& b) { return a.step < b.step; });
    R.ok = true; R.groove = std::move (g); R.bpm = bpm; R.confidence = conf; R.detectedKind = kind; R.noteCount = (int) R.groove.notes.size(); return R;
}

// ---------------------------------------------------------------- análise de loops (BPM + nº de tempos)
struct LoopInfoA { double bpm = 0; int beats = 0; double conf = 0; bool fromName = false; };
// 'mono' = loop inteiro; 'hintBpm' > 0 (ex.: "96bpm" no nome do ficheiro) fixa o andamento. O resultado cabe EXATAMENTE
// no comprimento: bpm = 60 * beats / duração.
inline LoopInfoA analyseLoop (const std::vector<float>& mono, double sr, double hintBpm = 0)
{
    LoopInfoA R; const double dur = sr > 0 ? (double) mono.size() / sr : 0; if (dur < 0.2) return R;
    auto finish = [&] (int beats) { R.beats = std::max (1, beats); R.bpm = 60.0 * R.beats / dur; };
    if (hintBpm >= 40 && hintBpm <= 300) { finish ((int) std::lround (dur * hintBpm / 60.0)); R.conf = 1; R.fromName = true; return R; }
    Tempo t; if (mono.size() > 4096) { const auto B = analyseBands (mono, sr); if (B.frames >= 64) t = estimateTempo (flux (B.total), B.hop, 60, 190); }
    static const int cand[] = { 1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64 };
    double best = -1e30; int bestB = 4;
    for (int b : cand)
    {
        const double bpm = 60.0 * b / dur; if (bpm < 55 || bpm > 200) continue;
        double s = -0.5 * std::pow (std::log2 (bpm / 105.0) / 0.8, 2.0);                       // prior suave
        if (b % 4 == 0) s += 0.35; if (b == 4 || b == 8 || b == 16 || b == 32) s += 0.25;       // compassos inteiros de 4/4
        if (t.bpm > 0) s -= std::fabs (std::log2 (bpm / t.bpm)) * (1.0 + 3.0 * t.conf) * 3.0;
        if (s > best) { best = s; bestB = b; }
    }
    finish (bestB); R.conf = t.conf; return R;
}
}} // namespace sk::a2m
