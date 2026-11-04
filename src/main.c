#include "capi324v221.h"
#include "robot_types.h"
#include "sensors.h"
#include "behaviors.h"

void CBOT_main(void)
{
    SENSOR_DATA sensors = {0};
    MOTOR_ACTION action = {350, 350, ROBOT_CRUISING};

    STEPPER_open();
    LCD_open();
    LCD_clear();
    sensors_init();

    while (1)
    {
        IR_sense(&sensors, 125);
        Photo_sense(&sensors, 150);
        SONAR_sense(&sensors, 100);

        /* Lowest to highest priority. */
        CRUISE(&action);
        LIGHT_FOLLOW(&action, &sensors);
        SONAR_AVOID(&action, &sensors);
        IR_AVOID(&action, &sensors);
        ESCAPE(&action);

        apply_motor_action(&action);
        LCD_debug(&action);

        TMRSRVC_delay(100);
    }
}
