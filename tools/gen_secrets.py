#!/usr/bin/env python3
"""
SYNTHKEY by Piradex — gera os segredos do build.

Cria (e NUNCA deve ir para o GitHub):
  Source/Secrets.h                  usado pelo compilador (já está no .gitignore)
  PRIVADO_senhas_SYNTHKEY.txt       as senhas em claro: master, beta, ficheiro de administrador, segredo das chaves
  secrets.b64                       conteúdo para o "secret" SYNTHKEY_SECRETS_B64 do GitHub Actions

Uso:   python3 tools/gen_secrets.py                      # gera tudo novo
       python3 tools/gen_secrets.py --master "MinhaSenha" # escolhe a senha master
       python3 tools/gen_secrets.py --beta-until 2026-11-01
"""
import argparse, base64, hashlib, os, secrets as sec, datetime, sys

ALPHA = "0123456789ABCDEFGHJKMNPQRSTVWXYZ"
ITER = 60000


def group(prefix, n_groups):
    return prefix + "".join("-" + "".join(sec.choice(ALPHA) for _ in range(4)) for _ in range(n_groups))


def pbkdf2(pw: str, salt: bytes) -> bytes:
    return hashlib.pbkdf2_hmac("sha256", pw.strip().encode(), salt, ITER, 32)


def carray(b: bytes) -> str:
    return ", ".join("0x%02x" % x for x in b)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--master"); ap.add_argument("--beta")
    ap.add_argument("--beta-until", default="2026-11-01")
    ap.add_argument("--out", default=os.path.join(os.path.dirname(__file__), ".."))
    a = ap.parse_args()
    root = os.path.abspath(a.out)
    until = datetime.date.fromisoformat(a.beta_until)

    master = a.master or group("PXM", 4)
    beta = a.beta or group("PXB", 3)
    admin_token = sec.token_hex(16)
    mac_secret = sec.token_bytes(32)
    mask = sec.token_bytes(32)
    masked = bytes(x ^ y for x, y in zip(mac_secret, mask))

    s_m, s_b, s_a = sec.token_bytes(16), sec.token_bytes(16), sec.token_bytes(16)
    h_m, h_b, h_a = pbkdf2(master, s_m), pbkdf2(beta, s_b), pbkdf2(admin_token, s_a)

    header = f'''// GERADO por tools/gen_secrets.py — NÃO COMMITAR (está no .gitignore).
#pragma once
#include "Core/Licensing.h"

namespace sk {{ namespace secrets
{{
// Segredo das chaves de packs (ofuscado com máscara; ver nota de segurança em Core/Keys.h)
inline const unsigned char kMasked[32] = {{ {carray(masked)} }};
inline const unsigned char kMask[32]   = {{ {carray(mask)} }};
inline std::string macSecret()
{{
    std::string s (32, '\\0');
    for (int i = 0; i < 32; ++i) s[(size_t) i] = (char) (kMasked[i] ^ kMask[i]);
    return s;
}}

// Senhas de acesso: só salt + hash PBKDF2-HMAC-SHA256 ({ITER} iterações)
inline const lic::PwEntry kPasswords[] =
{{
    {{ "master", lic::Role::Master, "{s_m.hex()}", "{h_m.hex()}", 0, 0, 0 }},
    {{ "beta-1", lic::Role::Beta,   "{s_b.hex()}", "{h_b.hex()}", {until.year}, {until.month}, {until.day} }},
}};
inline constexpr int kPasswordCount = 2;

// Ficheiro de administrador (admin.key na pasta do utilizador): token em claro -> hash
inline const char* kAdminSaltHex = "{s_a.hex()}";
inline const char* kAdminHashHex = "{h_a.hex()}";
}} }} // namespace sk::secrets
'''
    src = os.path.join(root, "Source", "Secrets.h")
    with open(src, "w") as f:
        f.write(header)
    with open(os.path.join(root, "secrets.b64"), "w") as f:
        f.write(base64.b64encode(header.encode()).decode())
    priv = f'''SYNTHKEY by Piradex — SENHAS PRIVADAS  (guarda num gestor de senhas; NUNCA no GitHub)
=====================================================================================

Senha MASTER (acesso total, não expira):
    {master}

Senha BETA (abre o plugin até ao fim de {until.strftime("%d/%m/%Y")}):
    {beta}

Ficheiro de ADMINISTRADOR (para ver o separador ADMIN / gerador de chaves):
  Cria um ficheiro de texto chamado  admin.key  com ESTA linha (e mais nada):
    {admin_token}
  e coloca-o em:
    macOS:    ~/Library/Application Support/Piradex/SYNTHKEY/admin.key
    Windows:  %APPDATA%\\Piradex\\SYNTHKEY\\admin.key
  O separador ADMIN só aparece com a senha MASTER + este ficheiro.

Segredo das chaves dos packs (hex) — usado pelo gerador de chaves em Python (tools/keygen.py):
    {mac_secret.hex()}

Se perderes isto, não consegues emitir chaves compatíveis com o plugin já distribuído.
'''
    with open(os.path.join(root, "PRIVADO_senhas_SYNTHKEY.txt"), "w") as f:
        f.write(priv)
    with open(os.path.join(root, ".synthkey_secret_hex"), "w") as f:
        f.write(mac_secret.hex())
    print("Gerado: Source/Secrets.h, secrets.b64, PRIVADO_senhas_SYNTHKEY.txt")


if __name__ == "__main__":
    main()
