# SYNTHKEY by Piradex

Sintetizador VSTi / Audio Unit híbrido (analógico + wavetable + samples) com **packs de estilo** (Trap, R&B,
Afrobeats, Amapiano, Drill, Kizomba, Kuduro) bloqueados por chave. Interface neon, 110 presets de fábrica.

* **Formatos:** VST3 + AU + Standalone (macOS) · VST3 + Standalone (Windows 10/11 x64)
* **macOS:** binário universal (Apple Silicon + Intel), **mínimo 10.13** · **Windows:** runtime estático (sem VC++ Redistributable)
* **Motor:** 2 motores (A/B: analógico, wavetable ou sample) · sub + ruído · 2 filtros (9 tipos) · 3 envelopes · 2 LFOs ·
  matriz de 16 slots · 8 macros · 2 inserts + aux (10 efeitos) · limitador · acordes/escalas/arpejador

## Como obter os instaladores (GitHub Actions)

1. Crie o repositório e envie este conteúdo (o `.gitignore` já exclui os ficheiros privados).
2. **Settings → Secrets and variables → Actions → New repository secret**
   * nome: `SYNTHKEY_SECRETS_B64`
   * valor: conteúdo do ficheiro `secrets.b64` (gerado por `python3 tools/gen_secrets.py`)
3. **Actions → Build SYNTHKEY → Run workflow**. No fim, a página **Releases** tem:
   * `SYNTHKEY-by-Piradex-1.1.1-macOS.dmg` (contém o `.pkg`, plugins soltos e packs)
   * `SYNTHKEY-by-Piradex-1.1.1-Windows-Setup.exe` (+ `…-Windows-Manual.zip`)
   * `SYNTHKEY_Pack_<estilo>_1.1.0.bfpack` (um por estilo)

O workflow corre os testes (DSP, chaves, packs, presets), gera os packs com o segredo, compila, verifica
arquiteturas/versão mínima do macOS, valida o AU (`auval`) e o VST3 (`pluginval`) e cria os instaladores.

## Senhas, administrador e chaves

* Ao abrir o plugin pede **senha**: `MASTER` (nunca expira, abre tudo) ou `BETA` (abre tudo até 01/11/2026).
  Só ficam no código o *salt* e o hash PBKDF2 — nunca a senha.
* **Separador ADMIN** (gerador de chaves com data limite): só com senha MASTER **e** o ficheiro `admin.key`
  na pasta de dados do utilizador. Ver `docs/ADMIN.md`.
* Cada pack tem a sua chave (`TRAP-XXXX-XXXX-XXXX-XXXX`, …): permanente, até uma data, ou N dias após a ativação.
  Gerador de linha de comandos: `python3 tools/keygen.py --help`.

## Estrutura

```
Source/Core/      motor, efeitos, chaves, packs (C++ puro, testado sem JUCE)
Source/UI/        componentes e páginas da interface
Source/           processador, editor, acesso (login/chaves), gestor de packs
tools/            packgen (gera .bfpack), gen_secrets.py, keygen.py, skkeys.py
tests/            test_core, test_packio, test_presets, smoke (processador)
installer/        .pkg/.dmg (macOS) e Inno Setup (Windows)
.github/workflows build.yml
```

## Compilar localmente

```
python3 tools/gen_secrets.py                    # 1.ª vez: gera Source/Secrets.h e as senhas
cmake -B build -DJUCE_PATH=/caminho/JUCE        # ou deixe sem JUCE_PATH para descarregar a 8.0.15
cmake --build build --config Release
g++ -O2 -std=c++17 -ISource tests/test_core.cpp -o test_core && ./test_core
```

## Limites honestos do licenciamento

O segredo das chaves está dentro do binário (necessário para validar offline): protege contra uso casual e
partilha de chaves, **não** contra engenharia inversa dedicada. O nº de computadores por chave não é imposto offline;
a ativação online está prevista para a v1.1.

## Novidades da v1.1.0

* **240 presets** de fábrica (Núcleo 83 + 21–23 por estilo).
* Separador **VISUAL**: osciloscópio, espectro, curva dos filtros e wavetable em 3D.
* **Anéis de modulação** nos botões: mostram o que a matriz faz a cada parâmetro.
* **Navegador de presets** (clique no nome do preset): pesquisa por nome/estilo/etiqueta, filtros por pack e categoria, favoritos (★).
* Separador **GROOVES**: 56 grooves, linhas de baixo, progressões de acordes e melodias por estilo (Trap, R&B, Afrobeat, Amapiano, Drill, Kizomba, Kuduro), com reprodução interna, escolha do tom e do BPM, **arrastar MIDI para o DAW** e botão **MODO ESTILO**.
* Correção: o modo "Reverso" dos samples agora toca.

## v1.1.1

* **Arpejador:** o padrão de 16 passos agora é editável — arrasta as barras (em baixo = pausa), clica no número para mudar a oitava, botão direito para repetir a nota ×2/×3, e escolhe quantos passos (1–16).
