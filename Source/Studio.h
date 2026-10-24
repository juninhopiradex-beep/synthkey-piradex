// =============================================================================
//  SYNTHKEY by Piradex — Estúdio de conteúdo (só administrador)
//  Projeto = pasta com os WAV copiados + projeto.json. Gera um pack .bfpack protegido (samples cifrados),
//  com instrumentos, kit de bateria automático, presets, grooves e capa — pronto a instalar ou a vender com chave.
// =============================================================================
#pragma once
#include <juce_graphics/juce_graphics.h>
#include "Access.h"
#include "AppPaths.h"
#include "GrooveStore.h"
#include "PackHub.h"
#include "UserAudio.h"
#include "Core/PresetBuilder.h"
#include "Core/PresetCalib.h"

namespace sk { namespace studio
{
struct StZone { std::string file; int root = 60, lo = 0, hi = 127, loVel = 1, hiVel = 127, rr = 0; };
struct Inst { std::string slug, name, cat; bool fixed = false; std::vector<StZone> zones; };

enum class ImportMode { OneShots, MultiSample, Loops };

inline std::string slugify (const juce::String& s)
{
    juce::String t = s.toLowerCase().retainCharacters ("abcdefghijklmnopqrstuvwxyz0123456789 -_");
    t = t.trim().replaceCharacters (" _", "--"); while (t.contains ("--")) t = t.replace ("--", "-");
    t = t.trimCharactersAtStart ("-").trimCharactersAtEnd ("-");
    return t.isEmpty() ? std::string ("sem-nome") : t.toStdString();
}

inline bool has (const juce::String& s, std::initializer_list<const char*> keys) { for (auto* k : keys) if (s.contains (k)) return true; return false; }

inline std::string guessCategory (const juce::String& stem, bool fixed)
{
    const auto s = stem.toLowerCase();
    if (has (s, { "loop" })) return "Loops";
    if (has (s, { "kick", "bombo", "snare", "caixa", "clap", "palma", "rim", "tom", "hat", "hihat", "hh" })) return "Bateria";
    if (has (s, { "shaker", "conga", "bongo", "perc", "guiro", "tamb", "cowbell", "clave" })) return "Percussão";
    if (has (s, { "808", "bass", "baixo", "sub" })) return "Baixo";
    if (has (s, { "pad", "atmos", "ambient" })) return "Pads";
    if (has (s, { "vox", "voice", "voz", "vocal", "chop", "choir", "coro" })) return "Vozes";
    if (has (s, { "bell", "mallet", "kalimba", "marimba", "vibra" })) return "Mallets";
    if (has (s, { "piano", "keys", "rhodes", "wurli", "ep ", "organ", "orgao" })) return "Teclas";
    if (has (s, { "fx", "riser", "impact", "sweep", "noise" })) return "FX";
    if (has (s, { "lead", "synth", "stab", "arp" })) return "Sintetizador";
    return fixed ? "Percussão" : "Teclas";
}

// nota de bateria (mapa do kit) a partir do nome; -1 se não for reconhecido
inline int drumKeyFromName (const juce::String& stem)
{
    const auto s = stem.toLowerCase();
    if (has (s, { "ohat", "open", "aberto", "ophh" })) return 46;
    if (has (s, { "hat", "hihat", "hh" })) return 42;
    if (has (s, { "kick", "bombo", "bd" })) return 36;
    if (has (s, { "rim", "side" })) return 37;
    if (has (s, { "snare", "caixa", "snr" })) return 38;
    if (has (s, { "clap", "palma" })) return 39;
    if (has (s, { "tom" })) return 41;
    if (has (s, { "conga" })) return has (s, { "low", "lo", "grave" }) ? 43 : 45;
    if (has (s, { "shaker", "guiro" })) return 44;
    if (has (s, { "bongo" })) return 47;
    return -1;
}

struct Project
{
    std::string id = "meu-pack", name = "Meu Pack", style = "trap", accent = "#A58BFF", desc, version = "1.0.0", coverFile;
    bool autoKit = true, autoPresets = true;
    std::vector<Inst> insts;
    std::vector<Groove> grooves;
    std::vector<std::string> extraPresets;            // nomes de presets do utilizador a incluir

    static juce::File root() { auto d = skpaths::userDir().getChildFile ("Estudio"); d.createDirectory(); return d; }
    static juce::File exportDir() { auto d = root().getChildFile ("Exportados"); d.createDirectory(); return d; }
    juce::File dir() const { auto d = root().getChildFile (juce::String (id)); d.createDirectory(); d.getChildFile ("samples").createDirectory(); return d; }
    juce::File sampleFile (const std::string& f) const { return dir().getChildFile ("samples").getChildFile (juce::String (f)); }

    // ---------------------------------------------------------------- guardar / carregar
    std::string toJson() const
    {
        json::Value o = json::Value::makeObject();
        o.set ("id", json::Value::makeString (id)); o.set ("name", json::Value::makeString (name)); o.set ("style", json::Value::makeString (style));
        o.set ("accent", json::Value::makeString (accent)); o.set ("desc", json::Value::makeString (desc)); o.set ("version", json::Value::makeString (version));
        o.set ("cover", json::Value::makeString (coverFile)); o.set ("autoKit", json::Value::makeNumber (autoKit)); o.set ("autoPresets", json::Value::makeNumber (autoPresets));
        json::Value ji = json::Value::makeArray();
        for (auto& in : insts)
        {
            json::Value x = json::Value::makeObject(); x.set ("slug", json::Value::makeString (in.slug)); x.set ("name", json::Value::makeString (in.name));
            x.set ("cat", json::Value::makeString (in.cat)); x.set ("fixed", json::Value::makeNumber (in.fixed));
            json::Value zs = json::Value::makeArray();
            for (auto& z : in.zones)
            {
                json::Value jz = json::Value::makeObject(); jz.set ("file", json::Value::makeString (z.file));
                jz.set ("root", json::Value::makeNumber (z.root)); jz.set ("lo", json::Value::makeNumber (z.lo)); jz.set ("hi", json::Value::makeNumber (z.hi));
                jz.set ("loVel", json::Value::makeNumber (z.loVel)); jz.set ("hiVel", json::Value::makeNumber (z.hiVel)); jz.set ("rr", json::Value::makeNumber (z.rr));
                zs.arr.push_back (jz);
            }
            x.set ("zones", zs); ji.arr.push_back (x);
        }
        o.set ("instruments", ji);
        json::Value gs = json::Value::makeArray();
        for (auto& g : grooves) { json::Value v; if (json::parse (groovejson::toJson (g), v)) gs.arr.push_back (v); }
        o.set ("grooves", gs);
        json::Value ep = json::Value::makeArray(); for (auto& n : extraPresets) ep.arr.push_back (json::Value::makeString (n)); o.set ("presets", ep);
        return json::stringify (o, true);
    }
    bool fromJson (const std::string& text)
    {
        json::Value v; if (! json::parse (text, v) || ! v.isObject()) return false;
        Project p; p.id = v.getString ("id"); p.name = v.getString ("name"); p.style = v.getString ("style", "trap"); p.accent = v.getString ("accent", "#A58BFF");
        p.desc = v.getString ("desc"); p.version = v.getString ("version", "1.0.0"); p.coverFile = v.getString ("cover");
        p.autoKit = v.getBool ("autoKit", true); p.autoPresets = v.getBool ("autoPresets", true);
        if (auto* is = v.find ("instruments")) for (auto& x : is->arr)
        {
            Inst in; in.slug = x.getString ("slug"); in.name = x.getString ("name"); in.cat = x.getString ("cat"); in.fixed = x.getBool ("fixed");
            if (auto* zs = x.find ("zones")) for (auto& jz : zs->arr)
            { StZone z; z.file = jz.getString ("file"); z.root = (int) jz.getNumber ("root", 60); z.lo = (int) jz.getNumber ("lo", 0); z.hi = (int) jz.getNumber ("hi", 127);
              z.loVel = (int) jz.getNumber ("loVel", 1); z.hiVel = (int) jz.getNumber ("hiVel", 127); z.rr = (int) jz.getNumber ("rr", 0); in.zones.push_back (z); }
            p.insts.push_back (std::move (in));
        }
        if (auto* gs = v.find ("grooves")) for (auto& x : gs->arr) { Groove g; if (groovejson::fromJson (json::stringify (x, false), g)) p.grooves.push_back (std::move (g)); }
        if (auto* ep = v.find ("presets")) for (auto& x : ep->arr) if (x.isString()) p.extraPresets.push_back (x.str);
        if (p.id.empty()) return false; *this = std::move (p); return true;
    }
    bool save() const { return dir().getChildFile ("projeto.json").replaceWithText (juce::String::fromUTF8 (toJson().c_str())); }
    static std::vector<std::string> listProjects()
    {
        std::vector<std::string> v;
        for (auto& d : root().findChildFiles (juce::File::findDirectories, false)) if (d.getChildFile ("projeto.json").existsAsFile() && d.getFileName() != "Exportados") v.push_back (d.getFileName().toStdString());
        std::sort (v.begin(), v.end()); return v;
    }
    static bool load (const std::string& id, Project& out)
    {
        const auto f = root().getChildFile (juce::String (id)).getChildFile ("projeto.json");
        Project p; return f.existsAsFile() && p.fromJson (f.loadFileAsString().toStdString()) && (out = std::move (p), true);
    }

    // ---------------------------------------------------------------- importar áudio
    juce::String uniqueSlug (const std::string& base) const
    {
        std::string s = base; int n = 2; auto taken = [&] (const std::string& x) { for (auto& i : insts) if (i.slug == x) return true; return x == "kit"; };
        while (taken (s)) s = base + "-" + std::to_string (n++);
        return juce::String (s);
    }
    juce::File copyIn (const juce::File& src) const
    {
        auto dst = dir().getChildFile ("samples").getChildFile (src.getFileName());
        if (dst != src) { if (dst.existsAsFile()) dst = dst.getNonexistentSibling(); src.copyFileTo (dst); }
        return dst;
    }
    struct Analysis { juce::File file; int root = 60; bool fixed = false; float peak = 0; juce::String err; };
    static Analysis analyse (const juce::File& f)
    {
        Analysis a; a.file = f; sk::SampleData sd; a.err = useraudio::decode (f, sd); if (a.err.isNotEmpty()) return a;
        for (float v : sd.l) a.peak = std::max (a.peak, std::fabs (v)); for (float v : sd.r) a.peak = std::max (a.peak, std::fabs (v));
        if (a.peak < 1.0e-5f) { a.err = juce::String (juce::CharPointer_UTF8 ("Ficheiro em sil\xC3\xAAncio: ")) + f.getFileName(); return a; }
        int root = useraudio::detectPitch (useraudio::toMono (sd), sd.sampleRate);
        if (root < 0) root = useraudio::noteFromName (f.getFileNameWithoutExtension());
        if (root < 0) { a.fixed = true; root = 60; } a.root = root; return a;
    }
    // devolve mensagens de aviso (ficheiros ignorados); 'groupName' só serve em MultiSample
    juce::StringArray addAudio (const juce::Array<juce::File>& files, ImportMode mode, const juce::String& groupName, const juce::String& catOverride = {})
    {
        juce::StringArray warn; std::vector<Analysis> an;
        for (auto& f : files) { if (! useraudio::isAudioFile (f)) { warn.add (f.getFileName() + juce::String (juce::CharPointer_UTF8 (": formato n\xC3\xA3o suportado"))); continue; }
                                auto a = analyse (f); if (a.err.isNotEmpty()) warn.add (f.getFileName() + ": " + a.err); else an.push_back (a); }
        if (an.empty()) return warn;
        auto mk = [&] (const Analysis& a, bool loop)
        {
            const auto dst = copyIn (a.file); Inst in; in.name = a.file.getFileNameWithoutExtension().toStdString();
            in.slug = uniqueSlug (slugify (a.file.getFileNameWithoutExtension())).toStdString();
            in.fixed = loop || a.fixed || drumKeyFromName (a.file.getFileNameWithoutExtension()) >= 0;      // bombo/caixa/hat… não acompanham a tecla
            in.cat = catOverride.isNotEmpty() ? catOverride.toStdString() : (loop ? "Loops" : guessCategory (a.file.getFileNameWithoutExtension(), a.fixed));
            StZone z; z.file = dst.getFileName().toStdString(); z.root = a.root; in.zones.push_back (z); return in;
        };
        if (mode == ImportMode::MultiSample)
        {
            std::sort (an.begin(), an.end(), [] (const Analysis& x, const Analysis& y) { return x.root < y.root; });
            Inst in; in.name = groupName.isEmpty() ? "Instrumento" : groupName.toStdString(); in.slug = uniqueSlug (slugify (groupName.isEmpty() ? juce::String ("instrumento") : groupName)).toStdString();
            bool allFixed = true; for (auto& a : an) allFixed = allFixed && a.fixed; in.fixed = allFixed;
            in.cat = catOverride.isNotEmpty() ? catOverride.toStdString() : guessCategory (groupName, allFixed);
            for (size_t i = 0; i < an.size(); ++i)
            {
                StZone z; z.file = copyIn (an[i].file).getFileName().toStdString(); z.root = an[i].root;
                if (allFixed) { z.lo = 0; z.hi = 127; z.rr = 0; }       // todos tocam em qualquer tecla (alternam)
                else
                {
                    z.lo = i == 0 ? 0 : (an[i - 1].root + an[i].root) / 2 + 1; z.hi = i + 1 == an.size() ? 127 : (an[i].root + an[i + 1].root) / 2;
                    if (i + 1 < an.size() && an[i + 1].root == an[i].root) z.hi = z.lo;                 // notas repetidas: ocupam só a sua tecla (o plugin alterna)
                }
                in.zones.push_back (z);
            }
            insts.push_back (std::move (in));
        }
        else for (auto& a : an) insts.push_back (mk (a, mode == ImportMode::Loops));
        save(); return warn;
    }

    // ---------------------------------------------------------------- construção do pack
    bool idValid() const { return id.size() >= 3 && id.size() <= 32 && juce::String (id).containsOnly ("abcdefghijklmnopqrstuvwxyz0123456789-"); }
    bool idReserved() const { for (auto& d : buildPackDefs()) if (d.id == id) return true; return id == "user" || id == "nucleo"; }

    bool buildLibrary (Library& lib, juce::String& err) const
    {
        std::map<std::string, int> dataOf; Library local;
        auto dataFor = [&] (const std::string& file, float& gain) -> int
        {
            auto it = dataOf.find (file);
            if (it == dataOf.end())
            {
                auto sd = std::make_shared<SampleData>(); const auto e = useraudio::decode (sampleFile (file), *sd); if (e.isNotEmpty()) { err = e + " (" + juce::String (file) + ")"; return -1; }
                float pk = 0; for (float v : sd->l) pk = std::max (pk, std::fabs (v)); for (float v : sd->r) pk = std::max (pk, std::fabs (v));
                gainOf[file] = std::min (4.0f, 0.9f / std::max (pk, 1.0e-4f)); it = dataOf.emplace (file, local.addData (sd)).first;
            }
            gain = gainOf[file]; return it->second;
        };
        for (auto& in : insts)
        {
            Instrument x; x.id = id + "." + in.slug; x.name = in.name; x.cat = in.cat; x.pack = id; x.fixedPitch = in.fixed;
            for (auto& z : in.zones)
            {
                sk::Zone q; float g = 1; q.data = dataFor (z.file, g); if (q.data < 0) return false;
                q.root = z.root; q.loKey = z.lo; q.hiKey = z.hi; q.loVel = z.loVel; q.hiVel = z.hiVel; q.rr = z.rr; q.gain = g; q.file = z.file; x.zones.push_back (q);
            }
            local.inst.push_back (std::move (x));
        }
        if (autoKit)
        {
            Instrument kit; kit.id = id + ".kit"; kit.name = "Kit de Bateria"; kit.cat = "Bateria"; kit.pack = id; kit.fixedPitch = true; std::set<int> used;
            for (auto& in : insts)
            {
                if (! in.fixed || in.zones.empty() || in.cat == "Loops" || (in.cat != "Bateria" && in.cat != "Percuss\xC3\xA3o")) continue;
                const int key = drumKeyFromName (juce::String (in.name)); if (key < 0 || used.count (key)) continue;
                used.insert (key); float g = 1; sk::Zone q; q.data = dataFor (in.zones[0].file, g); if (q.data < 0) return false;
                q.root = key; q.loKey = q.hiKey = key; q.gain = g; q.file = in.zones[0].file; kit.zones.push_back (q);
            }
            if (kit.zones.size() >= 2) local.inst.insert (local.inst.begin(), std::move (kit));
        }
        lib.absorb (std::move (local)); return true;
    }

    std::vector<Preset> buildPresets (const Library& lib) const
    {
        std::vector<Preset> out; std::set<std::string> names;
        auto add = [&] (Preset p) { std::string n = p.name; int k = 2; while (names.count (n)) n = p.name + " " + std::to_string (k++); p.name = n; names.insert (n); out.push_back (std::move (p)); };
        if (autoPresets)
        {
            for (auto& in : lib.inst)
            {
                const bool kit = in.id == id + ".kit"; std::string kv, tags, nm = kit ? "Kit " + name : in.name, cat = in.cat;
                const bool loop = in.cat == "Loops", pad = in.cat == "Pads";
                if (kit || in.fixedPitch) kv = std::string (" a_on=1 a_type=2 amp_a=1 amp_d=800 amp_s=1 amp_r=") + (loop ? "200" : "120") + " a_loop=0 a_rr=1";
                else if (pad) kv = " a_on=1 a_type=2 amp_a=500 amp_d=800 amp_s=0.9 amp_r=1500";
                else kv = " a_on=1 a_type=2 amp_a=8 amp_d=900 amp_s=0.8 amp_r=350";
                tags = kit ? "bateria,kit" : (loop ? "loop" : (in.fixedPitch ? "one-shot" : "sample"));
                presets::Row r { nm.c_str(), kit ? "Bateria" : cat.c_str(), tags.c_str(), id.c_str(), in.id.c_str(), "", kv.c_str() };
                Preset p = presets::make (r, style.c_str()); p.author = "Piradex"; presets::calibrate (p, lib); add (std::move (p));
            }
        }
        for (auto& n : extraPresets)
        {
            for (auto& u : PackHub::get().userPresets())
                if (u.name == n) { Preset p = u; p.pack = id; p.style = style; p.author = "Piradex"; add (std::move (p)); break; }
        }
        return out;
    }

    static std::string coverJpeg (const juce::File& f)
    {
        if (! f.existsAsFile()) return {};
        auto img = juce::ImageFileFormat::loadFrom (f); if (! img.isValid()) return {};
        const int w = std::min (640, img.getWidth()); const int h = (int) std::lround ((double) img.getHeight() * w / img.getWidth());
        if (w != img.getWidth()) img = img.rescaled (w, std::max (1, h), juce::Graphics::highResamplingQuality);
        juce::MemoryOutputStream mo; juce::JPEGImageFormat jf; jf.setQuality (0.88f); if (! jf.writeImageToStream (img, mo)) return {};
        return std::string ((const char*) mo.getData(), mo.getDataSize());
    }

    // gera o pack; devolve "" ou o erro. 'summary' leva o resumo (instrumentos, amostras, presets, grooves, MB)
    juce::String exportPack (juce::File& outFile, juce::String& summary) const
    {
        if (! idValid()) return juce::CharPointer_UTF8 ("O id do pack deve ter 3-32 caracteres: letras min\xC3\xBAsculas, n\xC3\xBA" "meros e h\xC3\xAD" "fenes.");
        if (idReserved()) return juce::CharPointer_UTF8 ("Este id est\xC3\xA1 reservado (packs de f\xC3\xA1" "brica). Escolhe outro.");
        if (name.empty()) return juce::CharPointer_UTF8 ("D\xC3\xA1 um nome ao pack.");
        if (insts.empty() && grooves.empty()) return juce::CharPointer_UTF8 ("O pack est\xC3\xA1 vazio: adiciona samples ou grooves.");
        Library lib; juce::String err; if (! buildLibrary (lib, err)) return err;
        auto pres = buildPresets (lib);
        PackDef def; def.id = id; def.name = name; def.style = style; def.desc = desc; def.version = version; def.accent = accent;
        std::vector<Groove> gr = grooves; for (auto& g : gr) { g.style = style; g.pack = id; g.user = false; g.file.clear(); }
        const auto bytes = packio::buildPack (def, lib, pres, coverJpeg (dir().getChildFile (juce::String (coverFile))), AccessManager::secret(), gr);
        outFile = exportDir().getChildFile ("SYNTHKEY_Pack_" + juce::String (id) + "_" + juce::String (version) + ".bfpack");
        outFile.deleteFile(); if (! outFile.replaceWithData (bytes.data(), bytes.size())) return juce::CharPointer_UTF8 ("N\xC3\xA3o foi poss\xC3\xADvel gravar o ficheiro do pack.");
        summary = juce::String ((int) lib.inst.size()) + " instrumentos  \xC2\xB7  " + juce::String ((int) lib.sampleCount()) + " samples  \xC2\xB7  " + juce::String ((int) pres.size()) + " presets  \xC2\xB7  "
                  + juce::String ((int) gr.size()) + " grooves  \xC2\xB7  " + juce::String ((double) bytes.size() / 1048576.0, 1) + " MB";
        return {};
    }
private:
    mutable std::map<std::string, float> gainOf;
};
}}
