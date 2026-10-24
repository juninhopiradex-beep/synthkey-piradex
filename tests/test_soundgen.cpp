// Testes do gerador "descrever som" (regras → preset): interpretação do texto, sons finitos, volume calibrado, escuro < brilhante, curto < longo
#include "Core/SoundGen.h"
#include <cstdio>
#include <cmath>
using namespace sk;
static int fails = 0, checks = 0;
#define CHECK(c, msg) do { ++checks; if (! (c)) { ++fails; std::printf("FALHOU: %s\n", msg); } } while (0)

static std::vector<float> render (const Preset& pr, double sr, double seconds, int note = 57, double releaseAt = 1e9)
{
    static float P[P_COUNT]; for (int i = 0; i < P_COUNT; ++i) P[i] = pr.get (i);
    Engine e; e.prepare (sr, 512); e.setParams (P); e.noteOn (note, 0.85f);
    std::vector<float> out, L (512), R (512); const int blocks = (int) (sr * seconds / 512.0);
    for (int b = 0; b < blocks; ++b) { if ((double) b * 512.0 / sr >= releaseAt) { e.noteOff (note); releaseAt = 1e9; } e.render (L.data(), R.data(), 512); for (int i = 0; i < 512; ++i) out.push_back (0.5f * (L[(size_t) i] + R[(size_t) i])); }
    return out;
}
// centroide espectral (Hz) por DFT de uma janela de 4096 amostras depois do ataque
static double centroid (const std::vector<float>& x, double sr)
{
    const int N = 4096; const size_t off = (size_t) (0.25 * sr); if (x.size() < off + N) return 0;
    double num = 0, den = 0;
    for (int k = 1; k < N / 2; k += 2)
    {
        double re = 0, im = 0; for (int n = 0; n < N; ++n) { const double w = 0.5 - 0.5 * std::cos (2.0 * M_PI * n / N); const double v = x[off + (size_t) n] * w; re += v * std::cos (2.0 * M_PI * k * n / N); im -= v * std::sin (2.0 * M_PI * k * n / N); }
        const double m = std::sqrt (re * re + im * im); num += m * k * sr / N; den += m;
    }
    return den > 0 ? num / den : 0;
}
// tempo (s) até o nível cair abaixo de -40 dB do pico, depois de largar a nota aos 0.2 s
static double decayTime (const std::vector<float>& x, double sr)
{
    float pk = 0; for (float v : x) pk = std::max (pk, std::fabs (v)); if (pk <= 0) return 0;
    const size_t w = (size_t) (0.02 * sr); size_t last = 0;
    for (size_t i = 0; i + w < x.size(); i += w) { float m = 0; for (size_t j = 0; j < w; ++j) m = std::max (m, std::fabs (x[i + j])); if (m > pk * 0.01f) last = i; }
    return (double) last / sr;
}

int main()
{
    const double sr = 48000.0;
    // [1] interpretação
    CHECK (soundgen::parse ("baixo escuro e agressivo").kind == soundgen::K_BASS, "baixo → BASS");
    CHECK (soundgen::parse ("808 drill").kind == soundgen::K_808, "808 → 808");
    CHECK (soundgen::parse ("pad quente e largo").kind == soundgen::K_PAD, "pad → PAD");
    CHECK (soundgen::parse ("amapiano log drum").kind == soundgen::K_LOG, "log drum → LOG");
    CHECK (soundgen::parse ("Órgão gospel").kind == soundgen::K_ORGAN, "órgão (acentos/maiúsculas)");
    CHECK (soundgen::parse ("bright short pluck").kind == soundgen::K_PLUCK, "inglês: pluck");
    CHECK (soundgen::parse ("xyzzy").kind == soundgen::K_NONE, "texto desconhecido → sem tipo");
    // [2] todas as saídas são finitas, têm nome e não estouram
    const char* prompts[] = { "baixo escuro e agressivo", "pad quente e largo", "bright short pluck", "808 drill", "lead brilhante com vibrato", "amapiano log drum", "sino cristalino longo e espaçoso",
        "kizomba teclas suaves", "kuduro lead agressivo", "xyzzy", "cordas lentas e largas", "órgão gospel", "riser evolutivo", "vocal coro etéreo", "arp rítmico brilhante", "" };
    for (auto* t : prompts)
        for (unsigned seed = 1; seed <= 3; ++seed)
        {
            const Preset p = soundgen::generateCalibrated (t, seed, sr); float pk = 0; const float rms = soundgen::measureRms (p, sr, &pk);
            CHECK (! p.name.empty(), "nome não vazio");
            bool ok = true; for (int i = 0; i < P_COUNT; ++i) { const float v = p.get (i); if (! std::isfinite (v) || v < kParams[i].mn - 1e-4f || v > kParams[i].mx + 1e-4f) { ok = false; std::printf("  '%s' seed %u: %s = %g (%g..%g)\n", t, seed, kParams[i].id, v, kParams[i].mn, kParams[i].mx); } }
            CHECK (ok, "parâmetros finitos e dentro dos limites");
            CHECK (std::isfinite (rms) && rms > 0.01f, "o som não é silencioso");
            CHECK (pk < 1.2f, "pico controlado");
            const float db = 20.0f * std::log10 (rms + 1e-9f);
            CHECK (db > -26.0f && db < -9.0f, "volume calibrado (-26..-9 dB RMS)");
        }
    // [3] mesma entrada + mesma semente = mesmo preset; sementes diferentes = variantes
    { const auto a = soundgen::generate ("pad quente", 5), b = soundgen::generate ("pad quente", 5), c = soundgen::generate ("pad quente", 6);
      bool same = true, diff = false; for (int i = 0; i < P_COUNT; ++i) { if (a.get (i) != b.get (i)) same = false; if (a.get (i) != c.get (i)) diff = true; }
      CHECK (same, "determinístico"); CHECK (diff, "sementes diferentes dão variantes"); }
    // [4] escuro < brilhante (centroide espectral)
    { const auto d = render (soundgen::generateCalibrated ("lead escuro", 1, sr), sr, 1.2), b = render (soundgen::generateCalibrated ("lead brilhante", 1, sr), sr, 1.2);
      const double cd = centroid (d, sr), cb = centroid (b, sr); std::printf("centroide escuro=%.0f Hz  brilhante=%.0f Hz\n", cd, cb); CHECK (cd > 0 && cb > cd, "brilhante tem centroide maior que escuro"); }
    // [5] curto < longo
    { const auto s = render (soundgen::generateCalibrated ("pluck curto", 1, sr), sr, 5.0, 57, 0.2), l = render (soundgen::generateCalibrated ("pad longo", 1, sr), sr, 5.0, 57, 0.2);
      const double ds = decayTime (s, sr), dl = decayTime (l, sr); std::printf("duração curto=%.2f s  longo=%.2f s\n", ds, dl); CHECK (dl > ds, "longo dura mais que curto"); }
    std::printf("%d verificações, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
