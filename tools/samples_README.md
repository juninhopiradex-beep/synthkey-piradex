# SYNTHKEY — packs de samples

Gera os packs `.bfpack` (Trap, R&B, Afrobeats, Amapiano, Drill, Kizomba, Kuduro) do plugin **SYNTHKEY by Piradex**.
Os samples são sintetizados proceduralmente e gravados em 24 bit, cifrados e assinados (ver `docs/PACKS.md` no repositório do plugin).

1. Defina o segredo `SYNTHKEY_SECRETS_B64` (Settings → Secrets → Actions) — o mesmo do repositório `synthkey-piradex`.
2. **Actions → Gerar packs SYNTHKEY → Run workflow**. Os ficheiros `.bfpack` ficam em **Releases**.

Cada pack só abre no plugin com a sua chave (gerada no separador ADMIN do plugin).
