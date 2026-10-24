// =============================================================================
//  SYNTHKEY by Piradex — ecrã de ajuda / tutorial (abre na primeira utilização da v1.5)
// =============================================================================
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Look.h"
#include "../AppPaths.h"

namespace ui
{
inline juce::File helpSeenFile() { return skpaths::userDir().getChildFile ("help_v15.seen"); }

class HelpOverlay : public juce::Component
{
public:
    HelpOverlay()
    {
        addAndMakeVisible (close); close.setButtonText (TX ("PERCEBI \xE2\x80\x94 FECHAR")); close.onClick = [this] { dismiss(); };
        setVisible (false);
    }
    void open() { setVisible (true); toFront (true); resized(); repaint(); }
    void openIfFirstRun() { if (! helpSeenFile().existsAsFile()) open(); }
    void dismiss() { helpSeenFile().replaceWithText ("1"); setVisible (false); }

    void paint (juce::Graphics& g) override
    {
        struct Item { const char* head; const char* body; };
        static const Item items[] = {
            { "LOOPS NO TEMPO DO PROJETO",    "Os loops (packs e Est\xC3\xBA" "dio) seguem o andamento da DAW: esticam-se sem mudar a nota e repetem sem emenda, alinhados com a grelha do projeto. No orbe v\xC3\xAA o anel de fase e \"96 \xE2\x86\x92 120 bpm\"; com o projeto parado o loop toca ao tempo do projeto." },
            { "MIDI E WAV PARA A DAW",        "Em GROOVES, arrasta MIDI \xE2\x86\x92 DAW ou WAV \xE2\x86\x92 DAW para uma pista. O WAV sai com o som escolhido em SOM e faz loop sem emenda; = PROJETO p\xC3\xB5" "e o andamento do projeto. Se a tua DAW n\xC3\xA3o aceitar o arrasto, clica na caixa: Guardar como\xE2\x80\xA6 ou Mostrar na pasta." },
            { "WAV \xE2\x86\x92 MIDI",             "Em GROOVES, o bot\xC3\xA3o WAV \xE2\x86\x92 MIDI (ou arrasta um \xC3\xA1udio para a p\xC3\xA1gina) descobre as notas de bateria, baixo, melodia ou acordes e cria um groove edit\xC3\xA1vel em MEUS. Diz o bpm se o souberes \xE2\x80\x94 fica mais certo." },
            { "EDITOR + IMPORTAR MIDI",       "Clica na grelha para criar notas, numa nota para a apagar; arrasta na vertical para a velocidade. NOVO, IMPORTAR MIDI (ou arrasta um .mid) e MEUS guardam na tua pasta; os de f\xC3\xA1" "brica geram uma c\xC3\xB3pia." },
            { "\xE2\x9C\xA6 DESCREVER SOM",   "Escreve o que queres \xE2\x80\x94 \"baixo escuro e agressivo\", \"pad quente e largo\", \"808 drill\" \xE2\x80\x94 no bot\xC3\xA3o-orbe da barra de cima e o SYNTHKEY monta um preset. Carrega outra vez para variantes." },
            { "CAMADAS (GROOVES)", "No separador GROOVES escolhe o som do groove (Auto = kit de bateria do estilo, Igual ao teu som, ou qualquer preset) e o volume. O groove toca com o seu pr\xC3\xB3prio som enquanto tu tocas outro no teclado." },
            { "EST\xC3\x9A" "DIO DE CONTE\xC3\x9A" "DO", "Na zona ADMIN (EST\xC3\x9A" "DIO DE CONTE\xC3\x9A" "DO): junta samples, loops, multi-samples, MIDI e presets, e gera um pack .bfpack. Nos loops o SYNTHKEY deteta bpm e tempos (ou l\xC3\xAA \"96bpm\" do nome); LOOP\xE2\x80\xA6 corrige. INSTALAR NO PLUGIN usa-o logo." },
            { "ECR\xC3\x83 SOM (ORBIT)",       "Tudo numa vista: origens \xC3\xA0 esquerda, oscilador (separadores A, B, SUB + RU\xC3\x8D" "DO, VOZES), filtros, volume, efeitos e arpejador ao centro, matriz e macros \xC3\xA0 direita. O orbe mostra o espectro e a onda em tempo real." },
            { "ARRASTAR PARA MODULAR",        "Pega numa origem (LFO, envelope, STEP, DESENHO...) e larga-a em cima de qualquer potenci\xC3\xB3metro: aparece um anel com a amplitude. As origens em uso ficam iluminadas." },
            { "IMPORTAR WAV",                 "No oscilador, bot\xC3\xA3o WAV: cria uma wavetable a partir de qualquer ficheiro. Arrasta um \xC3\xA1udio para a janela para o usares como instrumento (pack \"Utilizador\"); se o nome tiver \"loop\" ou \"96bpm\", vira um loop no tempo do projeto. Anular/refazer: Ctrl/Cmd+Z." } };
        g.fillAll (juce::Colour (0xD0050610));
        theme::drawCard (g, card.toFloat(), 16.0f, true);
        g.setColour (theme::accent()); g.fillRoundedRectangle ((float) card.getX() + 24, (float) card.getY() + 26, 3.0f, 20.0f, 1.5f);
        g.setColour (theme::text()); g.setFont (theme::font (21.0f, true)); g.drawText (TX ("Novidades e atalhos \xE2\x80\x94 SYNTHKEY 1.5"), card.getX() + 38, card.getY() + 20, card.getWidth() - 60, 32, juce::Justification::centredLeft);
        int y = card.getY() + 66;
        for (auto& it : items)
        {
            g.setColour (theme::accent()); g.setFont (theme::font (13.0f, true)); g.drawText (TX (it.head), card.getX() + 28, y, 200, 20, juce::Justification::topLeft);
            g.setColour (theme::text()); g.setFont (theme::font (13.0f)); g.drawFittedText (TX (it.body), card.getX() + 230, y, card.getWidth() - 260, 44, juce::Justification::topLeft, 3, 1.0f);
            y += 54;
        }
    }
    void resized() override
    {
        card = juce::Rectangle<int> (0, 0, 860, 690).withCentre (getLocalBounds().getCentre());
        close.setBounds (card.getRight() - 24 - 210, card.getBottom() - 54, 210, 34);
    }
    void mouseDown (const juce::MouseEvent& e) override { if (! card.contains (e.getPosition())) dismiss(); }
private:
    juce::TextButton close; juce::Rectangle<int> card;
};
}
