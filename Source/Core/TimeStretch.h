// =============================================================================
//  SYNTHKEY by Piradex — esticar/encolher no tempo SEM mudar a altura (WSOLA, circular)
//
//  Usado pelos loops sincronizados com o tempo do projeto. O sinal é tratado como circular: o fim liga
//  ao início sem emenda, por isso o loop esticado continua a fazer loop perfeito. Corre numa thread
//  de fundo (nunca no áudio).
//
//  Dois modos (escolhidos pelo conteúdo):
//   · rítmico  — o loop é cortado em semicolcheias e cada fatia é esticada separadamente, ancorada na sua
//                posição exata na grelha nova: os ataques (bombo, caixa, notas) ficam sempre no sítio certo;
//   · tónico   — WSOLA global com procura larga, para pads e acordes sustentados.
// =============================================================================
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace sk { namespace tstretch
{
namespace detail
{
struct Ext
{
    int nch = 0, n = 0, pre = 0; std::vector<std::vector<float>> X; std::vector<float> M;
    Ext (const std::vector<const float*>& in, int len, int preLen, int postLen) : nch ((int) in.size()), n (len), pre (preLen)
    {
        X.resize ((size_t) nch); const int total = pre + n + postLen;
        for (int c = 0; c < nch; ++c) { X[(size_t) c].resize ((size_t) total); for (int i = 0; i < total; ++i) { int j = (i - pre) % n; if (j < 0) j += n; X[(size_t) c][(size_t) i] = in[(size_t) c][j]; } }
        M.resize ((size_t) total); for (int i = 0; i < total; ++i) { float s = 0; for (int c = 0; c < nch; ++c) s += X[(size_t) c][(size_t) i]; M[(size_t) i] = s / (float) nch; }
    }
};

// WSOLA de um troço: lê do sinal estendido a partir de 'base' (índice no estendido), 'srcLen' amostras, produz 'outLen' amostras.
// firstFixed: o 1.º quadro vem exatamente de 'base' (âncora). Devolve acumulado/pesos já normalizados em 'out' (nch × outLen).
inline void wsolaRange (const Ext& E, int base, int srcLen, int outLen, int N, int S, bool firstFixed, std::vector<std::vector<float>>& out)
{
    const int nch = E.nch; out.assign ((size_t) nch, std::vector<float> ((size_t) outLen, 0.0f));
    if (outLen <= 0 || srcLen <= 0) return;
    N = std::max (64, N & ~1); const int Hs = N / 2; const double Ha = (double) Hs * (double) srcLen / (double) outLen;
    std::vector<float> win ((size_t) N); for (int i = 0; i < N; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos (2.0 * 3.14159265358979323846 * (i + 0.5) / N);
    std::vector<float> wsum ((size_t) outLen, 0.0f);
    const int total = (int) E.M.size(); const int frames = (int) std::ceil ((double) outLen / Hs) + 1;
    int prevPos = -1;
    for (int k = 0; k < frames; ++k)
    {
        int pos = base + (int) std::lround ((double) k * Ha);
        if (prevPos >= 0 && S > 0)
        {
            const int tpl = prevPos + Hs; int bestOff = 0; double best = -1e30; const int lo = std::max (-S, -pos), hi = std::min (S, total - N - 1 - pos);
            if (tpl + N < total) for (int d = lo; d <= hi; ++d)
            {
                const int p = pos + d; double c = 0, e = 1e-9;
                for (int i = 0; i < N; i += 2) { const double a = E.M[(size_t) (p + i)], b = E.M[(size_t) (tpl + i)]; c += a * b; e += a * a; }
                const double score = c / std::sqrt (e); if (score > best) { best = score; bestOff = d; }
            }
            pos += bestOff;
        }
        pos = std::max (0, std::min (total - N - 1, pos));
        if (k == 0 && ! firstFixed) {}
        const int o0 = k * Hs;
        for (int i = 0; i < N; ++i)
        {
            const int oi = o0 + i; if (oi >= outLen) break;
            const float w = (k == 0 && firstFixed && i < Hs) ? 1.0f : win[(size_t) i];            // a âncora não leva rampa de entrada
            wsum[(size_t) oi] += w; for (int c = 0; c < nch; ++c) out[(size_t) c][(size_t) oi] += E.X[(size_t) c][(size_t) (pos + i)] * w;
        }
        prevPos = pos;
    }
    for (int c = 0; c < nch; ++c) for (int i = 0; i < outLen; ++i) out[(size_t) c][(size_t) i] /= std::max (wsum[(size_t) i], 0.2f);
}
}

// há ataques claros em muitas fatias de semicolcheia? (bateria, baixo e notas rítmicas)
inline bool looksRhythmic (const std::vector<const float*>& in, int n, double sr, int slices)
{
    if (slices < 4 || n < slices * 64) return false;
    const int w = std::max (16, (int) (0.008 * sr)); int attacks = 0;
    for (int k = 0; k < slices; ++k)
    {
        const long a = (long) ((double) n * k / slices); double ea = 1e-9, eb = 1e-9;
        for (int i = 0; i < w; ++i)
        {
            float x = 0, y = 0; for (auto* ch : in) { x += ch[(a + i) % n]; y += ch[((a - 1 - i) % n + n) % n]; }
            ea += (double) x * x; eb += (double) y * y;
        }
        if (ea > 2.5 * eb && ea > 1.0e-5 * w) ++attacks;
    }
    return attacks >= std::max (2, slices / 8);
}

// 'in' = 1 ou 2 canais. 'slicesPerLoop' = nº de semicolcheias do loop (beats × 4); 0 = só modo tónico.
inline void stretchCircular (const std::vector<const float*>& in, int n, double sr, int outLen, std::vector<std::vector<float>>& out, int slicesPerLoop = 0)
{
    const int nch = (int) in.size(); out.assign ((size_t) nch, std::vector<float> ((size_t) std::max (0, outLen), 0.0f));
    if (n < 64 || outLen < 64 || nch < 1) { for (int c = 0; c < nch; ++c) for (int i = 0; i < outLen && n > 0; ++i) out[(size_t) c][(size_t) i] = in[(size_t) c][i % n]; return; }
    if (std::abs (outLen - n) <= std::max (1, n / 2000)) { for (int c = 0; c < nch; ++c) for (int i = 0; i < outLen; ++i) out[(size_t) c][(size_t) i] = in[(size_t) c][i % n]; return; }
    const double ratio = (double) outLen / (double) n;

    if (slicesPerLoop >= 4 && looksRhythmic (in, n, sr, slicesPerLoop) && n / slicesPerLoop > (int) (0.03 * sr))
    {
        // ---------------- modo rítmico: fatias ancoradas na grelha
        const int T = std::max (16, (int) (0.003 * sr));                              // metade da zona de fusão (3 ms)
        const int N = (int) (0.020 * sr) & ~1, S = std::max (8, (int) (0.003 * sr));
        const int Tsrc = (int) std::lround ((double) T / ratio);
        const int pre = Tsrc + S + N + 8, post = N + S + Tsrc + 8 + n / slicesPerLoop;
        detail::Ext E (in, n, pre, post);
        std::vector<std::vector<float>> acc ((size_t) nch, std::vector<float> ((size_t) outLen, 0.0f));
        for (int k = 0; k < slicesPerLoop; ++k)
        {
            const long a0 = (long) ((double) n * k / slicesPerLoop), a1 = (long) ((double) n * (k + 1) / slicesPerLoop);
            const long o0 = (long) ((double) outLen * k / slicesPerLoop), o1 = (long) ((double) outLen * (k + 1) / slicesPerLoop);
            const int sl = (int) (a1 - a0) + 2 * Tsrc, ol = (int) (o1 - o0) + 2 * T;
            std::vector<std::vector<float>> seg; detail::wsolaRange (E, pre + (int) a0 - Tsrc, sl, ol, N, S, true, seg);
            for (int i = 0; i < ol; ++i)
            {
                float w = 1.0f; if (i < 2 * T) w = (float) i / (float) (2 * T); else if (i >= ol - 2 * T) w = (float) (ol - 1 - i) / (float) (2 * T);
                long gi = (o0 - T + i) % outLen; if (gi < 0) gi += outLen;
                for (int c = 0; c < nch; ++c) acc[(size_t) c][(size_t) gi] += seg[(size_t) c][(size_t) i] * w;
            }
        }
        out = std::move (acc); return;
    }

    // ---------------- modo tónico: WSOLA circular global
    int N = (int) (0.046 * sr) & ~1; N = std::max (256, std::min (N, n / 2)); N &= ~1;
    const int S = std::max (16, std::min (N / 2, (int) (0.012 * sr)));
    const int pre = S + 4, post = N + 2 * S + 8 + (int) (N * 2.0);
    detail::Ext E (in, n, pre, post);
    // um único troço circular: o 1.º quadro parte de 0 e o resultado é lido como circular
    const int Hs = N / 2; const double Ha = (double) Hs / ratio;
    std::vector<float> win ((size_t) N); for (int i = 0; i < N; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos (2.0 * 3.14159265358979323846 * (i + 0.5) / N);
    std::vector<std::vector<float>> acc ((size_t) nch, std::vector<float> ((size_t) outLen, 0.0f)); std::vector<float> wsum ((size_t) outLen, 0.0f);
    const int frames = (int) std::ceil ((double) outLen / Hs) + 1; const int total = (int) E.M.size(); int prevPos = -1;
    for (int k = 0; k < frames; ++k)
    {
        int pos = pre + (int) std::lround ((double) k * Ha);
        if (prevPos >= 0)
        {
            const int tpl = prevPos + Hs; int bestOff = 0; double best = -1e30; const int lo = std::max (-S, -pos), hi = std::min (S, total - N - 1 - pos);
            if (tpl + N < total) for (int d = lo; d <= hi; ++d)
            {
                const int p = pos + d; double c = 0, e = 1e-9;
                for (int i = 0; i < N; i += 3) { const double a = E.M[(size_t) (p + i)], b = E.M[(size_t) (tpl + i)]; c += a * b; e += a * a; }
                const double score = c / std::sqrt (e); if (score > best) { best = score; bestOff = d; }
            }
            pos += bestOff;
        }
        pos = std::max (0, std::min (total - N - 1, pos));
        const int o0 = k * Hs;
        for (int i = 0; i < N; ++i)
        {
            const int oi = (o0 + i) % outLen; const float w = win[(size_t) i]; wsum[(size_t) oi] += w;
            for (int c = 0; c < nch; ++c) acc[(size_t) c][(size_t) oi] += E.X[(size_t) c][(size_t) (pos + i)] * w;
        }
        prevPos = pos;
    }
    for (int c = 0; c < nch; ++c) for (int i = 0; i < outLen; ++i) out[(size_t) c][(size_t) i] = acc[(size_t) c][(size_t) i] / std::max (wsum[(size_t) i], 0.2f);
}
}} // namespace sk::tstretch
