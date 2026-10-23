// Testes de licenciamento, ZIP e formato .bfpack (C++ puro)
#include "Core/PackIO.h"
#include <chrono>
#include <cstdio>
using namespace sk;
static int fails = 0, checks = 0;
#define CHECK(c, msg) do { ++checks; if (! (c)) { ++fails; std::printf ("FALHOU: %s (linha %d)\n", msg, __LINE__); } } while (0)

int main()
{
    const std::string secret = "segredo-de-teste-0123456789abcdef";
    // --- zip
    { zip::Writer w; w.add ("a.txt", "ola"); w.add ("dir/b.bin", std::string (1000, '\x7f')); std::string z = w.finish();
      zip::Reader r; CHECK (r.open (&z), "zip abre"); CHECK (r.list().size() == 2, "zip 2 itens");
      CHECK (r.read (*r.find ("a.txt")) == "ola", "zip conteúdo"); CHECK (r.verifyCrc (*r.find ("dir/b.bin")), "zip crc");
      z[z.size() / 2] ^= 0x55; zip::Reader r2; bool ok = r2.open (&z); (void) ok; }
    // --- crc32 vetor conhecido
    CHECK (zip::crc32 ("123456789", 9) == 0xCBF43926u, "crc32 vetor");
    // --- senhas
    {
        auto mk = [&] (const char* pw, const char* saltHex, std::string& hashHex) { hashHex = crypto::toHex (crypto::pbkdf2Sha256 (pw, crypto::fromHex (saltHex), lic::kPbkdf2Iterations, 32)); };
        std::string h1, h2; mk ("MASTER-PW", "00112233445566778899aabbccddeeff", h1); mk ("BETA-PW", "ffeeddccbbaa99887766554433221100", h2);
        lic::PwEntry e[2] = { { "master", lic::Role::Master, "00112233445566778899aabbccddeeff", h1.c_str(), 0, 0, 0 },
                              { "beta-1", lic::Role::Beta, "ffeeddccbbaa99887766554433221100", h2.c_str(), 2026, 11, 1 } };
        const int d1031 = keys::daysSince2026 (2026, 10, 31), d1101 = keys::daysSince2026 (2026, 11, 1), d1102 = keys::daysSince2026 (2026, 11, 2);
        bool ex = false;
        CHECK (lic::matchPassword (e, 2, "MASTER-PW", d1102, ex) == 0 && ! ex, "master nunca expira");
        CHECK (lic::matchPassword (e, 2, "  MASTER-PW \n", d1031, ex) == 0, "trim");
        CHECK (lic::matchPassword (e, 2, "BETA-PW", d1031, ex) == 1, "beta antes do fim");
        CHECK (lic::matchPassword (e, 2, "BETA-PW", d1101, ex) == 1, "beta no último dia");
        CHECK (lic::matchPassword (e, 2, "BETA-PW", d1102, ex) == -1 && ex, "beta expirada");
        CHECK (lic::matchPassword (e, 2, "beta-pw", d1031, ex) == -1 && ! ex, "senha errada");
        CHECK (lic::effectiveToday (d1031, d1102) == d1102, "relógio recuado");
    }
    // --- chaves, ativações, estados
    {
        std::vector<lic::Activation> acts; const int today = keys::daysSince2026 (2026, 10, 23);
        lic::IssueRequest rq; rq.packs = { "trap", "kuduro" }; rq.validity = lic::IssueRequest::UntilDate; rq.untilDay = keys::daysSince2026 (2026, 12, 31); rq.quantity = 2; rq.note = "cliente";
        uint32_t serial = 100; auto issued = lic::issueKeys (secret, rq, serial, today);
        CHECK (issued.size() == 4 && serial == 104, "emitidas 4");
        CHECK (lic::activate (secret, "kuduro", issued[0].key, today, acts) == lic::ActivateResult::Invalid, "chave de outro pack");
        CHECK (lic::activate (secret, "trap", issued[0].key, today, acts) == lic::ActivateResult::Ok, "ativa");
        CHECK (lic::activate (secret, "trap", issued[0].key, today, acts) == lic::ActivateResult::AlreadyActive, "repetida");
        auto st = lic::packStatus (secret, "trap", false, lic::Role::None, true, today, acts);
        CHECK (st.access == lic::Access::Key && st.daysLeft == (rq.untilDay - today + 1), "estado ativo + dias");
        st = lic::packStatus (secret, "trap", false, lic::Role::None, true, rq.untilDay + 1, acts); CHECK (st.access == lic::Access::Expired, "expirada");
        st = lic::packStatus (secret, "rnb", false, lic::Role::None, true, today, acts); CHECK (st.access == lic::Access::Locked, "bloqueado");
        st = lic::packStatus (secret, "rnb", false, lic::Role::Beta, true, today, acts); CHECK (st.access == lic::Access::Beta, "beta abre");
        st = lic::packStatus (secret, "rnb", false, lic::Role::Beta, false, today, acts); CHECK (st.access == lic::Access::Locked, "beta sem abrir tudo");
        st = lic::packStatus (secret, "rnb", false, lic::Role::Master, true, today, acts, true); CHECK (st.access == lic::Access::Locked, "simular cliente");
        st = lic::packStatus (secret, "nucleo", true, lic::Role::None, true, today, acts); CHECK (st.access == lic::Access::Core, "núcleo livre");
        // adulterar o dia de ativação (relativa)
        lic::IssueRequest r2; r2.packs = { "afrobeat" }; r2.validity = lic::IssueRequest::DaysAfterActivation; r2.daysAfter = 7; auto k2 = lic::issueKeys (secret, r2, serial, today);
        CHECK (lic::activate (secret, "afrobeat", k2[0].key, today, acts) == lic::ActivateResult::Ok, "relativa ativa");
        st = lic::packStatus (secret, "afrobeat", false, lic::Role::None, true, today + 6, acts); CHECK (st.access == lic::Access::Key && st.daysLeft == 1, "7.º dia");
        st = lic::packStatus (secret, "afrobeat", false, lic::Role::None, true, today + 7, acts); CHECK (st.access == lic::Access::Expired, "8.º dia expirada");
        acts.back().activatedDay += 100; st = lic::packStatus (secret, "afrobeat", false, lic::Role::None, true, today + 7, acts); CHECK (st.access != lic::Access::Key, "MAC da ativação impede edição");
        // permanente
        lic::IssueRequest r3; r3.packs = { "drill" }; auto k3 = lic::issueKeys (secret, r3, serial, today);
        CHECK (lic::activate (secret, "drill", k3[0].key, today, acts) == lic::ActivateResult::Ok, "permanente");
        st = lic::packStatus (secret, "drill", false, lic::Role::None, true, today + 5000, acts); CHECK (st.access == lic::Access::Key && st.daysLeft == -1, "permanente sempre");
        // tolerância de escrita (minúsculas, espaços)
        std::string lower = k3[0].key; for (auto& c : lower) c = (char) std::tolower ((unsigned char) c); std::vector<lic::Activation> a2;
        CHECK (lic::activate (secret, "drill", " " + lower + " ", today, a2) == lic::ActivateResult::Ok, "tolerante");
        // chave já expirada na emissão
        lic::IssueRequest r4; r4.packs = { "rnb" }; r4.validity = lic::IssueRequest::UntilDate; r4.untilDay = today - 1; auto k4 = lic::issueKeys (secret, r4, serial, today);
        CHECK (lic::activate (secret, "rnb", k4[0].key, today, a2) == lic::ActivateResult::Expired, "expirada à entrada");
        CHECK (lic::issuedState (issued[0], today, true) == "ATIVA", "estado emitido");
    }
    // --- pack completo (Trap)
    {
        const auto defs = buildPackDefs(); const PackDef* trap = nullptr; for (auto& d : defs) if (d.id == "trap") trap = &d;
        Library lib; buildPackLibrary (*trap, lib);
        std::vector<Preset> presets; Preset p; p.name = "Teste 808"; p.pack = "trap"; p.sampleA = "trap.808long"; p.set (P_MASTER_VOL, -6.0f); presets.push_back (p);
        const auto t0 = std::chrono::steady_clock::now();
        std::string bytes = packio::buildPack (*trap, lib, presets, "JPEGFAKE", secret);
        const double ms = std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - t0).count();
        std::printf ("pack trap: %.1f MB, %zu instrumentos, %zu amostras, build %.0f ms\n", bytes.size() / 1048576.0, lib.inst.size(), lib.data.size(), ms);
        packio::PackInfo info; CHECK (packio::readInfo (bytes, secret, info), "readInfo"); CHECK (info.id == "trap" && info.presets == 1, "info");
        CHECK (! packio::readInfo (bytes, "outro-segredo", info), "segredo errado rejeita");
        Library back; std::string err; const auto t1 = std::chrono::steady_clock::now();
        CHECK (packio::loadLibrary (bytes, secret, back, &err), ("loadLibrary: " + err).c_str());
        std::printf ("load: %.0f ms\n", std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - t1).count());
        CHECK (back.inst.size() == lib.inst.size() && back.data.size() == lib.data.size(), "mesmo número de instrumentos/dados");
        const int ii = back.indexOf ("trap.808long"); CHECK (ii >= 0, "instrumento 808long");
        if (ii >= 0) { const auto& a = lib.inst[(size_t) lib.indexOf ("trap.808long")]; const auto& b = back.inst[(size_t) ii]; CHECK (a.zones.size() == b.zones.size() && a.zones[0].root == b.zones[0].root, "zonas");
            const auto& da = *lib.data[(size_t) a.zones[0].data]; const auto& db = *back.data[(size_t) b.zones[0].data]; CHECK (da.frames() == db.frames(), "mesmos frames");
            double maxd = 0; for (int i = 0; i < da.frames(); ++i) maxd = std::max (maxd, (double) std::fabs (da.l[(size_t) i] - db.l[(size_t) i])); CHECK (maxd < 2e-7, "áudio idêntico (24 bit)"); }
        std::vector<Preset> rp; CHECK (packio::readPresets (bytes, rp) && rp.size() == 1 && rp[0].name == "Teste 808" && rp[0].get (P_MASTER_VOL) == -6.0f, "preset ida e volta");
        std::string jpg; CHECK (packio::readCover (bytes, jpg) && jpg == "JPEGFAKE", "capa");
        // adulteração
        std::string bad = bytes; bad[bad.size() / 3] ^= 0x10; Library l2; CHECK (! packio::loadLibrary (bad, secret, l2, &err), "adulterado rejeitado");
        Library l3; CHECK (! packio::loadLibrary (bytes, "outro-segredo", l3, &err), "segredo errado rejeitado");
        // sem o segredo certo os bytes do WAV não são áudio: verificar que o ficheiro cifrado não contém "RIFF"
        CHECK (bytes.find ("RIFF") == std::string::npos, "WAV cifrado (sem RIFF em claro)");
    }
    std::printf ("%d verificações, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
