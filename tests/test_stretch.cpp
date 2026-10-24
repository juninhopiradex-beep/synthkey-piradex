#include "Core/TimeStretch.h"
#include <cstdio>
#include <cmath>
#include <vector>
#include <string>
static int fails = 0, checks = 0;
#define CHECK(c, msg) do { ++checks; if (! (c)) { ++fails; std::printf ("FALHOU: %s (linha %d)\n", msg, __LINE__); } } while (0)
int main()
{
    const double sr = 44100;
    // 1) seno 440 Hz: a altura tem de se manter e o RMS também
    for (double r : { 0.75, 0.9, 1.1, 1.25, 1.6 })
    {
        const int n = (int) (sr * 2.0); std::vector<float> s ((size_t) n);
        const double f = 441.0; for (int i = 0; i < n; ++i) s[(size_t) i] = 0.6f * std::sin (2 * 3.14159265358979 * f * i / sr);
        const int outLen = (int) std::lround (n * r); std::vector<std::vector<float>> o;
        sk::tstretch::stretchCircular ({ s.data() }, n, sr, outLen, o);
        CHECK ((int) o[0].size() == outLen, "comprimento de saída");
        double rms = 0; bool bad = false; for (int i = 1; i < outLen; ++i) { rms += o[0][(size_t) i] * o[0][(size_t) i]; if (! std::isfinite (o[0][(size_t) i])) bad = true; }
        rms = std::sqrt (rms / outLen);
        int bestLag = 0; double bestC = -1; for (int lag = 60; lag < 160; ++lag) { double c = 0; for (int i = 4000; i < outLen - 4000 - 200; ++i) c += (double) o[0][(size_t) i] * o[0][(size_t) (i + lag)]; if (c > bestC) { bestC = c; bestLag = lag; } }
        const double fo = sr / bestLag;
        double glitch = 0; { double sumErr = 0; for (int i = 2; i < outLen; ++i) { const double d2 = o[0][(size_t) i] - 2 * o[0][(size_t) i - 1] + o[0][(size_t) i - 2]; glitch = std::max (glitch, std::fabs (d2)); sumErr += d2 * d2; } (void) sumErr; }
        std::printf ("   maior segunda diferença %.4f (seno puro: %.4f)\n", glitch, 0.6 * std::pow (2 * 3.14159 * 441 / sr, 2));
        std::printf ("seno ratio %.2f: altura %.1f Hz (alvo 441), rms %.3f (alvo 0.424)\n", r, fo, rms);
        CHECK (! bad, "sem NaN"); CHECK (std::fabs (fo - 441.0) < 5.0, "altura mantida"); CHECK (glitch < 0.05, "sem estalidos"); CHECK (std::fabs (rms - 0.424) < 0.06, "volume mantido");
        const double seam = std::fabs (o[0][0] - o[0][(size_t) outLen - 1]); CHECK (seam < 0.25, "emenda do loop sem salto grande");
    }
    // 2) cliques em batidas: depois de esticar, os cliques ficam nas batidas novas (±12 ms)
    {
        const double bpm0 = 90, bpm1 = 120; const int beats = 8; const int n = (int) std::lround (beats * 60.0 / bpm0 * sr); std::vector<float> s ((size_t) n, 0.0f);
        for (int b = 0; b < beats; ++b) { const int p = (int) std::lround (b * 60.0 / bpm0 * sr); for (int k = 0; k < 600; ++k) if (p + k < n) s[(size_t) (p + k)] = 0.8f * std::exp (-k / 90.0f) * std::sin (2 * 3.14159 * 180.0 * k / sr * 6); }
        const int outLen = (int) std::lround (beats * 60.0 / bpm1 * sr); std::vector<std::vector<float>> o;
        sk::tstretch::stretchCircular ({ s.data() }, n, sr, outLen, o, beats * 4);
        int found = 0; double maxErrMs = 0; for (int b = 0; b < beats; ++b)
        {
            const int p = (int) std::lround (b * 60.0 / bpm1 * sr); int lo = std::max (0, p - (int) (0.02 * sr)), hi = std::min (outLen, p + (int) (0.02 * sr)); float pk = 0; int at = lo; for (int i = lo; i < hi; ++i) if (std::fabs (o[0][(size_t) i]) > pk) { pk = std::fabs (o[0][(size_t) i]); at = i; }
            (void) at; { int first = lo; while (first < hi && std::fabs (o[0][(size_t) first]) < 0.15f) ++first; maxErrMs = std::max (maxErrMs, std::fabs ((first - p) * 1000.0 / sr)); }
            float away = 0; const int q = p + (int) (0.22 * sr); for (int i = q; i < std::min (outLen, q + 800); ++i) away = std::max (away, std::fabs (o[0][(size_t) i]));
            if (pk > 0.25f && pk > away * 2.0f) ++found;
        }
        std::printf ("cliques 90->120 bpm: %d/%d batidas no sítio, erro máx. do ataque %.2f ms\n", found, beats, maxErrMs); CHECK (maxErrMs < 2.5, "ataques dentro de 2.5 ms da grelha"); CHECK (found >= beats - 1, "transientes nas batidas");
    }
    // 3) estéreo
    { const int n = 44100; std::vector<float> a ((size_t) n), b ((size_t) n); for (int i = 0; i < n; ++i) { a[(size_t) i] = std::sin (i * 0.05f); b[(size_t) i] = -a[(size_t) i]; }
      std::vector<std::vector<float>> o; sk::tstretch::stretchCircular ({ a.data(), b.data() }, n, 44100, 52000, o);
      double d = 0; for (int i = 0; i < 52000; ++i) d = std::max (d, (double) std::fabs (o[0][(size_t) i] + o[1][(size_t) i])); CHECK (d < 1e-4, "estéreo mantém a relação entre canais"); }
    std::printf ("%d verificações, %d falhas\n", checks, fails); return fails ? 1 : 0;
}
