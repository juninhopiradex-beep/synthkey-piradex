// =============================================================================
//  SYNTHKEY by Piradex — formato .bfpack (escrita e leitura), C++17 puro
//
//  .bfpack = ZIP "stored" com:
//    manifest.json      metadados, instrumentos/zonas, SHA-256 de cada WAV (em claro), MAC
//    samples/*.wav      áudio 24 bit, CIFRADO (SHA-256-CTR; chave = f(segredo, id do pack))
//    presets/*.skpreset presets do estilo (JSON em claro)
//    cover.jpg          capa
//    LICENSE.txt
// =============================================================================
#pragma once
#include "GrooveJson.h"
#include "Licensing.h"
#include "PackSpecs.h"
#include "Preset.h"
#include "Zip.h"
#include <map>

namespace sk { namespace packio
{
struct PackInfo
{
    std::string id, name, style, version, accent, desc;
    struct Inst { std::string id, name, cat; };
    std::vector<Inst> insts;
    int instruments = 0, samples = 0, presets = 0;
    std::vector<std::string> presetFiles;
    std::string path;     // ficheiro no disco (preenchido pelo gestor)
};

inline std::string manifestMac (const std::string& secret, const std::string& id, const std::string& version, const std::vector<std::pair<std::string, std::string>>& files)
{
    std::string m = "synthkey-manifest|" + id + "|" + version;
    for (auto& f : files) m += "|" + f.first + ":" + f.second;
    return crypto::toHex (crypto::hmacSha256 (secret, m));
}

struct PresetFile { std::string name, json; };

// constrói um pack a partir da definição, da biblioteca (já gerada por buildPackLibrary) e dos presets
inline std::string buildPack (const PackDef& def, const Library& lib, const std::vector<Preset>& presets, const std::string& coverJpg, const std::string& secret,
                              const std::vector<Groove>& grooves = {})
{
    zip::Writer zw;
    json::Value man = json::Value::makeObject();
    man.set ("format", json::Value::makeNumber (1));
    man.set ("id", json::Value::makeString (def.id)); man.set ("name", json::Value::makeString (def.name));
    man.set ("style", json::Value::makeString (def.style)); man.set ("version", json::Value::makeString (def.version));
    man.set ("accent", json::Value::makeString (def.accent)); man.set ("desc", json::Value::makeString (def.desc));
    const crypto::Bytes ck = lic::contentKey (secret, def.id);

    std::map<int, std::string> fileOfData;           // índice de dados -> nome do ficheiro
    std::vector<std::pair<std::string, std::string>> fileHashes;
    json::Value insts = json::Value::makeArray();
    for (const auto& in : lib.inst)
    {
        json::Value ji = json::Value::makeObject();
        ji.set ("id", json::Value::makeString (in.id)); ji.set ("name", json::Value::makeString (in.name)); ji.set ("cat", json::Value::makeString (in.cat));
        ji.set ("fixedPitch", json::Value::makeNumber (in.fixedPitch ? 1 : 0));
        json::Value zs = json::Value::makeArray(); int zi = 0;
        for (const auto& z : in.zones)
        {
            auto it = fileOfData.find (z.data);
            if (it == fileOfData.end())
            {
                std::string leaf = in.id; for (auto& ch : leaf) if (ch == '.') ch = '_';
                const std::string name = "samples/" + leaf + "_" + std::to_string (zi) + ".wav";
                auto wav = writeWav24 (*lib.data[(size_t) z.data]); std::string bytes (wav.begin(), wav.end());
                const std::string h = crypto::sha256Hex (bytes);
                crypto::streamXor (bytes, ck, name);
                zw.add (name, bytes); fileHashes.emplace_back (name, h);
                it = fileOfData.emplace (z.data, name).first;
            }
            json::Value jz = json::Value::makeObject();
            jz.set ("file", json::Value::makeString (it->second)); jz.set ("root", json::Value::makeNumber (z.root));
            jz.set ("loKey", json::Value::makeNumber (z.loKey)); jz.set ("hiKey", json::Value::makeNumber (z.hiKey));
            jz.set ("loVel", json::Value::makeNumber (z.loVel)); jz.set ("hiVel", json::Value::makeNumber (z.hiVel));
            jz.set ("loopStart", json::Value::makeNumber (z.loopStart)); jz.set ("loopEnd", json::Value::makeNumber (z.loopEnd));
            jz.set ("gain", json::Value::makeNumber (z.gain)); jz.set ("rr", json::Value::makeNumber (z.rr));
            zs.arr.push_back (jz); ++zi;
        }
        ji.set ("zones", zs); insts.arr.push_back (ji);
    }
    man.set ("instruments", insts);
    json::Value files = json::Value::makeArray();
    for (auto& f : fileHashes) { json::Value jf = json::Value::makeObject(); jf.set ("name", json::Value::makeString (f.first)); jf.set ("sha256", json::Value::makeString (f.second)); files.arr.push_back (jf); }
    man.set ("files", files);
    json::Value pf = json::Value::makeArray();
    for (const auto& p : presets) { std::string leaf = p.name; for (auto& ch : leaf) if (! std::isalnum ((unsigned char) ch)) ch = '_'; pf.arr.push_back (json::Value::makeString ("presets/" + leaf + ".skpreset")); }
    man.set ("presets", pf);
    json::Value gf = json::Value::makeArray();
    for (const auto& g : grooves) { std::string leaf = g.name; for (auto& ch : leaf) if (! std::isalnum ((unsigned char) ch)) ch = '_'; gf.arr.push_back (json::Value::makeString ("grooves/" + leaf + ".skgroove")); }
    man.set ("grooves", gf);
    man.set ("mac", json::Value::makeString (manifestMac (secret, def.id, def.version, fileHashes)));
    zw.add ("manifest.json", json::stringify (man, true));
    for (size_t i = 0; i < presets.size(); ++i) zw.add (pf.arr[i].str, presetToJson (presets[i]));
    for (size_t i = 0; i < grooves.size(); ++i) zw.add (gf.arr[i].str, groovejson::toJson (grooves[i]));
    if (! coverJpg.empty()) zw.add ("cover.jpg", coverJpg);
    zw.add ("LICENSE.txt", "SYNTHKEY by Piradex — pack \"" + def.name + "\" " + def.version + "\nLicenca de utilizador unico para producao musical. Proibida a redistribuicao dos samples isolados.\n(c) Piradex / BeatFreak Studio.\n");
    return zw.finish();
}

// lê só o manifesto (rápido) — para listar packs instalados
inline bool readInfo (const zip::Reader& zr, const std::string& secret, PackInfo& info)
{
    const auto* mi = zr.find ("manifest.json"); if (! mi) return false;
    json::Value m; if (! json::parse (zr.read (*mi), m) || ! m.isObject()) return false;
    PackInfo pi; pi.id = m.getString ("id"); pi.name = m.getString ("name"); pi.style = m.getString ("style"); pi.version = m.getString ("version");
    pi.accent = m.getString ("accent", "#6FE7F5"); pi.desc = m.getString ("desc");
    if (pi.id.empty()) return false;
    std::vector<std::pair<std::string, std::string>> fh;
    if (auto* f = m.find ("files")) for (auto& x : f->arr) { fh.emplace_back (x.getString ("name"), x.getString ("sha256")); }
    if (m.getString ("mac") != manifestMac (secret, pi.id, pi.version, fh)) return false;     // pack adulterado ou de terceiros
    if (auto* in = m.find ("instruments")) { pi.instruments = (int) in->arr.size(); for (auto& ji : in->arr) pi.insts.push_back ({ ji.getString ("id"), ji.getString ("name"), ji.getString ("cat") }); }
    pi.samples = (int) fh.size();
    if (auto* pf = m.find ("presets")) for (auto& x : pf->arr) if (x.isString()) pi.presetFiles.push_back (x.str);
    pi.presets = (int) pi.presetFiles.size();
    info = pi; return true;
}

inline bool readCover (const zip::Reader& zr, std::string& jpg)
{
    const auto* c = zr.find ("cover.jpg"); if (! c) return false; jpg = zr.read (*c); return true;
}

inline bool readPresets (const zip::Reader& zr, std::vector<Preset>& out)
{
    for (auto& it : zr.list())
        if (it.name.rfind ("presets/", 0) == 0)
        {
            Preset p; if (presetFromJson (zr.read (it), p)) out.push_back (std::move (p));
        }
    return true;
}

// grooves incluídos no pack (JSON em claro)
inline bool readGrooves (const zip::Reader& zr, std::vector<Groove>& out)
{
    for (auto& it : zr.list())
        if (it.name.rfind ("grooves/", 0) == 0) { Groove g; if (groovejson::fromJson (zr.read (it), g)) out.push_back (std::move (g)); }
    return true;
}

// carrega os samples (decifra e verifica o SHA-256 de cada um) para uma biblioteca
inline bool loadLibrary (const zip::Reader& zr, const std::string& secret, Library& out, std::string* err = nullptr)
{
    auto fail = [&] (const std::string& m) { if (err) *err = m; return false; };
    const auto* mi = zr.find ("manifest.json"); if (! mi) return fail ("sem manifest.json");
    json::Value m; if (! json::parse (zr.read (*mi), m)) return fail ("manifest inválido");
    const std::string id = m.getString ("id");
    std::vector<std::pair<std::string, std::string>> fh;
    if (auto* f = m.find ("files")) for (auto& x : f->arr) fh.emplace_back (x.getString ("name"), x.getString ("sha256"));
    if (m.getString ("mac") != manifestMac (secret, id, m.getString ("version"), fh)) return fail ("pack não reconhecido");
    const crypto::Bytes ck = lic::contentKey (secret, id);
    Library lib; std::map<std::string, int> dataOf;
    for (auto& f : fh)
    {
        const auto* it = zr.find (f.first); if (! it) return fail ("falta " + f.first);
        std::string b = zr.read (*it); crypto::streamXor (b, ck, f.first);
        if (crypto::sha256Hex (b) != f.second) return fail ("integridade falhou em " + f.first);
        auto sd = std::make_shared<SampleData>();
        if (! readWav ((const uint8_t*) b.data(), b.size(), *sd)) return fail ("WAV inválido " + f.first);
        dataOf[f.first] = lib.addData (sd);
    }
    if (auto* ins = m.find ("instruments"))
        for (auto& ji : ins->arr)
        {
            Instrument in; in.id = ji.getString ("id"); in.name = ji.getString ("name"); in.cat = ji.getString ("cat"); in.pack = id; in.fixedPitch = ji.getBool ("fixedPitch");
            if (auto* zs = ji.find ("zones"))
                for (auto& jz : zs->arr)
                {
                    Zone z; z.file = jz.getString ("file"); auto d = dataOf.find (z.file); if (d == dataOf.end()) continue; z.data = d->second;
                    z.root = (int) jz.getNumber ("root", 60); z.loKey = (int) jz.getNumber ("loKey", 0); z.hiKey = (int) jz.getNumber ("hiKey", 127);
                    z.loVel = (int) jz.getNumber ("loVel", 1); z.hiVel = (int) jz.getNumber ("hiVel", 127);
                    z.loopStart = (int) jz.getNumber ("loopStart", -1); z.loopEnd = (int) jz.getNumber ("loopEnd", -1);
                    z.gain = (float) jz.getNumber ("gain", 1.0); z.rr = (int) jz.getNumber ("rr", 0);
                    in.zones.push_back (z);
                }
            lib.inst.push_back (std::move (in));
        }
    out.absorb (std::move (lib)); return true;
}

// ---- conveniências: memória
inline bool readInfo (const std::string& bytes, const std::string& secret, PackInfo& i) { zip::Reader z; return z.open (&bytes) && readInfo (z, secret, i); }
inline bool readCover (const std::string& bytes, std::string& j) { zip::Reader z; return z.open (&bytes) && readCover (z, j); }
inline bool readPresets (const std::string& bytes, std::vector<Preset>& o) { zip::Reader z; return z.open (&bytes) && readPresets (z, o); }
inline bool loadLibrary (const std::string& bytes, const std::string& secret, Library& o, std::string* e = nullptr)
{ zip::Reader z; if (! z.open (&bytes)) { if (e) *e = "ficheiro .bfpack corrompido"; return false; } return loadLibrary (z, secret, o, e); }
}} // namespace sk::packio
