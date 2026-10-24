#include "PackHub.h"
#include "Access.h"
#include "AppPaths.h"
#include "BinaryData.h"
#include "Core/NucleoGains.h"
#include "UserAudio.h"

using namespace sk;

PackHub& PackHub::get() { static PackHub h; return h; }

PackHub::PackHub()
{
    defs = buildPackDefs();
    pool = std::make_unique<juce::ThreadPool> (1);
    rescan();
    pool->addJob ([this] { reloadUserSamples(); });
}
PackHub::~PackHub() { pool->removeAllJobs (true, 8000); }

juce::Image PackHub::coverFor (const std::string& id)
{
    static std::map<std::string, juce::Image> cache; static std::mutex m;
    std::lock_guard<std::mutex> g (m);
    auto it = cache.find (id); if (it != cache.end()) return it->second;
    const juce::String want = juce::String (id) + "_jpg"; juce::Image img;
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
        if (want == BinaryData::namedResourceList[i]) { int sz = 0; const char* d = BinaryData::getNamedResource (BinaryData::namedResourceList[i], sz); img = juce::ImageFileFormat::loadFrom (d, (size_t) sz); break; }
    cache[id] = img; return img;
}

juce::File PackHub::userPresetFolder() { return skpaths::userPresets(); }

void PackHub::rescan()
{
    std::vector<PackEntry> next; std::map<std::string, std::vector<Preset>> pres; std::map<std::string, std::vector<Groove>> grv;
    // ficheiros instalados
    std::map<std::string, std::pair<juce::File, packio::PackInfo>> found;
    for (auto dir : { skpaths::sharedPacks(), skpaths::userPacks() })
    {
        if (! dir.isDirectory()) continue;
        for (auto& f : dir.findChildFiles (juce::File::findFiles, false, "*.bfpack"))
        {
            zip::Reader zr; packio::PackInfo info;
            if (! zr.openFile (f.getFullPathName().toStdString()) || ! packio::readInfo (zr, AccessManager::secret(), info)) continue;
            auto it = found.find (info.id);
            if (it == found.end() || info.version >= it->second.second.version) { info.path = f.getFullPathName().toStdString(); found[info.id] = { f, info }; }
        }
    }
    for (auto& d : defs)
    {
        PackEntry e; e.id = d.id; e.name = d.name; e.style = d.style; e.desc = d.desc; e.accent = d.accent; e.version = d.version; e.core = d.core;
        for (auto& s : d.insts) e.insts.push_back ({ d.id + "." + s.id, s.name, s.cat });
        if (! d.kit.empty()) e.insts.insert (e.insts.begin(), { d.id + ".kit", "Kit de Bateria", "Bateria" });
        e.cover = coverFor (d.id);
        if (d.core)
        {
            e.installed = true; auto pl = presets::forPack ("nucleo");
            for (auto& p : pl) for (auto& g : presets::kNucleoGains) if (p.name == g.first) p.set (P_MASTER_VOL, g.second);
            e.presetCount = (int) pl.size(); pres[d.id] = std::move (pl);
        }
        else if (auto it = found.find (d.id); it != found.end())
        {
            e.installed = true; e.file = it->second.first; e.version = it->second.second.version;
            zip::Reader zr; std::vector<Preset> pl;
            if (zr.openFile (e.file.getFullPathName().toStdString())) packio::readPresets (zr, pl);
            e.presetCount = (int) pl.size(); pres[d.id] = std::move (pl);
            std::vector<Groove> gl; if (zr.openFile (e.file.getFullPathName().toStdString())) packio::readGrooves (zr, gl);
            for (auto& g : gl) g.pack = d.id;
            e.grooveCount = (int) gl.size(); grv[d.id] = std::move (gl);
        }
        next.push_back (std::move (e));
    }
    // packs criados no Estúdio (id que não existe nas definições de fábrica)
    for (auto& kv : found)
    {
        bool known = false; for (auto& d : defs) if (d.id == kv.first) known = true; if (known) continue;
        const auto& info = kv.second.second; PackEntry e; e.id = info.id; e.name = info.name; e.style = info.style; e.desc = info.desc; e.accent = info.accent; e.version = info.version;
        e.custom = true; e.installed = true; e.file = kv.second.first; for (auto& i : info.insts) e.insts.push_back (i);
        zip::Reader zr; std::vector<Preset> pl; std::vector<Groove> gl; std::string jpg;
        if (zr.openFile (e.file.getFullPathName().toStdString()))
        {
            packio::readPresets (zr, pl); packio::readGrooves (zr, gl);
            if (packio::readCover (zr, jpg)) e.cover = juce::ImageFileFormat::loadFrom (jpg.data(), jpg.size());
        }
        for (auto& g : gl) g.pack = info.id;
        e.presetCount = (int) pl.size(); e.grooveCount = (int) gl.size(); pres[info.id] = std::move (pl); grv[info.id] = std::move (gl);
        next.push_back (std::move (e));
    }
    { std::lock_guard<std::mutex> g (mtx); list = std::move (next); presetsOf = std::move (pres); groovesOf = std::move (grv); }
    ++rev; sendChangeMessage();
}

std::vector<PackEntry> PackHub::entries() const { std::lock_guard<std::mutex> g (mtx); return list; }
bool PackHub::entry (const std::string& id, PackEntry& out) const { std::lock_guard<std::mutex> g (mtx); for (auto& e : list) if (e.id == id) { out = e; return true; } return false; }

std::vector<Preset> PackHub::factoryPresets() const
{
    std::lock_guard<std::mutex> g (mtx); std::vector<Preset> all;
    for (auto& e : list) { auto it = presetsOf.find (e.id); if (it != presetsOf.end()) all.insert (all.end(), it->second.begin(), it->second.end()); }
    return all;
}

std::vector<Groove> PackHub::packGrooves() const
{
    std::lock_guard<std::mutex> g (mtx); std::vector<Groove> all;
    for (auto& e : list) { auto it = groovesOf.find (e.id); if (it != groovesOf.end()) all.insert (all.end(), it->second.begin(), it->second.end()); }
    return all;
}

std::vector<Preset> PackHub::userPresets() const
{
    std::vector<Preset> v;
    for (auto& f : userPresetFolder().findChildFiles (juce::File::findFiles, false, "*.skpreset"))
    { Preset p; if (presetFromJson (f.loadFileAsString().toStdString(), p)) { p.pack = "user"; p.style = "user"; v.push_back (std::move (p)); } }
    return v;
}

std::shared_ptr<const Library> PackHub::library (const std::string& id) const { std::lock_guard<std::mutex> g (mtx); auto it = libs.find (id); return it == libs.end() ? nullptr : it->second; }
bool PackHub::isLoading (const std::string& id) const { std::lock_guard<std::mutex> g (mtx); return loading.count (id) > 0; }
bool PackHub::isLoadFailed (const std::string& id) const { std::lock_guard<std::mutex> g (mtx); return failed.count (id) > 0; }

void PackHub::ensureLoaded (const std::string& id)
{
    if (id.empty()) return;
    { std::lock_guard<std::mutex> g (mtx); if (libs.count (id) || loading.count (id)) return; loading.insert (id); failed.erase (id); }
    pool->addJob ([this, id] { loadJob (id); });
}

void PackHub::loadJob (std::string id)
{
    auto lib = std::make_shared<Library>(); bool ok = false;
    const PackDef* def = nullptr; for (auto& d : defs) if (d.id == id) def = &d;
    if (def && def->core) { buildPackLibrary (*def, *lib); ok = true; }
    else
    {
        PackEntry e; if (entry (id, e) && e.installed)
        {
            zip::Reader zr; std::string err;
            ok = zr.openFile (e.file.getFullPathName().toStdString()) && packio::loadLibrary (zr, AccessManager::secret(), *lib, &err);
        }
    }
    { std::lock_guard<std::mutex> g (mtx); loading.erase (id); if (ok) libs[id] = lib; else failed.insert (id); }
    sendChangeMessage();
}

juce::String PackHub::install (const juce::File& src)
{
    zip::Reader zr; packio::PackInfo info;
    if (! zr.openFile (src.getFullPathName().toStdString())) return juce::CharPointer_UTF8 ("Ficheiro inv\xC3\xA1lido ou corrompido.");
    if (! packio::readInfo (zr, AccessManager::secret(), info)) return juce::CharPointer_UTF8 ("Este ficheiro n\xC3\xA3o \xC3\xA9 um pack SYNTHKEY reconhecido (ou foi alterado).");
    const auto dir = skpaths::userPacks();
    for (auto& f : dir.findChildFiles (juce::File::findFiles, false, "*.bfpack"))   // remove versões anteriores do mesmo pack
    { zip::Reader z2; packio::PackInfo i2; if (z2.openFile (f.getFullPathName().toStdString()) && packio::readInfo (z2, AccessManager::secret(), i2) && i2.id == info.id) f.deleteFile(); }
    const auto dst = dir.getChildFile ("SYNTHKEY_Pack_" + juce::String (info.id) + "_" + juce::String (info.version) + ".bfpack");
    if (! src.copyFileTo (dst)) return juce::CharPointer_UTF8 ("N\xC3\xA3o foi poss\xC3\xADvel copiar o pack para a pasta de packs.");
    { std::lock_guard<std::mutex> g (mtx); libs.erase (info.id); failed.erase (info.id); }
    rescan(); return {};
}

juce::String PackHub::uninstall (const std::string& id)
{
    PackEntry e; if (! entry (id, e) || e.core || ! e.installed) return {};
    if (! e.file.deleteFile()) return juce::CharPointer_UTF8 ("N\xC3\xA3o foi poss\xC3\xADvel remover o ficheiro (pode estar em /Library: remova-o manualmente).");
    { std::lock_guard<std::mutex> g (mtx); libs.erase (id); }
    rescan(); return {};
}

// ------------------------------------------------------------------------------------------------
//  Samples do utilizador
// ------------------------------------------------------------------------------------------------
std::shared_ptr<const Library> PackHub::userLibrary() const { std::lock_guard<std::mutex> g (mtx); return userLib; }

std::vector<packio::PackInfo::Inst> PackHub::userInstruments() const
{
    std::vector<packio::PackInfo::Inst> v; std::lock_guard<std::mutex> g (mtx);
    if (userLib) for (auto& i : userLib->inst) v.push_back ({ i.id, i.name, i.fixedPitch ? "Percuss\xC3\xA3o" : "Tonal" });
    return v;
}

void PackHub::reloadUserSamples()
{
    auto lib = std::make_shared<Library>(); std::map<std::string, UserCacheItem> next;
    std::map<std::string, UserCacheItem> old; { std::lock_guard<std::mutex> g (mtx); old = userCache; }
    auto files = skpaths::userSamples().findChildFiles (juce::File::findFiles, false, "*");
    files.sort();
    for (auto& f : files)
    {
        if (! useraudio::isAudioFile (f)) continue;
        const auto key = f.getFullPathName().toStdString(); const auto mod = f.getLastModificationTime().toMilliseconds(); const auto size = f.getSize();
        UserCacheItem item;
        if (auto it = old.find (key); it != old.end() && it->second.mod == mod && it->second.size == size) item = it->second;
        else { juce::String err; item.mod = mod; item.size = size; if (! useraudio::makeInstrument (f, item.lib, err)) continue; }
        // ids únicos
        for (auto& in : item.lib.inst) { std::string base = in.id; int n = 2; while (lib->indexOf (in.id) >= 0) in.id = base + "_" + std::to_string (n++); }
        lib->appendShared (item.lib); next[key] = std::move (item);
    }
    { std::lock_guard<std::mutex> g (mtx); userLib = lib; userCache = std::move(next); }
    sendChangeMessage();
}

juce::String PackHub::importUserSample (const juce::File& src, std::string* newId)
{
    if (! useraudio::isAudioFile (src)) return juce::CharPointer_UTF8 ("Escolhe um ficheiro WAV, AIFF ou FLAC.");
    { sk::Library tmp; juce::String err; if (! useraudio::makeInstrument (src, tmp, err)) return err; }       // valida antes de copiar
    auto dst = skpaths::userSamples().getChildFile (src.getFileName());
    if (dst != src) { if (dst.existsAsFile()) dst = dst.getNonexistentSibling(); if (! src.copyFileTo (dst)) return juce::CharPointer_UTF8 ("N\xC3\xA3o foi poss\xC3\xADvel copiar o ficheiro para a pasta de samples."); }
    reloadUserSamples();
    if (newId) { *newId = {}; if (auto l = userLibrary()) for (auto& i : l->inst) if (i.zones.size() && i.zones[0].file == dst.getFileName().toStdString()) *newId = i.id; }
    return {};
}

juce::String PackHub::removeUserSample (const std::string& instId)
{
    std::string fn; if (auto l = userLibrary()) for (auto& i : l->inst) if (i.id == instId && ! i.zones.empty()) fn = i.zones[0].file;
    if (fn.empty()) return {};
    if (! skpaths::userSamples().getChildFile (fn).deleteFile()) return juce::CharPointer_UTF8 ("N\xC3\xA3o foi poss\xC3\xADvel apagar o ficheiro.");
    reloadUserSamples(); return {};
}
