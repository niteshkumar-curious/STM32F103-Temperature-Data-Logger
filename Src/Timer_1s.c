#include "Timer_1s.h"


#include <stdbool.h>




#define RCC_APB1ENR     (*(volatile uint32_t *)0x4002101C)




#define TIM3_CR1        (*(volatile uint32_t *)0x40000400)
#define TIM3_DIER       (*(volatile uint32_t *)0x4000040C)
#define TIM3_SR         (*(volatile uint32_t *)0x40000410)
#define TIM3_PSC        (*(volatile uint32_t *)0x40000428)
#define TIM3_ARR        (*(volatile uint32_t *)0x4000042C)


#define NVIC_ISER0      (*(volatile uint32_t *)0xE000E100)




#define TIM_CR1_CEN     (1 << 0)     // Counter enable
#define TIM_DIER_UIE    (1 << 0)     // Update interrupt enable
#define TIM_SR_UIF      (1 << 0)     // Update interrupt flag




volatile uint32_t timer_seconds = 0;



void Timer3_init(void)
{
    
    RCC_APB1ENR |= (1 << 1);


    /*  Prescaler

       Timer clock = 8 MHz

       8,000,000 / 800
       = 10,000 Hz

       Therefore:
       1 timer tick = 100 us
    */
    TIM3_PSC = 800 - 1;


    /*  Auto-reload register

       10,000 timer ticks � 100 us
       = 1 second

       
    */
    TIM3_ARR = 10000 - 1;


    /*  Enable TIM3 interrupt */
    TIM3_DIER |= TIM_DIER_UIE;


    /*  Enable TIM3 interrupt in NVIC

       TIM3 IRQ number = 29
    */
    NVIC_ISER0 |= (1 << 29);


    /*  Start TIM3 counter */
    TIM3_CR1 |= TIM_CR1_CEN;
}




void TIM3_IRQHandler(void)
{
   
    if (TIM3_SR & TIM_SR_UIF)
    {
        
        TIM3_SR &= ~TIM_SR_UIF;


        
        timer_seconds++;
    }
}


