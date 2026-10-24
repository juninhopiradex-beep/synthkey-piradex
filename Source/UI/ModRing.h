// =============================================================================
//  SYNTHKEY by Piradex — anéis de modulação nos potenciómetros
//  Mostra, à volta do botão, a amplitude que a matriz de modulação aplica a esse parâmetro.
// =============================================================================
#pragma once
#include <cmath>
#include "../Core/ParamTable.h"
#include "../PluginProcessor.h"

namespace ui
{
using namespace sk;

struct ModTarget { int param; float scale; bool logCut; };

// parâmetro "visível" que cada destino da matriz mexe, e quanto vale amount=1 (na unidade do parâmetro)
inline ModTarget modTargetFor (int dst)
{
    switch (dst)
    {
        case MD_PITCH_A:  return { P_A_COARSE, 24.0f, false };
        case MD_PITCH_B:  return { P_B_COARSE, 24.0f, false };
        case MD_PITCH_ALL: return { P_TRANSPOSE, 24.0f, false };
        case MD_WTPOS_A:  return { P_A_WTPOS, 1.0f, false };
        case MD_WTPOS_B:  return { P_B_WTPOS, 1.0f, false };
        case MD_WARP_A:   return { P_A_WARP, 1.0f, false };
        case MD_WARP_B:   return { P_B_WARP, 1.0f, false };
        case MD_CUT1:     return { P_F1_CUT, 5.0f, true };
        case MD_CUT2:     return { P_F2_CUT, 5.0f, true };
        case MD_RES1:     return { P_F1_RES, 1.0f, false };
        case MD_RES2:     return { P_F2_RES, 1.0f, false };
        case MD_DRIVE1:   return { P_F1_DRIVE, 1.0f, false };
        case MD_DRIVE2:   return { P_F2_DRIVE, 1.0f, false };
        case MD_UNIDET_A: return { P_A_UNIDET, 50.0f, false };
        case MD_UNIDET_B: return { P_B_UNIDET, 50.0f, false };
        case MD_PWM_A:    return { P_A_PWM, 0.45f, false };
        case MD_PWM_B:    return { P_B_PWM, 0.45f, false };
        case MD_FM_A:     return { P_A_FMAMT, 1.0f, false };
        case MD_FM_B:     return { P_B_FMAMT, 1.0f, false };
        case MD_START_A:  return { P_A_START, 1.0f, false };
        case MD_START_B:  return { P_B_START, 1.0f, false };
        case MD_SUB:      return { P_SUB_LEVEL, 30.0f, false };
        case MD_NOISE:    return { P_NOISE_LEVEL, 30.0f, false };
        case MD_FOLD_A:   return { P_A_FOLD, 1.0f, false };
        case MD_FOLD_B:   return { P_B_FOLD, 1.0f, false };
        case MD_FILTBAL:  return { P_FILT_BAL, 1.0f, false };
        case MD_SENDA:    return { P_SEND_A, 1.0f, false };
        case MD_SENDB:    return { P_SEND_B, 1.0f, false };
        default:          return { -1, 0.0f, false };
    }
}

// fontes que oscilam para os dois lados (LFOs, tecla, pitch bend, aleatório)
inline bool modSourceIsBipolar (int src) { return src == MS_LFO1 || src == MS_LFO2 || src == MS_KEY || src == MS_PITCHBEND || src == MS_RANDOM; }

// Calcula o intervalo [lo, hi] (em proporção 0..1 do curso do botão, relativo à posição atual) coberto pela modulação.
inline bool modRangeFor (const SynthKeyProcessor& proc, int paramIdx, float& lo, float& hi)
{
    const auto& pi = kParams[paramIdx]; const float span = pi.mx - pi.mn; if (span <= 0.0f) return false;
    float neg = 0.0f, pos = 0.0f; bool any = false;
    for (int s = 0; s < kNumModSlots; ++s)
    {
        const int base = modSlotBase (s);
        const int src = (int) proc.getReal (base), dst = (int) proc.getReal (base + 1); const float amt = proc.getReal (base + 2);
        if (src == 0 || dst == 0 || std::fabs (amt) < 0.005f) continue;
        const auto t = modTargetFor (dst); if (t.param != paramIdx) continue;
        const float frac = t.logCut ? amt * t.scale / std::log2 (pi.mx / pi.mn) : amt * t.scale / span;
        any = true;
        if (modSourceIsBipolar (src)) { pos += std::fabs (frac); neg -= std::fabs (frac); }
        else if (frac >= 0.0f) pos += frac; else neg += frac;
    }
    lo = neg; hi = pos; return any;
}

// assinatura rápida da matriz (para a interface saber quando redesenhar os botões)
inline int modMatrixSignature (const SynthKeyProcessor& proc)
{
    int sig = 7;
    for (int s = 0; s < kNumModSlots; ++s)
    {
        const int base = modSlotBase (s);
        sig = sig * 31 + (int) proc.getReal (base); sig = sig * 31 + (int) proc.getReal (base + 1);
        sig = sig * 31 + (int) std::lround (proc.getReal (base + 2) * 1000.0f);
    }
    return sig;
}
}
