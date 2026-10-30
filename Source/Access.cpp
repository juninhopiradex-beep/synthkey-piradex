#include "Access.h"
#include "AppPaths.h"
#include "Core/MiniJson.h"
#include "Secrets.h"
#include "Pwtable.h"

using namespace sk;

AccessManager& AccessManager::get() { static AccessManager a; return a; }
std::string AccessManager::secret() { static const std::string s = secrets::macSecret(); return s; }

AccessManager::AccessManager()
{
    juce::PropertiesFile::Options o; o.storageFormat = juce::PropertiesFile::storeAsXML; o.millisecondsBeforeSaving = 0;
    props = std::make_unique<juce::PropertiesFile> (skpaths::settingsFile(), o);

    // guarda do relógio: último dia visto (com MAC)
    const auto ls = props->getValue ("lastSeen").toStdString(); const auto lm = props->getValue ("lastSeenMac").toStdString();
    if (! ls.empty() && lm == crypto::toHex (crypto::hmacSha256 (secret(), "lastseen|" + ls)).substr (0, 16)) lastSeenDay = std::atoi (ls.c_str());

    // ativações guardadas
    json::Value v; if (json::parse (props->getValue ("activations").toStdString(), v) && v.isArray())
        for (auto& x : v.arr) { lic::Activation a; a.pack = x.getString ("pack"); a.key = x.getString ("key"); a.activatedDay = (int) x.getNumber ("day"); a.mac = x.getString ("mac"); acts.push_back (a); }

    // sessão lembrada neste computador
    const auto id = props->getValue ("loginId"); const auto tok = props->getValue ("loginToken");
    if (id.isNotEmpty() && tok.isNotEmpty())
        for (int i = 0; i < pwtab::kPasswordCount; ++i)
            if (id == pwtab::kPasswords[i].id && tok == tokenFor (i) && ! lic::pwExpired (pwtab::kPasswords[i], today())) { setRole (i); refreshAdmin(); break; }
}

int AccessManager::today() const
{
    const juce::ScopedLock sl (lock);
    const int now = lic::todayDays2026();
    if (now > lastSeenDay)
    {
        lastSeenDay = now; const auto s = std::to_string (now);
        props->setValue ("lastSeen", juce::String (s)); props->setValue ("lastSeenMac", juce::String (crypto::toHex (crypto::hmacSha256 (secret(), "lastseen|" + s)).substr (0, 16)));
        props->saveIfNeeded();
    }
    return lic::effectiveToday (now, lastSeenDay);
}

juce::String AccessManager::tokenFor (int index) const
{
    const auto s = std::string (juce::SystemStats::getUniqueDeviceID().toRawUTF8()) + "|" + pwtab::kPasswords[index].id + "|" + pwtab::kPasswords[index].hashHex + "|SYNTHKEY";
    return juce::String (crypto::sha256Hex (s));
}

void AccessManager::setRole (int index)
{
    currentIndex = index; roleValue.store ((int) pwtab::kPasswords[index].role); unlocked.store (true);
}

AccessManager::Match AccessManager::matchPassword (const juce::String& password)
{
    Match m; bool ex = false;
    m.index = lic::matchPassword (pwtab::kPasswords, pwtab::kPasswordCount, password.toStdString(), AccessManager::get().today(), ex);
    m.expired = ex; return m;
}

AccessManager::LoginResult AccessManager::applyLogin (Match m, bool remember)
{
    if (m.index < 0) return m.expired ? LoginResult::Expired : LoginResult::Wrong;
    {
        const juce::ScopedLock sl (lock);
        setRole (m.index);
        if (remember) { props->setValue ("loginId", juce::String (pwtab::kPasswords[m.index].id)); props->setValue ("loginToken", tokenFor (m.index)); }
        else { props->removeValue ("loginId"); props->removeValue ("loginToken"); }
        props->saveIfNeeded();
    }
    refreshAdmin();
    return LoginResult::Ok;
}

void AccessManager::logout()
{
    const juce::ScopedLock sl (lock);
    unlocked.store (false); roleValue.store (0); currentIndex = -1; adminOk.store (false);
    props->removeValue ("loginId"); props->removeValue ("loginToken"); props->saveIfNeeded();
}

void AccessManager::checkExpiry()
{
    if (unlocked.load() && currentIndex >= 0 && lic::pwExpired (pwtab::kPasswords[currentIndex], today())) logout();
}

int AccessManager::betaDaysLeft() const
{
    if (! unlocked.load() || role() != lic::Role::Beta || currentIndex < 0) return -1;
    const auto& e = pwtab::kPasswords[currentIndex];
    return std::max (0, keys::daysSince2026 (e.expYear, e.expMonth, e.expDay) - today() + 1);
}

juce::String AccessManager::roleText() const
{
    if (! unlocked.load()) return {};
    if (role() == lic::Role::Master) return juce::CharPointer_UTF8 ("MESTRE");
    const auto& e = pwtab::kPasswords[std::max (0, currentIndex)];
    return juce::String (juce::CharPointer_UTF8 ("BETA \xC2\xB7 at\xC3\xA9 ")) + juce::String (e.expDay).paddedLeft ('0', 2) + "/" + juce::String (e.expMonth).paddedLeft ('0', 2) + "/" + juce::String (e.expYear);
}

// ----------------------------------------------------------------------------- packs
lic::PackStatus AccessManager::packStatus (const std::string& packId, bool core) const
{
    const juce::ScopedLock sl (lock);
    return lic::packStatus (secret(), packId, core, role(), kBetaOpensAllPacks, today(), acts, simulate.load());
}

lic::ActivateResult AccessManager::activate (const std::string& packId, const juce::String& keyText)
{
    const juce::ScopedLock sl (lock);
    auto r = lic::activate (secret(), packId, keyText.toStdString(), today(), acts);
    if (r == lic::ActivateResult::Ok) saveActivations();
    return r;
}

std::vector<lic::Activation> AccessManager::activations() const { const juce::ScopedLock sl (lock); return acts; }

void AccessManager::saveActivations()
{
    json::Value arr = json::Value::makeArray();
    for (auto& a : acts) { json::Value o = json::Value::makeObject(); o.set ("pack", json::Value::makeString (a.pack)); o.set ("key", json::Value::makeString (a.key)); o.set ("day", json::Value::makeNumber (a.activatedDay)); o.set ("mac", json::Value::makeString (a.mac)); arr.arr.push_back (o); }
    props->setValue ("activations", juce::String (json::stringify (arr, false)));
    props->saveIfNeeded();
}

// ------------------------------------------------------------------------ administrador
void AccessManager::refreshAdmin()
{
    bool ok = false;
    if (unlocked.load() && role() == lic::Role::Master)
    {
        const auto f = skpaths::adminKeyFile();
        if (f.existsAsFile())
        {
            std::string tok = f.loadFileAsString().trim().toStdString();
            const auto expected = crypto::fromHex (secrets::kAdminHashHex);
            const auto got = crypto::pbkdf2Sha256 (tok, crypto::fromHex (secrets::kAdminSaltHex), lic::kPbkdf2Iterations, (int) expected.size());
            ok = crypto::constantTimeEq (got, expected);
        }
    }
    adminOk.store (ok);
}

void AccessManager::loadIssued() const
{
    if (issuedLoaded) return;
    issuedLoaded = true; issuedCache.clear(); nextSerial = 1;
    json::Value v; const auto f = skpaths::issuedFile();
    if (! f.existsAsFile() || ! json::parse (f.loadFileAsString().toStdString(), v)) return;
    nextSerial = (uint32_t) v.getNumber ("nextSerial", 1);
    if (auto* a = v.find ("keys")) for (auto& x : a->arr)
    {
        lic::IssuedKey k; k.pack = x.getString ("pack"); k.key = x.getString ("key"); k.note = x.getString ("note"); k.serial = (uint32_t) x.getNumber ("serial");
        k.createdDay = (int) x.getNumber ("created"); k.validity = (int) x.getNumber ("validity"); k.untilDay = (int) x.getNumber ("until"); k.daysAfter = (int) x.getNumber ("daysAfter"); k.seats = (int) x.getNumber ("seats", 1);
        issuedCache.push_back (k);
    }
}
void AccessManager::saveIssued() const
{
    json::Value v = json::Value::makeObject(); v.set ("nextSerial", json::Value::makeNumber (nextSerial)); json::Value a = json::Value::makeArray();
    for (auto& k : issuedCache)
    {
        json::Value o = json::Value::makeObject(); o.set ("pack", json::Value::makeString (k.pack)); o.set ("key", json::Value::makeString (k.key)); o.set ("note", json::Value::makeString (k.note));
        o.set ("serial", json::Value::makeNumber (k.serial)); o.set ("created", json::Value::makeNumber (k.createdDay)); o.set ("validity", json::Value::makeNumber (k.validity));
        o.set ("until", json::Value::makeNumber (k.untilDay)); o.set ("daysAfter", json::Value::makeNumber (k.daysAfter)); o.set ("seats", json::Value::makeNumber (k.seats)); a.arr.push_back (o);
    }
    v.set ("keys", a); skpaths::issuedFile().replaceWithText (juce::String (json::stringify (v, true)));
}

std::vector<lic::IssuedKey> AccessManager::issuedKeys() const { const juce::ScopedLock sl (lock); loadIssued(); return issuedCache; }

std::vector<lic::IssuedKey> AccessManager::issue (const lic::IssueRequest& r)
{
    if (! isAdmin()) return {};
    const juce::ScopedLock sl (lock); loadIssued();
    auto out = lic::issueKeys (secret(), r, nextSerial, today());
    for (auto& k : out) issuedCache.push_back (k);
    saveIssued(); return out;
}

void AccessManager::removeIssued (const juce::String& key)
{
    if (! isAdmin()) return;
    const juce::ScopedLock sl (lock); loadIssued();
    issuedCache.erase (std::remove_if (issuedCache.begin(), issuedCache.end(), [&] (const lic::IssuedKey& k) { return key == juce::String (k.key); }), issuedCache.end());
    saveIssued();
}

bool AccessManager::isIssuedKeyActivatedHere (const lic::IssuedKey& k) const
{
    const juce::ScopedLock sl (lock);
    for (auto& a : acts) if (a.pack == k.pack && a.key == k.key) return true;
    return false;
}

juce::String AccessManager::issuedCsv (const std::vector<lic::IssuedKey>& keys) const
{
    juce::String s = juce::CharPointer_UTF8 ("pack;chave;validade;computadores;serie;nota;criada\n");
    for (auto& k : keys)
    {
        juce::String val = k.validity == 0 ? juce::String ("permanente") : (k.validity == 1 ? juce::String ("at\xC3\xA9 ") + lic::formatDay (k.untilDay) : juce::String (k.daysAfter) + " dias apos ativacao");
        s << k.pack << ";" << k.key << ";" << val << ";" << k.seats << ";" << (int) k.serial << ";" << juce::String (k.note).replace (";", ",") << ";" << lic::formatDay (k.createdDay) << "\n";
    }
    return s;
}
