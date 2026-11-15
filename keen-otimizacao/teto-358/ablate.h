#include "ablate_cfg.h"
// Throw-away ablation switches (measurement only). All off by default.
#ifndef ABL_H
#define ABL_H 192          // rows of the viewport that the ring redraws
#endif
#ifndef ABL_NOENEMY
#define ABL_NOENEMY 0
#endif
#ifndef ABL_NOHUD
#define ABL_NOHUD 0
#endif
#ifndef ABL_COMBAT
#define ABL_COMBAT 2       // CombatTick calls per 2 logic steps: 2 normal, 1 half rate, 0 none
#endif
#ifndef ABL_DRAW2
#define ABL_DRAW2 0        // draw only every 2nd main-loop iteration
#endif
