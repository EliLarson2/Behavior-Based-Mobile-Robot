/* Original Lab 3 progression: basic stop, CRUISE + IR_AVOID, adaptive escape. */
#include "capi324v221.h"

BOOL IR_AVOID(void)
{
    if (ATTINY_get_IR_state(ATTINY_IR_BOTH))
    {
        STEPPER_stop(STEPPER_BOTH, STEPPER_BRK_ON);
        STEPPER_move_stwt(STEPPER_BOTH,
            STEPPER_REV, 400, 200, 400, STEPPER_BRK_OFF,
            STEPPER_REV, 400, 200, 400, STEPPER_BRK_OFF);
        return TRUE;
    }
    else if (ATTINY_get_IR_state(ATTINY_IR_LEFT))
    {
        STEPPER_stop(STEPPER_BOTH, STEPPER_BRK_ON);
        STEPPER_move_stwt(STEPPER_BOTH,
            STEPPER_REV, 200, 200, 400, STEPPER_BRK_OFF,
            STEPPER_REV, 200, 200, 400, STEPPER_BRK_OFF);
        STEPPER_move_stwt(STEPPER_BOTH,
            STEPPER_FWD, 150, 250, 400, STEPPER_BRK_OFF,
            STEPPER_REV, 150, 250, 400, STEPPER_BRK_OFF);
        return TRUE;
    }
    else if (ATTINY_get_IR_state(ATTINY_IR_RIGHT))
    {
        STEPPER_stop(STEPPER_BOTH, STEPPER_BRK_ON);
        STEPPER_move_stwt(STEPPER_BOTH,
            STEPPER_REV, 200, 200, 400, STEPPER_BRK_OFF,
            STEPPER_REV, 200, 200, 400, STEPPER_BRK_OFF);
        STEPPER_move_stwt(STEPPER_BOTH,
            STEPPER_REV, 150, 250, 400, STEPPER_BRK_OFF,
            STEPPER_FWD, 150, 250, 400, STEPPER_BRK_OFF);
        return TRUE;
    }
    return FALSE;
}
