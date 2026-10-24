// =============================================================================
//  SYNTHKEY by Piradex — ecrã de ajuda / tutorial (abre na primeira utilização da v1.3)
// =============================================================================
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Look.h"
#include "../AppPaths.h"

namespace ui
{
inline juce::File helpSeenFile() { return skpaths::userDir().getChildFile ("help_v13.seen"); }

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
            { "\xE2\x9C\xA6 DESCREVER SOM",   "Escreve o que queres \xE2\x80\x94 \"baixo escuro e agressivo\", \"pad quente e largo\", \"pluck brilhante curto\", \"808 drill\" \xE2\x80\x94 no bot\xC3\xA3o-orbe da barra de cima e o SYNTHKEY monta um preset. Carrega outra vez para variantes." },
            { "ECR\xC3\x83 SOM (ORBIT)",       "Tudo numa vista: origens \xC3\xA0 esquerda, oscilador (separadores A, B, SUB + RU\xC3\x8D" "DO, VOZES), filtros, volume, efeitos e arpejador ao centro, matriz e macros \xC3\xA0 direita. O orbe mostra o espectro e a onda em tempo real." },
            { "ARRASTAR PARA MODULAR",        "Pega numa origem (LFO, envelope, STEP, DESENHO...) e larga-a em cima de qualquer potenci\xC3\xB3metro: aparece um anel com a amplitude. As origens em uso ficam iluminadas." },
            { "MATRIZ COMPACTA",              "Clica na origem ou no destino de uma linha para escolher; arrasta a barra para a quantidade (duplo clique = 0, bot\xC3\xA3o direito = limpar a linha). A lista completa (16) est\xC3\xA1 no separador MATRIZ." },
            { "TECLADO DE PERFORMANCE",       "Cada nota tem a sua cor e a dica mostra nome, frequ\xC3\xAAncia e velocidade. Clica mais abaixo na tecla para tocar mais forte. As rodas PITCH (volta ao centro) e MOD enviam MIDI." },
            { "IMPORTAR WAV",                 "No oscilador, bot\xC3\xA3o WAV: cria uma wavetable a partir de qualquer ficheiro. Arrasta um \xC3\xA1udio para a janela para o usares como instrumento (pack \"Utilizador\")." },
            { "ANULAR / REFAZER",             "Setas na barra de cima, ou Ctrl/Cmd+Z e Ctrl/Cmd+Shift+Z. No menu \xE2\x8B\xAF: guardar preset com etiquetas (Meu Lead #lead, brilhante), aleatorizar, ajuda." } };
        g.fillAll (juce::Colour (0xD0050610));
        theme::drawCard (g, card.toFloat(), 16.0f, true);
        g.setColour (theme::accent()); g.fillRoundedRectangle ((float) card.getX() + 24, (float) card.getY() + 26, 3.0f, 20.0f, 1.5f);
        g.setColour (theme::text()); g.setFont (theme::font (21.0f, true)); g.drawText (TX ("Novidades e atalhos \xE2\x80\x94 SYNTHKEY 1.3"), card.getX() + 38, card.getY() + 20, card.getWidth() - 60, 32, juce::Justification::centredLeft);
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
        card = juce::Rectangle<int> (0, 0, 860, 520).withCentre (getLocalBounds().getCentre());
        close.setBounds (card.getRight() - 24 - 210, card.getBottom() - 54, 210, 34);
    }
    void mouseDown (const juce::MouseEvent& e) override { if (! card.contains (e.getPosition())) dismiss(); }
private:
    juce::TextButton close; juce::Rectangle<int> card;
};
}
