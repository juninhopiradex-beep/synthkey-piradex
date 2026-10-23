// =============================================================================
//  SYNTHKEY by Piradex — chaves de packs (uma por estilo), validadas offline
//
//  Formato:  PREFIXO-XXXX-XXXX-XXXX-XXXX   (Base32 Crockford, 80 bits)
//    40 bits de dados : [1] relativa [13] dias [2] computadores-1 [24] n.º de série
//    40 bits de MAC   : HMAC-SHA256(segredo, "synthkey-key|" + pack + "|" + dados)
//  "dias": absoluta = dias desde 2026-01-01 (0 = permanente); relativa = dias após a ativação.
//
//  NOTA DE SEGURANÇA (honesta): o MAC é simétrico, por isso o segredo vai no plugin
//  (ofuscado). Isto trava uso casual, partilha ingénua e chaves inventadas; não impede
//  um atacante determinado que extraia o segredo do binário. A solução forte é a
//  ativação online pela loja (planeada para a versão 1.1).
// =============================================================================
#pragma once
#include "Crypto.h"
#include <cstdint>
#include <string>

namespace sk { namespace keys
{
struct Payload
{
    bool relative = false;      // validade conta a partir da ativação
    bool permanent = true;
    int  expiryDay = 0;         // absoluta: dias desde 2026-01-01; relativa: n.º de dias
    int  seats = 1;             // 1..4 computadores (registo; não imposto offline)
    uint32_t serial = 0;        // 24 bits
};

inline const char* prefixFor (const std::string& packId)
{
    if (packId == "trap") return "TRAP"; if (packId == "rnb") return "RNB"; if (packId == "afrobeat") return "AFRO";
    if (packId == "amapiano") return "AMAP"; if (packId == "drill") return "DRILL"; if (packId == "kizomba") return "KIZ"; if (packId == "kuduro") return "KUDU";
    return "PACK";
}

inline int daysFromCivil (int y, unsigned m, unsigned d)
{
    y -= m <= 2; const int era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned) (y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1, doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (int) doe - 719468;
}
inline int daysSince2026 (int y, int m, int d) { return daysFromCivil (y, (unsigned) m, (unsigned) d) - daysFromCivil (2026, 1, 1); }
inline void civilFromDays2026 (int days, int& y, int& m, int& d)
{
    int z = days + daysFromCivil (2026, 1, 1) + 719468; const int era = (z >= 0 ? z : z - 146096) / 146097; const unsigned doe = (unsigned) (z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; y = (int) yoe + era * 400; const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100), mp = (5 * doy + 2) / 153;
    d = (int) (doy - (153 * mp + 2) / 5 + 1); m = (int) (mp < 10 ? mp + 3 : mp - 9); if (m <= 2) ++y;
}

inline const char* b32Alphabet() { return "0123456789ABCDEFGHJKMNPQRSTVWXYZ"; }

inline crypto::Bytes packBits (const Payload& p)
{
    const uint64_t days = p.permanent && ! p.relative ? 0 : (uint64_t) (p.expiryDay & 0x1FFF);
    uint64_t v = ((uint64_t) (p.relative ? 1 : 0) << 39) | (days << 26) | ((uint64_t) ((p.seats - 1) & 3) << 24) | (uint64_t) (p.serial & 0xFFFFFF);
    crypto::Bytes b (5, '\0'); for (int i = 0; i < 5; ++i) b[(size_t) i] = (char) (v >> (8 * (4 - i))); return b;
}
inline crypto::Bytes macFor (const std::string& secret, const std::string& packId, const crypto::Bytes& data)
{
    return crypto::hmacSha256 (secret, "synthkey-key|" + packId + "|" + data).substr (0, 5);
}

inline std::string makeKey (const std::string& secret, const std::string& packId, const Payload& p)
{
    crypto::Bytes all = packBits (p) + macFor (secret, packId, packBits (p));   // 10 bytes = 80 bits
    std::string chars;
    uint64_t hi = 0, lo = 0; for (int i = 0; i < 5; ++i) hi = (hi << 8) | (unsigned char) all[(size_t) i]; for (int i = 5; i < 10; ++i) lo = (lo << 8) | (unsigned char) all[(size_t) i];
    for (int i = 0; i < 16; ++i)   // 5 bits por caracter, do bit 79 para o 0
    {
        const int shift = 75 - 5 * i; unsigned v;
        if (shift >= 40) v = (unsigned) ((hi >> (shift - 40)) & 31); else if (shift + 5 <= 40) v = (unsigned) ((lo >> shift) & 31); else v = (unsigned) (((hi << (40 - shift)) | (lo >> shift)) & 31);
        chars += b32Alphabet()[v];
    }
    std::string out = std::string (prefixFor (packId));
    for (int g = 0; g < 4; ++g) out += "-" + chars.substr ((size_t) g * 4, 4);
    return out;
}

// valida a chave para um pack. Não verifica datas (isso é feito por quem usa o resultado)
inline bool parseKey (const std::string& secret, const std::string& packId, const std::string& text, Payload& out)
{
    std::string c; for (char ch : text) { if (ch == ' ' || ch == '-' || ch == '\t' || ch == '\n' || ch == '\r') { c += (ch == '-' ? '-' : ' '); } else c += (char) std::toupper ((unsigned char) ch); }
    // remover prefixo (antes do primeiro '-')
    const size_t dash = c.find ('-'); std::string body = dash == std::string::npos ? c : c.substr (dash + 1);
    std::string clean; for (char ch : body) { if (ch == ' ' || ch == '-') continue; if (ch == 'O') ch = '0'; if (ch == 'I' || ch == 'L') ch = '1'; clean += ch; }
    if (clean.size() != 16) return false;
    uint64_t hi = 0, lo = 0; int bit = 0;
    for (char ch : clean)
    {
        const char* pos = std::strchr (b32Alphabet(), ch); if (! pos || ! *pos) return false; const unsigned v = (unsigned) (pos - b32Alphabet());
        for (int k = 4; k >= 0; --k) { const unsigned b = (v >> k) & 1; if (bit < 40) hi = (hi << 1) | b; else lo = (lo << 1) | b; ++bit; }
    }
    crypto::Bytes data (5, '\0'), mac (5, '\0'); for (int i = 0; i < 5; ++i) { data[(size_t) i] = (char) (hi >> (8 * (4 - i))); mac[(size_t) i] = (char) (lo >> (8 * (4 - i))); }
    if (! crypto::constantTimeEq (mac, macFor (secret, packId, data))) return false;
    uint64_t v = 0; for (int i = 0; i < 5; ++i) v = (v << 8) | (unsigned char) data[(size_t) i];
    out.relative = ((v >> 39) & 1) != 0; out.expiryDay = (int) ((v >> 26) & 0x1FFF); out.seats = (int) ((v >> 24) & 3) + 1; out.serial = (uint32_t) (v & 0xFFFFFF);
    out.permanent = ! out.relative && out.expiryDay == 0;
    return true;
}
}} // namespace sk::keys
