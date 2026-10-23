# Formato .bfpack

ZIP "stored" (abre em qualquer descompressor) com `manifest.json`, `samples/*.wav` (24 bit, **cifrados**
SHA-256-CTR, chave derivada do segredo e do id do pack), `presets/*.skpreset` (JSON), `cover.jpg`, `LICENSE.txt`.
O manifesto leva o SHA-256 de cada WAV e um MAC (HMAC-SHA256); o plugin recusa packs adulterados ou de terceiros.

Pastas de packs: macOS `/Library/Application Support/Piradex/SYNTHKEY/Packs` (todos os utilizadores) ou
`~/Library/Application Support/Piradex/SYNTHKEY/Packs`; Windows `C:\ProgramData\Piradex\SYNTHKEY\Packs` ou `%APPDATA%\Piradex\SYNTHKEY\Packs`.

Gerar: `packgen packs <saída> Resources/covers <segredo_hex | ->` (com `-` usa `Source/Secrets.h`).
