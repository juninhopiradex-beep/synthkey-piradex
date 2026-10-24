// =============================================================================
//  SYNTHKEY by Piradex — importação de áudio do utilizador (WAV / AIFF / FLAC)
//  Descodifica ficheiros, deteta a nota (nome do ficheiro ou autocorrelação) e decide se é tonal ou percussivo.
// =============================================================================
#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include "Core/Samples.h"
#include <cmath>

namespace useraudio
{
constexpr int kMaxSeconds = 60;                         // limite de segurança por ficheiro

inline juce::AudioFormatManager& formats()
{
    static juce::AudioFormatManager m; static bool init = false;
    if (! init) { m.registerBasicFormats(); init = true; }
    return m;
}

inline bool isAudioFile (const juce::File& f) { return f.hasFileExtension ("wav;aif;aiff;flac;ogg"); }

// descodifica para floats (mono ou estéreo); devolve erro ou ""
inline juce::String decode (const juce::File& f, sk::SampleData& out)
{
    if (! f.existsAsFile()) return juce::CharPointer_UTF8 ("Ficheiro n\xC3\xA3o encontrado.");
    std::unique_ptr<juce::AudioFormatReader> rd (formats().createReaderFor (f));
    if (! rd) return juce::CharPointer_UTF8 ("Formato de \xC3\xA1udio n\xC3\xA3o suportado (usa WAV, AIFF ou FLAC).");
    if (rd->lengthInSamples < 64) return juce::CharPointer_UTF8 ("Ficheiro demasiado curto.");
    if (rd->sampleRate <= 0 || rd->lengthInSamples > (juce::int64) (rd->sampleRate * kMaxSeconds)) return juce::CharPointer_UTF8 ("Ficheiro demasiado longo (m\xC3\xA1ximo 60 segundos).");
    const int n = (int) rd->lengthInSamples; const int ch = (int) std::min<unsigned> (rd->numChannels, 2);
    juce::AudioBuffer<float> b (ch, n);
    if (! rd->read (&b, 0, n, 0, true, ch > 1)) return juce::CharPointer_UTF8 ("N\xC3\xA3o foi poss\xC3\xADvel ler o ficheiro.");
    out.sampleRate = (float) rd->sampleRate;
    out.l.assign (b.getReadPointer (0), b.getReadPointer (0) + n);
    if (ch > 1) out.r.assign (b.getReadPointer (1), b.getReadPointer (1) + n); else out.r.clear();
    return {};
}

inline std::vector<float> toMono (const sk::SampleData& d)
{
    std::vector<float> m = d.l;
    if (d.stereo()) for (size_t i = 0; i < m.size(); ++i) m[i] = 0.5f * (d.l[i] + d.r[i]);
    return m;
}

// "Piano_C#3.wav", "bass-a2", "Pad Eb4" → nota MIDI (C3 = 60, como no Ableton/FL); -1 se não houver
inline int noteFromName (const juce::String& name)
{
    const juce::String s = name.toUpperCase();
    for (int i = 0; i < s.length(); ++i)
    {
        const juce::juce_wchar c = s[i]; if (c < 'A' || c > 'G') continue;
        if (i > 0 && juce::CharacterFunctions::isLetter (s[i - 1])) continue;           // início de palavra
        int j = i + 1, acc = 0; if (j < s.length() && (s[j] == '#' || s[j] == 'S')) { acc = 1; ++j; } else if (j < s.length() && name[j] == 'b') { acc = -1; ++j; }
        bool neg = false; if (j < s.length() && s[j] == '-') { neg = true; ++j; }
        if (j >= s.length() || ! juce::CharacterFunctions::isDigit (s[j])) continue;
        const int oct = (int) (s[j] - '0') * (neg ? -1 : 1);
        if (j + 1 < s.length() && juce::CharacterFunctions::isLetter (s[j + 1])) continue;
        static const int semi[7] = { 9, 11, 0, 2, 4, 5, 7 };                            // A B C D E F G
        return std::max (0, std::min (127, 60 + (oct - 3) * 12 + semi[c - 'A'] + acc));
    }
    return -1;
}

// autocorrelação normalizada sobre a parte estável do som; devolve a nota MIDI ou -1 (sem altura clara = percussivo)
inline int detectPitch (const std::vector<float>& x, double sr)
{
    const int n = (int) x.size(); if (n < 2048) return -1;
    int peakAt = 0; float pk = 0; for (int i = 0; i < n; ++i) if (std::fabs (x[(size_t) i]) > pk) { pk = std::fabs (x[(size_t) i]); peakAt = i; }
    if (pk < 1.0e-4f) return -1;
    const int W = 4096, start = std::min (std::max (0, peakAt + (int) (sr * 0.03)), std::max (0, n - W - 2400));
    const int minLag = std::max (20, (int) (sr / 2000.0)), maxLag = std::min (2400, (int) (sr / 30.0));
    if (start + W + maxLag >= n) return -1;
    double e0 = 0; for (int i = 0; i < W; ++i) e0 += (double) x[(size_t) (start + i)] * x[(size_t) (start + i)];
    if (e0 < 1.0e-6) return -1;
    std::vector<float> r ((size_t) maxLag + 2, 0.0f);
    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        double c = 0, e1 = 0;
        for (int i = 0; i < W; ++i) { const double a = x[(size_t) (start + i)], b = x[(size_t) (start + i + lag)]; c += a * b; e1 += b * b; }
        r[(size_t) lag] = (float) (c / std::sqrt (e0 * e1 + 1.0e-12));
    }
    float best = 0; for (int lag = minLag; lag <= maxLag; ++lag) best = std::max (best, r[(size_t) lag]);
    if (best < 0.8f) return -1;
    int lag = minLag; for (int l = minLag + 1; l < maxLag; ++l) if (r[(size_t) l] >= best * 0.93f && r[(size_t) l] >= r[(size_t) l - 1] && r[(size_t) l] >= r[(size_t) l + 1]) { lag = l; break; }
    const double a = r[(size_t) lag - 1], b = r[(size_t) lag], c = r[(size_t) lag + 1], den = a - 2 * b + c;
    const double off = std::fabs (den) > 1e-9 ? 0.5 * (a - c) / den : 0.0;
    const double f = sr / ((double) lag + off); if (f < 25 || f > 4200) return -1;
    return std::max (0, std::min (127, (int) std::lround (69.0 + 12.0 * std::log2 (f / 440.0))));
}

// instrumento a partir de um ficheiro (deteção de altura → senão nota no nome → senão percussivo/altura fixa)
inline bool makeInstrument (const juce::File& f, sk::Library& lib, juce::String& err)
{
    auto sd = std::make_shared<sk::SampleData>(); err = decode (f, *sd); if (err.isNotEmpty()) return false;
    float pk = 0; for (float v : sd->l) pk = std::max (pk, std::fabs (v)); for (float v : sd->r) pk = std::max (pk, std::fabs (v));
    if (pk < 1.0e-5f) { err = juce::CharPointer_UTF8 ("O ficheiro est\xC3\xA1 em sil\xC3\xAAncio."); return false; }
    const auto mono = toMono (*sd); const auto stem = f.getFileNameWithoutExtension();
    // a altura medida manda (os nomes de oitava variam entre programas: C3 pode ser 130 ou 261 Hz); o nome só serve se a medição falhar
    int root = detectPitch (mono, sd->sampleRate); bool fixed = false;
    if (root < 0) { root = noteFromName (stem); if (root < 0) { fixed = true; root = 60; } }
    sk::Instrument in; in.id = "user." + juce::String (stem).retainCharacters ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-# ").replaceCharacter (' ', '_').toStdString();
    in.name = stem.toStdString(); in.cat = "Utilizador"; in.pack = "user"; in.fixedPitch = fixed;
    sk::Zone z; z.data = lib.addData (sd); z.root = root; z.gain = std::min (4.0f, 0.9f / pk); z.file = f.getFileName().toStdString();
    in.zones.push_back (z); lib.inst.push_back (std::move (in)); return true;
}
}
