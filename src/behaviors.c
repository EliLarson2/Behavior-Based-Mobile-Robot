#include "behaviors.h"

#define CRUISE_SPEED            350
#define LIGHT_MIN               150
#define LIGHT_DIFF_DEADBAND      25
#define FOLLOW_SPEED            350
#define FOLLOW_TURN             100
#define SONAR_MIN_DIST         20.0f

static unsigned int repeated_ir_events = 0;

static void backup(void)
{
    STEPPER_move_stwt(
        STEPPER_BOTH,
        STEPPER_REV, 200, 200, 400, STEPPER_BRK_OFF,
        STEPPER_REV, 200, 200, 400, STEPPER_BRK_OFF);
}

static void turn_right(void)
{
    STEPPER_move_stwt(
        STEPPER_BOTH,
        STEPPER_FWD, 150, 250, 400, STEPPER_BRK_OFF,
        STEPPER_REV, 150, 250, 400, STEPPER_BRK_OFF);
}

void CRUISE(volatile MOTOR_ACTION *a)
{
    a->speed_L = CRUISE_SPEED;
    a->speed_R = CRUISE_SPEED;
    a->state = ROBOT_CRUISING;
}

void LIGHT_FOLLOW(volatile MOTOR_ACTION *a, volatile SENSOR_DATA *s)
{
    unsigned int avg = (s->photo_L + s->photo_R) / 2;

    if (avg > LIGHT_MIN)
    {
        int diff = (int)s->photo_L - (int)s->photo_R;
        a->state = LIGHT_HOMING;

        if (diff > LIGHT_DIFF_DEADBAND)
        {
            a->speed_L = FOLLOW_SPEED - FOLLOW_TURN;
            a->speed_R = FOLLOW_SPEED;
        }
        else if (diff < -LIGHT_DIFF_DEADBAND)
        {
            a->speed_L = FOLLOW_SPEED;
            a->speed_R = FOLLOW_SPEED - FOLLOW_TURN;
        }
        else
        {
            a->speed_L = FOLLOW_SPEED;
            a->speed_R = FOLLOW_SPEED;
        }
    }
}

void SONAR_AVOID(volatile MOTOR_ACTION *a, volatile SENSOR_DATA *s)
{
    if (s->sonar_dist < SONAR_MIN_DIST && s->sonar_dist > 0.0f)
    {
        a->speed_L = FOLLOW_SPEED - FOLLOW_TURN;
        a->speed_R = FOLLOW_SPEED;
        a->state = SONAR_AVOIDING;
    }
}

void IR_AVOID(volatile MOTOR_ACTION *a, volatile SENSOR_DATA *s)
{
    if (s->left_IR || s->right_IR)
    {
        a->state = IR_AVOIDING;
        repeated_ir_events++;

        STEPPER_stop(STEPPER_BOTH, STEPPER_BRK_ON);
        backup();

        if (s->left_IR && !s->right_IR)
        {
            turn_right();
        }
        else if (s->right_IR && !s->left_IR)
        {
            STEPPER_move_stwt(
                STEPPER_BOTH,
                STEPPER_REV, 150, 250, 400, STEPPER_BRK_OFF,
                STEPPER_FWD, 150, 250, 400, STEPPER_BRK_OFF);
        }
    }
}

void ESCAPE(volatile MOTOR_ACTION *a)
{
    if (repeated_ir_events >= 3)
    {
        a->state = ESCAPING;
        backup();
        turn_right();
        repeated_ir_events = 0;
    }
}

void apply_motor_action(volatile MOTOR_ACTION *a)
{
    STEPPER_move_stnb(
        STEPPER_BOTH,
        STEPPER_FWD, 10000, a->speed_L, 400, STEPPER_BRK_OFF,
        STEPPER_FWD, 10000, a->speed_R, 400, STEPPER_BRK_OFF);
}

void LCD_debug(volatile MOTOR_ACTION *a)
{
    LCD_clear();

    if (a->state == ESCAPING)
        LCD_printf("ESCAPE\n");
    else if (a->state == IR_AVOIDING)
        LCD_printf("IR AVOID\n");
    else if (a->state == SONAR_AVOIDING)
        LCD_printf("SONAR AVOID\n");
    else if (a->state == LIGHT_HOMING)
        LCD_printf("LIGHT FOLLOW\n");
    else
        LCD_printf("CRUISE\n");
}
