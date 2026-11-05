# SYNTHKEY by Piradex

Sintetizador VSTi / Audio Unit híbrido (analógico + wavetable + samples) com **packs de estilo** (Trap, R&B,
Afrobeats, Amapiano, Drill, Kizomba, Kuduro) bloqueados por chave. Interface neon, 573 presets de fábrica (+ 46 presets de módulo); os 291 do Núcleo adicionados em 2026-11 só foram validados tecnicamente, ainda não ouvidos.

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
   * `SYNTHKEY-by-Piradex-1.5.1-macOS.dmg` (contém o `.pkg`, plugins soltos e packs)
   * `SYNTHKEY-by-Piradex-1.5.1-Windows-Setup.exe` (+ `…-Windows-Manual.zip`)
   * `SYNTHKEY_Pack_<estilo>_1.2.0.bfpack` (um por estilo)

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

* **573 presets** de fábrica (Núcleo 380 + 25–30 por estilo) e **46 presets de módulo**. Os 291 presets novos do Núcleo (anexo v1.5) (Cordas, Metais, Sinos, Mallets, Guitarras, Órgãos, Sopros, Acordes, Sequências, Texturas, Cinemático, Híbrido, Experimental) têm `qa: "auto"` — passaram nos testes automáticos mas ainda não foram validados ao ouvido.
* Separador **VISUAL**: osciloscópio, espectro, curva dos filtros e wavetable em 3D.
* **Anéis de modulação** nos botões: mostram o que a matriz faz a cada parâmetro.
* **Navegador de presets** (clique no nome do preset): pesquisa por nome/estilo/etiqueta, filtros por pack e categoria, favoritos (★).
* Separador **GROOVES**: 56 grooves, linhas de baixo, progressões de acordes e melodias por estilo (Trap, R&B, Afrobeats, Amapiano, Drill, Kizomba, Kuduro), com reprodução interna, escolha do tom e do BPM, **arrastar MIDI para o DAW** e botão **MODO ESTILO**.
* Correção: o modo "Reverso" dos samples agora toca.

## v1.1.1

* **Arpejador:** o padrão de 16 passos agora é editável — arrasta as barras (em baixo = pausa), clica no número para mudar a oitava, botão direito para repetir a nota ×2/×3, e escolhe quantos passos (1–16).

## v1.5.1

* Barra de navegação de baixo mais visível: texto mais claro, separador ativo em pílula colorida com brilho, barra com linha de realce.

## v1.5.0 — loops no tempo, MIDI/WAV para a DAW, WAV → MIDI

* **Loops sincronizados com o projeto:** os loops (dos packs e do Estúdio) seguem o andamento da DAW — esticam-se sem mudar a nota (WSOLA, em thread de fundo), repetem sem emenda e ficam alinhados com a grelha (posição ligada ao relógio do projeto; com o projeto parado tocam ao tempo do projeto). O orbe mostra o anel de fase e "96 → 120 bpm ×1,25".
* **Estúdio:** o BPM e os tempos de cada loop são detetados (ou lidos do nome, ex. `Loop_96bpm.wav`); botão **LOOP…** para corrigir.
* **GROOVES → DAW:** caixas **MIDI → DAW** e **WAV → DAW** (o WAV é renderizado com o som escolhido, no andamento do slider ou do projeto — botão **= PROJETO** — e faz loop sem emenda). Se a DAW não aceitar o arrasto, clica na caixa: *Guardar como…*, *Mostrar na pasta*, *Copiar o caminho*. Nomes de ficheiro só com ASCII.
* **WAV → MIDI:** botão (ou arrasta um áudio para a página) que deteta bateria, baixo, melodia ou acordes, o andamento e as notas, e cria um groove editável em MEUS.
* Limitações: o som foi validado por medição, não de ouvido; WAV → MIDI é aproximado em misturas complexas (o groove fica editável; podes indicar o bpm); não consegui testar o arrasto numa DAW real (só em X11).

## v1.4.0 — camadas, editor, estúdio

* **Camadas:** o groove toca com o seu próprio som (Auto = kit de bateria do estilo, "igual ao teu som" ou qualquer preset) e volume próprio, enquanto tocas outro som no teclado.
* **Editor de grooves:** piano-roll editável (criar/apagar notas, velocidade), **NOVO**, **IMPORTAR MIDI** (.mid, quantizado a semicolcheias, deteta bateria/acordes/baixo/melodia), grooves "MEUS" guardados em `MeusGrooves`.
* **Estúdio de conteúdo (ADMIN):** juntar samples, loops, multi-samples, MIDI e presets por estilo e gerar um pack `.bfpack` (com capa, kit automático e presets automáticos); "Instalar no plugin" para usar logo.
* **Mais som:** kits de bateria B e C em todos os estilos, vozes (Eh/Ee/Oh/Ey/Ah), claves, timbal, pandeireta e congas, e ~40 presets novos (camadas, percussão, vozes). Packs passam à versão 1.2.0.
* Limitações: os sons de fábrica são sintéticos.

## v1.3.0 — interface "Orbit"

* **Novo ecrã SOM numa só vista:** origens de modulação à esquerda (LFO 1/2, ENV 2, STEP, DESENHO, ENV MULTI + velocidade,
  tecla, pressão…, com mini-gráficos reais e destaque das que estão em uso), oscilador com o **orbe** (espectro radial em tempo
  real + forma de onda + anéis de posição/modulação) e separadores A · B · SUB+RUÍDO · VOZES, filtros com curva e zona de
  modulação do corte, volume, resumo dos efeitos, arpejador em miniatura, **matriz compacta** (clica para escolher origem/destino,
  arrasta a barra) e macros.
* **Teclado de performance:** cada nota tem a sua cor, mostra a velocidade (clica mais abaixo na tecla = mais forte), ponto
  luminoso nas notas premidas e dica com nome, frequência e velocidade. Rodas de **PITCH** e **MOD** que enviam MIDI.
* **Barra de cima nova:** anular/refazer, navegação de presets com etiqueta da categoria, botão-orbe **✦ Descrever som…**,
  ajuda, modos Fácil/Avançado/Expert e paletas (nova paleta **ORBIT** violeta/ciano por defeito).
* **Navegação em baixo** (SOM · MATRIZ · MODULAD. · EFEITOS · ARP · VISUAL · GROOVES · PACKS); nos outros separadores as macros,
  o volume e as origens ficam numa faixa por cima do teclado.
* Mesmo motor de som e mesmos presets da 1.2 (nenhum parâmetro mudou).

## v1.2.0

* **Motor:** qualidade Normal / Alta (2x) / Máxima (4x) com sobre-amostragem (menos aliasing); filtro **Formante** (vogais); reverb e delay mais suaves; **Delay sincronizado** ao tempo do anfitrião.
* **Importar WAV:** botão WAV no oscilador cria uma wavetable a partir de qualquer ficheiro; arrasta áudio para a janela para o usar como **instrumento próprio** (pack "Utilizador", nunca bloqueado). A wavetable do utilizador fica guardada no projecto do DAW, não dentro dos presets.
* **Separador MODULAD.:** sequenciador de passos, LFO desenhado à mão, envelope multi-ponto; **MPE** e **aftertouch por nota**; novas origens na MATRIZ (STEP, DESENHO, ENV MULTI, PRESSÃO, SLIDE).
* **Arrastar para modular:** pega numa origem da faixa por baixo do teclado e larga-a em qualquer botão.
* **Anular / Refazer** (Ctrl/Cmd+Z, Ctrl/Cmd+Shift+Z) e presets guardados com **etiquetas** (`Nome #etiqueta, outra`).
* **✦ DESCREVER:** escreve o som em palavras ("baixo escuro e agressivo", "pad quente e largo", "808 drill") e o SYNTHKEY monta um preset (regras internas, sem internet), com volume calibrado. Ecrã de **ajuda** na primeira abertura.
* Correção: a origem de modulação aceita agora todas as fontes (as 5 novas não se podiam escolher).

## v1.5.0 (em desenvolvimento) — trabalho posterior à auditoria

* **Motor:** corrigidos início do sample (`MD_START`), anti-aliasing de sync/dobra/FM, regressão do oversampling (FIR de 90 dB) e limitador O(1); envios A/B passam a ser **por voz** (antes dos inserts); corrigida uma leitura fora do buffer no `DelayLine` (afetava todos os efeitos).
* **Novo motor Corda** (modelação física, Karplus-Strong): tipo de motor "Corda" com Amortecimento, Duração e Palheta (`a_ks_*` / `b_ks_*`).
* **Split / Layer:** cada motor tem zona de teclas (`a_keylo/a_keyhi`, `b_keylo/b_keyhi`).
* **5 efeitos novos:** Flanger, Ensemble, Shimmer, Multi-Tap, Pitch (parâmetros 1–5 normalizados 0–1; ver `Fx.h`).
* **Arpejador:** ritmo euclidiano (`arp_eucl_k/rot`) e probabilidade com semente reprodutível (`arp_prob`, `arp_seed`).
* **Presets:** 13 famílias novas (Cordas, Metais, Sinos, Mallets, Guitarras, Órgãos, Sopros, Acordes, Sequências, Texturas, Cinemático, Híbrido, Experimental), modelação física, split/layer, e 46 presets de módulo (osc/filtro/env/mod/fx/arp) que só alteram o seu módulo. Formato `.skpreset` 2 com família, subfamília, motor, género, papel, módulo, QA e versão; ficheiros do formato 1 continuam a abrir.
* **Estado de validação:** os presets novos têm `qa: auto` (passaram nos testes automáticos de nível, DC, cauda, mono, duplicados e NaN); **nenhum foi ouvido por uma pessoa**. O morph entre presets existe no núcleo (`morphParams`) mas ainda não tem controlo na interface.
* Os ganhos de calibração do Núcleo (`NucleoGains.h`) são gerados por `packgen gains` e um teste falha se faltar algum preset.

## v1.6.0 (em desenvolvimento) — motores Granular e Espectral, 4 packs novos, 3 450 presets

* **Motor Granular** (tipo 4): até 12 grãos Hann por voz sobre a zona de sample, semente determinística por nota. Controlos: Tamanho, Densidade, Posição, Aleatório, Altura±, Velocidade (−1…2, negativo = reverso) e Espalhar (`a_gr_*` / `b_gr_*`).
* **Motor Espectral** (tipo 5): síntese aditiva (256 harmónicas) por tabela de 32 frames band-limited — Inclinação, Formante, Nitidez, Pente, Par/Ímpar, Varrer, Dispersão (`a_sp_*` / `b_sp_*`). A tabela reconstrói-se de forma incremental na thread de áudio (sem esperas nem alocações); a POSIÇÃO percorre o espectro e é modulável por voz.
* **4 packs novos de género:** House · Deep · Tech, EDM · Future Bass, Pop · Dance, Cinematic (capas provisórias geradas, a substituir por arte final).
* **3 450 presets** (alvo do PDF: 3 480): Factory 2 124 / 2 150 e expansões 1 326 / 1 330. 738 escritos à mão ("bases") e 2 712 variações geradas por `gen::derive` (código de 27 bits = 9 eixos × 3 bits) e **curadas por máquina** (`packgen curate`: render, impressão digital de 8 características, rejeição de quase-duplicados, ganho calibrado) — tabela em `GenCurated.h`.
* **Quotas ainda abaixo do PDF** (o gerador não encontrou variações suficientemente distintas): Vocals −10, Percussion −10, SFX −5, Drums −1 (Factory) e Cinematic −4 (pack). Resolve-se com mais bases escritas à mão.
* **Estado de validação:** todos os gerados têm `qa: "auto"` — **ninguém os ouviu**. A medição agora é determinística (motor novo por medição; antes o estado residual de um preset alterava o ganho em até ±2 dB).
* **Qualidade "Auto":** novo modo (além de Normal / 2x / 4x) que liga o oversampling 2x só quando o patch usa **dobra forte** (>30 %) ou **sync alto** (≥12 st) no motor Analógico, com histerese. Medido: dobra 100 % na nota 93 passa de ≈ −22 dB (Normal) para −44 dB (2x); sync 24 st na nota 93 de ≈ −13 dB para −47 dB. Em Normal esses extremos continuam a ter aliasing audível — é o custo de não sobreamostrar.
* **Limites conhecidos que NÃO são defeitos a corrigir:**
  * *Pads largos com perda em mono de 3,0–3,3 dB:* vem da cauda do reverb, descorrelacionada entre L e R (uma cauda totalmente descorrelacionada perde ≈ 3 dB ao somar em mono). Reduzir a largura do chorus não altera o valor. Os presets "Largo/Wide" perdem mais (4–6 dB) por desenho.
  * *Envios A/B por voz tirados antes dos inserts:* é uma escolha de desenho (permite modulação de envio por tecla/voz, que um envio pós-inserts, feito sobre o barramento somado, não permite).
* **Teste da Corda:** a janela de cauda (1,5–2,0 s) ultrapassava o vector renderizado (leitura fora dos limites); corrigido (200 blocos).
* **Regenerar:** `packgen curate Source/Core/GenCurated.h 0.6` (≈9 min) e depois `packgen gains Source/Core/NucleoGains.h`. Um teste falha se um ganho curado estiver desactualizado.
