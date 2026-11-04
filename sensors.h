#ifndef SENSORS_H
#define SENSORS_H

#include "robot_types.h"

void sensors_init(void);
void IR_sense(volatile SENSOR_DATA *s, TIMER16 interval);
void Photo_sense(volatile SENSOR_DATA *s, TIMER16 interval);
void SONAR_sense(volatile SENSOR_DATA *s, TIMER16 interval);

#endif
