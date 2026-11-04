#include "sensors.h"

void sensors_init(void)
{
    ADC_open();
    ADC_set_VREF(ADC_VREF_AVCC);
    USONIC_open();
}

void IR_sense(volatile SENSOR_DATA *s, TIMER16 interval)
{
    static BOOL started = FALSE;
    static TIMEROBJ timer;

    if (!started)
    {
        TMRSRVC_new(&timer, TMRFLG_NOTIFY_FLAG, TMRTCM_RESTART, interval);
        started = TRUE;
    }
    else if (timer.tc)
    {
        s->left_IR = ATTINY_get_IR_state(ATTINY_IR_LEFT);
        s->right_IR = ATTINY_get_IR_state(ATTINY_IR_RIGHT);
        timer.tc = 0;
    }
}

void Photo_sense(volatile SENSOR_DATA *s, TIMER16 interval)
{
    static BOOL started = FALSE;
    static TIMEROBJ timer;

    if (!started)
    {
        TMRSRVC_new(&timer, TMRFLG_NOTIFY_FLAG, TMRTCM_RESTART, interval);
        started = TRUE;
    }
    else if (timer.tc)
    {
        ADC_set_channel(ADC_CHAN3);
        s->photo_R = ADC_sample() - 30;

        ADC_set_channel(ADC_CHAN4);
        s->photo_L = ADC_sample();

        timer.tc = 0;
    }
}

void SONAR_sense(volatile SENSOR_DATA *s, TIMER16 interval)
{
    static BOOL started = FALSE;
    static TIMEROBJ timer;

    if (!started)
    {
        TMRSRVC_new(&timer, TMRFLG_NOTIFY_FLAG, TMRTCM_RESTART, interval);
        started = TRUE;
    }
    else if (timer.tc)
    {
        SWTIME ticks = USONIC_ping();
        unsigned long time_us = 10UL * (unsigned long)ticks;
        s->sonar_dist = 0.01724f * (float)time_us;
        timer.tc = 0;
    }
}
