// =============================================================================
//  SYNTHKEY by Piradex — ZIP mínimo (método "stored", sem compressão), C++17 puro.
//  Um .bfpack é um ZIP normal: abre-se com qualquer descompressor. Os WAV de 24 bit
//  quase não comprimem, por isso o método "stored" não custa espaço e simplifica tudo.
// =============================================================================
#pragma once
#include <cstdint>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace sk { namespace zip
{
inline uint32_t crc32 (const void* data, size_t n, uint32_t crc = 0)
{
    static uint32_t table[256]; static bool init = false;
    if (! init) { for (uint32_t i = 0; i < 256; ++i) { uint32_t c = i; for (int k = 0; k < 8; ++k) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1; table[i] = c; } init = true; }
    const uint8_t* p = (const uint8_t*) data; crc = ~crc;
    for (size_t i = 0; i < n; ++i) crc = table[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);
    return ~crc;
}

class Writer
{
public:
    void add (const std::string& name, const std::string& bytes)
    {
        Entry e; e.name = name; e.crc = crc32 (bytes.data(), bytes.size()); e.size = (uint32_t) bytes.size(); e.offset = (uint32_t) out.size();
        put32 (0x04034b50); put16 (20); put16 (0x0800); put16 (0); put16 (0); put16 (0x21);   // versão, utf-8, stored, hora/data fixas
        put32 (e.crc); put32 (e.size); put32 (e.size); put16 ((uint16_t) name.size()); put16 (0); out += name; out += bytes;
        entries.push_back (e);
    }
    std::string finish()
    {
        const uint32_t cdStart = (uint32_t) out.size();
        for (auto& e : entries)
        {
            put32 (0x02014b50); put16 (20); put16 (20); put16 (0x0800); put16 (0); put16 (0); put16 (0x21);
            put32 (e.crc); put32 (e.size); put32 (e.size); put16 ((uint16_t) e.name.size()); put16 (0); put16 (0); put16 (0); put16 (0); put32 (0); put32 (e.offset); out += e.name;
        }
        const uint32_t cdSize = (uint32_t) out.size() - cdStart;
        put32 (0x06054b50); put16 (0); put16 (0); put16 ((uint16_t) entries.size()); put16 ((uint16_t) entries.size()); put32 (cdSize); put32 (cdStart); put16 (0);
        return std::move (out);
    }
private:
    struct Entry { std::string name; uint32_t crc, size, offset; };
    std::string out; std::vector<Entry> entries;
    void put16 (uint32_t v) { out += (char) v; out += (char) (v >> 8); }
    void put32 (uint32_t v) { for (int i = 0; i < 4; ++i) out += (char) (v >> (8 * i)); }
};

struct Item { std::string name; size_t offset = 0, size = 0; uint32_t crc = 0; };

// lê o índice de um ZIP "stored" (devolve false se estiver corrompido ou usar compressão).
// A origem pode ser memória ou um ficheiro (lê só o que precisa: ideal para packs de 20 MB)
class Reader
{
public:
    bool open (const std::string* data)
    {
        mem = data; file.reset();
        return parse (data->size());
    }
    bool openFile (const std::string& path)
    {
        file.reset (new std::ifstream (path, std::ios::binary)); mem = nullptr;
        if (! *file) return false;
        file->seekg (0, std::ios::end); const std::streamoff sz = file->tellg();
        if (sz < 22) return false;
        return parse ((size_t) sz);
    }
    const std::vector<Item>& list() const { return items; }
    const Item* find (const std::string& name) const { for (auto& i : items) if (i.name == name) return &i; return nullptr; }
    std::string read (const Item& it) const { return get (it.offset, it.size); }
    bool verifyCrc (const Item& it) const { const std::string b = read (it); return crc32 (b.data(), b.size()) == it.crc; }
private:
    const std::string* mem = nullptr; mutable std::unique_ptr<std::ifstream> file; std::vector<Item> items;
    std::string get (size_t off, size_t n) const
    {
        if (mem) return off + n <= mem->size() ? mem->substr (off, n) : std::string();
        std::string o (n, '\0'); file->clear(); file->seekg ((std::streamoff) off); file->read (&o[0], (std::streamsize) n);
        if ((size_t) file->gcount() != n) o.clear();
        return o;
    }
    static uint32_t u16 (const std::string& d, size_t o) { return (uint8_t) d[o] | ((uint32_t) (uint8_t) d[o + 1] << 8); }
    static uint32_t u32 (const std::string& d, size_t o) { return u16 (d, o) | (u16 (d, o + 2) << 16); }
    bool parse (size_t total)
    {
        items.clear(); if (total < 22) return false;
        const size_t tailLen = std::min<size_t> (total, 65557 + 22); const std::string tail = get (total - tailLen, tailLen);
        if (tail.size() != tailLen) return false;
        size_t e = std::string::npos;
        for (size_t i = tailLen - 22 + 1; i-- > 0;) if (u32 (tail, i) == 0x06054b50) { e = i; break; }
        if (e == std::string::npos) return false;
        const uint32_t count = u16 (tail, e + 10), cdSize = u32 (tail, e + 12), cdOff = u32 (tail, e + 16);
        if ((size_t) cdOff + cdSize > total) return false;
        const std::string cd = get (cdOff, cdSize); if (cd.size() != cdSize) return false;
        size_t p = 0;
        for (uint32_t k = 0; k < count; ++k)
        {
            if (p + 46 > cd.size() || u32 (cd, p) != 0x02014b50) return false;
            const uint32_t method = u16 (cd, p + 10), crc = u32 (cd, p + 16), csize = u32 (cd, p + 20), usize = u32 (cd, p + 24), nameLen = u16 (cd, p + 28), extraLen = u16 (cd, p + 30), cmtLen = u16 (cd, p + 32), lho = u32 (cd, p + 42);
            if (method != 0 || csize != usize || p + 46 + nameLen > cd.size()) return false;
            const std::string lh = get (lho, 30); if (lh.size() != 30 || u32 (lh, 0) != 0x04034b50) return false;
            const size_t dataOff = (size_t) lho + 30 + u16 (lh, 26) + u16 (lh, 28);
            if (dataOff + usize > total) return false;
            Item it; it.name = cd.substr (p + 46, nameLen); it.offset = dataOff; it.size = usize; it.crc = crc; items.push_back (it);
            p += 46 + nameLen + extraLen + cmtLen;
        }
        return true;
    }
};
}} // namespace sk::zip
