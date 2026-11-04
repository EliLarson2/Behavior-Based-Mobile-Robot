#ifndef ROBOT_TYPES_H
#define ROBOT_TYPES_H

#include "capi324v221.h"

typedef enum
{
    ROBOT_CRUISING,
    LIGHT_HOMING,
    SONAR_AVOIDING,
    IR_AVOIDING,
    ESCAPING
} ROBOT_STATE;

typedef struct
{
    BOOL left_IR;
    BOOL right_IR;
    ADC_SAMPLE photo_L;
    ADC_SAMPLE photo_R;
    float sonar_dist;
} SENSOR_DATA;

typedef struct
{
    int speed_L;
    int speed_R;
    ROBOT_STATE state;
} MOTOR_ACTION;

#endif
