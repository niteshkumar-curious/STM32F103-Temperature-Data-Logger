#include "temp_sensor.h"

#define RCC_APB2ENR (*(volatile uint32_t *)0x40021018)
#define RCC_APB1ENR (*(volatile uint32_t *)0x4002101C)
#define GPIOA_CRL   (*(volatile uint32_t *)0x40010800)
#define ADC_CR1     (*(volatile uint32_t *)0x40012404)
#define ADC_CR2     (*(volatile uint32_t *)0x40012408)
#define ADC_SR      (*(volatile uint32_t *)0x40012400)
#define ADC_DR      (*(volatile uint32_t *)0x4001244C)

#define NVIC_ISER0  (*(volatile uint32_t *)0xE000E100)

#define ADC_ON      (1 << 0)
#define ADC_RSTCAL  (1 << 3)
#define ADC_CAL     (1 << 2)
#define ADC_EOC     (1 << 1)
#define ADC_EOCIE   (1 << 5)
#define TIM_SR_UIF  (1 << 0)
#define ADC_CONT    (1 << 1) 
#define VOLATAGE    3.3f

static volatile uint16_t adc_raw = 0;
static volatile uint16_t adc_filtered = 0;
static volatile bool new_temp_ready = false;

static const float R_fixed = 9800.0f;
static const float T0 = 298.15f;
static const float R0 = 10000.0f;
static const float B = 3950.0f;

static float custom_ln(float x)
{
    if (x <= 0.0f) return -1000.0f; 

    union {
        float f;
        uint32_t i;
    } conv;

    conv.f = x;
    uint32_t ex = conv.i >> 23;
    int32_t t = (int32_t)ex - 127;

    conv.i = 1065353216 | (conv.i & 8388607);
    float m = conv.f;

    float ln_m = -1.49278f + (2.11263f + (-0.729104f + 0.10969f * m) * m) * m;
    return ln_m + 0.69314718f * (float)t;
}

void ADC1_2_IRQHandler(void)
{
    if (ADC_SR & ADC_EOC)
    {
        adc_raw = (uint16_t)ADC_DR;

        if (adc_filtered == 0) {
            adc_filtered = adc_raw;
        } else {
            adc_filtered = ((adc_filtered * 15) + adc_raw) >> 4;
        }
        new_temp_ready = true;
    }
}



void temp_sensor_init(void)
{
    // ADC Init
    RCC_APB2ENR |= (1 << 0) | (1 << 2) | (1 << 9);
    GPIOA_CRL &= ~(0xF << 0);
    ADC_CR2 |= ADC_ON;

    for (volatile int i = 0; i < 1000; i++);

    ADC_CR2 |= ADC_RSTCAL;
    while (ADC_CR2 & ADC_RSTCAL);

    ADC_CR2 |= ADC_CAL;
    while (ADC_CR2 & ADC_CAL);

    ADC_CR1 |= ADC_EOCIE;
    NVIC_ISER0 |= (1 << 18);

    

    
}

bool temp_sensor_get(volatile float *calculated_temp)
{

    ADC_CR2 |= ADC_ON;
    if (!new_temp_ready) {
        return false;
    }
    new_temp_ready = false;

    if (adc_filtered > 10 && adc_filtered < 4080)
    {
        float voltage = (adc_filtered * VOLATAGE) / 4095.0f;
        if (voltage < 3.29f)
        {
            float R_ntc = (voltage * R_fixed) / (VOLATAGE - voltage);
            if (R_ntc > 0.0f)
            {
                float ln_ratio = custom_ln(R_ntc / R0);
                float tempk = 1.0f / ((1.0f / T0) + (ln_ratio / B));
                *calculated_temp = tempk - 273.15f;
							
								return true;
            }
        }
    }
    return false;
}