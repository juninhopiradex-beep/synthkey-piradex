// =============================================================================
//  SYNTHKEY by Piradex — regras de licenciamento (puras, sem JUCE, testáveis)
//
//   • Senhas de acesso (master / beta com data limite): só salt + hash PBKDF2.
//   • Chaves de packs: ver Keys.h. Aqui ficam as ativações, a validade e o acesso.
//   • Segredo MAC: injetado no build (Secrets.h, nunca no repositório público).
// =============================================================================
#pragma once
#include "Crypto.h"
#include "Keys.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <string>
#include <vector>

namespace sk { namespace lic
{
enum class Role { None = 0, Master, Beta };

struct PwEntry
{
    const char* id;
    Role        role;
    const char* saltHex;
    const char* hashHex;
    int expYear, expMonth, expDay;     // só beta: válida até ao fim deste dia (hora local)
};

constexpr int kPbkdf2Iterations = 60000;

// ---------------------------------------------------------------- datas
inline int todayDays2026()
{
    std::time_t t = std::time (nullptr); std::tm lt {};
   #if defined (_WIN32)
    localtime_s (&lt, &t);
   #else
    localtime_r (&t, &lt);
   #endif
    return keys::daysSince2026 (lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday);
}
inline std::string formatDay (int days2026)
{
    int y, m, d; keys::civilFromDays2026 (days2026, y, m, d);
    char b[24]; std::snprintf (b, sizeof b, "%02d/%02d/%04d", d, m, y); return b;
}
// proteção contra recuo do relógio: usa-se sempre o maior entre "hoje" e o último dia visto
inline int effectiveToday (int nowDays, int lastSeenDays) { return std::max (nowDays, lastSeenDays); }

// ------------------------------------------------------- senhas de acesso
inline bool pwExpired (const PwEntry& e, int today)
{
    if (e.role != Role::Beta) return false;
    return today > keys::daysSince2026 (e.expYear, e.expMonth, e.expDay);
}

// índice da entrada que corresponde à senha; -1 se errada. expired=true se acertou mas já caducou.
inline int matchPassword (const PwEntry* entries, int n, const std::string& passwordRaw, int today, bool& expired)
{
    std::string pw = passwordRaw;
    while (! pw.empty() && std::isspace ((unsigned char) pw.back())) pw.pop_back();
    size_t a = 0; while (a < pw.size() && std::isspace ((unsigned char) pw[a])) ++a; pw = pw.substr (a);
    expired = false; int found = -1;
    for (int i = 0; i < n; ++i)   // percorre todas (tempo constante em relação ao acerto)
    {
        const crypto::Bytes expected = crypto::fromHex (entries[i].hashHex);
        const crypto::Bytes got = crypto::pbkdf2Sha256 (pw, crypto::fromHex (entries[i].saltHex), kPbkdf2Iterations, (int) expected.size());
        if (! crypto::constantTimeEq (got, expected)) continue;
        if (pwExpired (entries[i], today)) { expired = true; continue; }
        if (found < 0) found = i;
    }
    return found;
}

// ------------------------------------------------------------------ packs
inline crypto::Bytes contentKey (const std::string& secret, const std::string& packId)
{
    return crypto::hmacSha256 (secret, "synthkey-content|" + packId);
}

struct Activation
{
    std::string pack, key; int activatedDay = 0; std::string mac;   // mac impede editar o dia de ativação
};
inline std::string activationMac (const std::string& secret, const std::string& pack, const std::string& key, int day)
{
    return crypto::toHex (crypto::hmacSha256 (secret, "synthkey-act|" + pack + "|" + key + "|" + std::to_string (day))).substr (0, 16);
}
inline Activation makeActivation (const std::string& secret, const std::string& pack, const std::string& key, int day)
{ Activation a; a.pack = pack; a.key = key; a.activatedDay = day; a.mac = activationMac (secret, pack, key, day); return a; }

enum class Access { Core, Master, Beta, Key, Locked, Expired };

struct PackStatus
{
    Access access = Access::Locked;
    int daysLeft = -1;       // -1 = sem limite; ≥0 = dias restantes (incl. hoje)
    int lastDay = -1;        // último dia válido (dias desde 2026-01-01), se houver
    bool usable() const { return access == Access::Core || access == Access::Master || access == Access::Beta || access == Access::Key; }
};

// último dia válido de uma chave (−1 = permanente)
inline int keyLastDay (const keys::Payload& p, int activatedDay)
{
    if (p.relative) return activatedDay + p.expiryDay - 1;     // N dias a contar do dia da ativação, inclusive
    if (p.permanent) return -1;
    return p.expiryDay;                                          // válida até ao fim deste dia
}

inline PackStatus packStatus (const std::string& secret, const std::string& packId, bool core, Role role, bool betaOpensAll,
                              int today, const std::vector<Activation>& acts, bool ignoreRole = false)
{
    PackStatus s;
    if (core) { s.access = Access::Core; return s; }
    if (! ignoreRole)
    {
        if (role == Role::Master) { s.access = Access::Master; return s; }
        if (role == Role::Beta && betaOpensAll) { s.access = Access::Beta; return s; }
    }
    bool sawExpired = false;
    for (auto& a : acts)
    {
        if (a.pack != packId) continue;
        if (a.mac != activationMac (secret, a.pack, a.key, a.activatedDay)) continue;
        keys::Payload p; if (! keys::parseKey (secret, packId, a.key, p)) continue;
        const int last = keyLastDay (p, a.activatedDay);
        if (last >= 0 && today > last) { sawExpired = true; continue; }
        s.access = Access::Key; s.lastDay = last; s.daysLeft = last < 0 ? -1 : last - today + 1; return s;
    }
    s.access = sawExpired ? Access::Expired : Access::Locked; return s;
}

enum class ActivateResult { Ok, Invalid, Expired, AlreadyActive };

inline ActivateResult activate (const std::string& secret, const std::string& packId, const std::string& keyText, int today, std::vector<Activation>& acts, std::string* normalized = nullptr)
{
    keys::Payload p; if (! keys::parseKey (secret, packId, keyText, p)) return ActivateResult::Invalid;
    if (! p.relative && ! p.permanent && today > p.expiryDay) return ActivateResult::Expired;
    // chave normalizada (maiúsculas, hífens): reconstruída a partir do payload
    const std::string canon = keys::makeKey (secret, packId, p);
    for (auto& a : acts) if (a.pack == packId && a.key == canon) { if (normalized) *normalized = canon; return ActivateResult::AlreadyActive; }
    acts.push_back (makeActivation (secret, packId, canon, today));
    if (normalized) *normalized = canon;
    return ActivateResult::Ok;
}

// ----------------------------------------------------------- administrador
struct IssueRequest
{
    std::vector<std::string> packs;
    enum Validity { Permanent, UntilDate, DaysAfterActivation } validity = Permanent;
    int untilDay = 0;        // dias desde 2026-01-01 (UntilDate)
    int daysAfter = 30;      // (DaysAfterActivation)
    int seats = 1;           // 1..4
    int quantity = 1;
    std::string note;
};
struct IssuedKey { std::string pack, key, note; uint32_t serial = 0; int createdDay = 0; int validity = 0, untilDay = 0, daysAfter = 0, seats = 1; };

// gera as chaves; "nextSerial" avança (24 bits)
inline std::vector<IssuedKey> issueKeys (const std::string& secret, const IssueRequest& r, uint32_t& nextSerial, int today)
{
    std::vector<IssuedKey> out;
    for (int q = 0; q < std::max (1, r.quantity); ++q)
        for (auto& pk : r.packs)
        {
            keys::Payload p; p.seats = std::max (1, std::min (4, r.seats)); p.serial = (nextSerial++) & 0xFFFFFF;
            if (r.validity == IssueRequest::Permanent) { p.permanent = true; p.relative = false; p.expiryDay = 0; }
            else if (r.validity == IssueRequest::UntilDate) { p.permanent = false; p.relative = false; p.expiryDay = std::max (1, std::min (8191, r.untilDay)); }
            else { p.permanent = false; p.relative = true; p.expiryDay = std::max (1, std::min (8191, r.daysAfter)); }
            IssuedKey k; k.pack = pk; k.key = keys::makeKey (secret, pk, p); k.note = r.note; k.serial = p.serial; k.createdDay = today;
            k.validity = (int) r.validity; k.untilDay = r.untilDay; k.daysAfter = r.daysAfter; k.seats = p.seats; out.push_back (k);
        }
    return out;
}

// descrição do estado de uma chave emitida (para o registo do administrador)
inline std::string issuedState (const IssuedKey& k, int today, bool activatedHere)
{
    if (k.validity == (int) IssueRequest::Permanent) return activatedHere ? "ATIVA · permanente" : "POR ATIVAR · permanente";
    if (k.validity == (int) IssueRequest::DaysAfterActivation) return activatedHere ? "ATIVA" : "POR ATIVAR · " + std::to_string (k.daysAfter) + " d";
    const int left = k.untilDay - today + 1;
    if (left <= 0) return "EXPIRADA";
    if (left <= 30) return "EXPIRA EM " + std::to_string (left) + " D";
    return activatedHere ? "ATIVA" : "POR ATIVAR";
}
}} // namespace sk::lic
