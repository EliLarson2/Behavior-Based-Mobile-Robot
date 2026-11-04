#ifndef BEHAVIORS_H
#define BEHAVIORS_H

#include "robot_types.h"

void CRUISE(volatile MOTOR_ACTION *a);
void LIGHT_FOLLOW(volatile MOTOR_ACTION *a, volatile SENSOR_DATA *s);
void SONAR_AVOID(volatile MOTOR_ACTION *a, volatile SENSOR_DATA *s);
void IR_AVOID(volatile MOTOR_ACTION *a, volatile SENSOR_DATA *s);
void ESCAPE(volatile MOTOR_ACTION *a);
void apply_motor_action(volatile MOTOR_ACTION *a);
void LCD_debug(volatile MOTOR_ACTION *a);

#endif
