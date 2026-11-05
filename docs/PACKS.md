# Formato .bfpack

ZIP "stored" (abre em qualquer descompressor) com `manifest.json`, `samples/*.wav` (24 bit, **cifrados**
SHA-256-CTR, chave derivada do segredo e do id do pack), `presets/*.skpreset` (JSON), `cover.jpg`, `LICENSE.txt`.
O manifesto leva o SHA-256 de cada WAV e um MAC (HMAC-SHA256); o plugin recusa packs adulterados ou de terceiros.

Pastas de packs: macOS `/Library/Application Support/Piradex/SYNTHKEY/Packs` (todos os utilizadores) ou
`~/Library/Application Support/Piradex/SYNTHKEY/Packs`; Windows `C:\ProgramData\Piradex\SYNTHKEY\Packs` ou `%APPDATA%\Piradex\SYNTHKEY\Packs`.

Gerar: `packgen packs <saída> Resources/covers <segredo_hex | ->` (com `-` usa `Source/Secrets.h`).

## Atualizações automáticas

O plugin consulta um **manifesto** JSON (no máximo 1×/24 h ao abrir, ou em *PACKS → VERIFICAR ATUALIZAÇÕES*) e propõe as versões
mais recentes dos packs **já instalados**. Packs novos aparecem como novidade mas nunca se instalam sozinhos; o Núcleo vai com o plugin.

```json
{ "schema": 1, "published": "2026-11-05",
  "packs": [ { "id": "trap", "name": "Trap", "version": "1.2.0",
               "url": "https://…/SYNTHKEY_Pack_trap_1.2.0.bfpack",
               "sha256": "<SHA-256 hex (64) do ficheiro .bfpack>", "size": 123456789, "notes": "5 presets novos" } ] }
```

* `url` **https** obrigatório · `sha256` obrigatório (o download é rejeitado se não conferir) · `size` opcional (se existir, tem de coincidir).
* Depois do SHA-256, o instalador ainda valida o HMAC/assinatura do próprio pack: um ficheiro adulterado ou de terceiros é recusado.
* Endereço do manifesto: por omissão `docs/packs-manifest.json` deste repositório (branch `main`); para usar outro, criar `atualizacoes.txt`
  na pasta de dados do utilizador com a linha `url=https://…/manifest.json` (outras chaves: `auto=1` instala sozinho, `check=0` desliga a verificação diária).
* `sha256sum SYNTHKEY_Pack_trap_1.2.0.bfpack` gera o valor do manifesto.
* No Linux o download precisa de `JUCE_USE_CURL=1` (libcurl); em Windows e macOS usa o sistema.
