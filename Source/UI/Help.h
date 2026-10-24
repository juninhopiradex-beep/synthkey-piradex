// =============================================================================
//  SYNTHKEY by Piradex — ecrã de ajuda / tutorial (abre na primeira utilização da v1.2)
// =============================================================================
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Look.h"
#include "../AppPaths.h"

namespace ui
{
inline juce::File helpSeenFile() { return skpaths::userDir().getChildFile ("help_v12.seen"); }

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
            { "\xE2\x9C\xA6 DESCREVER SOM",   "Escreve o que queres \xE2\x80\x94 \"baixo escuro e agressivo\", \"pad quente e largo\", \"pluck brilhante curto\", \"808 drill\", \"amapiano log drum\" \xE2\x80\x94 e o SYNTHKEY monta um preset. Carrega outra vez para variantes." },
            { "ARRASTAR PARA MODULAR",        "Pega numa origem da faixa (LFO, envelope, SEQ, DESENHO...) e larga-a em cima de qualquer potenci\xC3\xB3metro. Aparece um anel com a amplitude; afina na MATRIZ." },
            { "MODULAD.",                     "Sequenciador de passos, LFO desenhado \xC3\xA0 m\xC3\xA3o e envelope multi-ponto. Clica e arrasta para desenhar. MPE e aftertouch por nota j\xC3\xA1 est\xC3\xA3o ligados." },
            { "IMPORTAR WAV",                 "No oscilador, bot\xC3\xA3o WAV: cria uma wavetable a partir de qualquer ficheiro. Arrasta um \xC3\xA1udio para a janela para o usares como instrumento (pack \"Utilizador\")." },
            { "ANULAR / REFAZER",             "Ctrl/Cmd+Z e Ctrl/Cmd+Shift+Z (ou menu \xE2\x8B\xAF) desfazem qualquer altera\xC3\xA7\xC3\xA3o de som." },
            { "QUALIDADE",                    "Normal, Alta (2x) ou M\xC3\xA1xima (4x) em SOM \xE2\x86\x92 Qualidade: menos aliasing, mais CPU. Filtro Formante e Delay sincronizado ao tempo do anfitri\xC3\xA3o." },
            { "GUARDAR COM ETIQUETAS",        "Menu \xE2\x8B\xAF \xE2\x86\x92 Guardar preset: escreve o nome e, depois de #, as etiquetas (ex.: Meu Lead #lead, brilhante). Pesquisa-as no navegador de presets." } };
        g.fillAll (juce::Colour (0xD0050610));
        theme::drawCard (g, card.toFloat(), 16.0f, true);
        g.setColour (theme::accent()); g.fillRoundedRectangle ((float) card.getX() + 24, (float) card.getY() + 26, 3.0f, 20.0f, 1.5f);
        g.setColour (theme::text()); g.setFont (theme::font (21.0f, true)); g.drawText (TX ("Novidades e atalhos \xE2\x80\x94 SYNTHKEY 1.2"), card.getX() + 38, card.getY() + 20, card.getWidth() - 60, 32, juce::Justification::centredLeft);
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
