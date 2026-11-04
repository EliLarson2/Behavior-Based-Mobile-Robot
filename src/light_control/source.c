#include "capi324v221.h"

#define MAX_SPEED 600
#define BASE_SPEED 600
#define BIAS 60

void CBOT_main(void)
{
    ADC_SAMPLE right_adc;
    ADC_SAMPLE left_adc;
    float right_v;
    float left_v;
    int left_speed;
    int right_speed;

    ADC_open();
    ADC_set_VREF(ADC_VREF_AVCC);

    while (1)
    {
        ADC_set_channel(ADC_CHAN3);
        right_adc = ADC_sample();
        right_v = (right_adc * 5.0f) / 1024.0f;

        ADC_set_channel(ADC_CHAN4);
        left_adc = ADC_sample();
        left_v = (left_adc * 5.0f) / 1024.0f;

        /* Crossed excitatory controller with orbit bias. */
        left_speed = (int)((right_v / 5.0f) * MAX_SPEED);
        right_speed = (int)((left_v / 5.0f) * MAX_SPEED) + BIAS;

        (void)left_speed;
        (void)right_speed;
        TMRSRVC_delay(200);
    }
}
