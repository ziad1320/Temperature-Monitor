#include "Timer.h"
#include "Timer_Private.h"
#include "../lib/std_types.h"
#include "../lib/Bit_Operations.h"
#include "../Nvic/Nvic.h"

// address mapping
static uint32 timer_base_addresses[NUM_OF_TIMERS] = {TIM2_BASE_ADDR, TIM3_BASE_ADDR, TIM4_BASE_ADDR, TIM5_BASE_ADDR};
#define TIMER_GET_PERIPHERAL(TimerId)   ((TimerType *) timer_base_addresses[TimerId - TIMER_2]) 

/* NVIC IRQ numbers:  TIM2=28, TIM3=29, TIM4=30, TIM5=50 */
static uint8 Timer_NvicIrq[NUM_OF_TIMERS] = {28, 29, 30, 50};

static Timer_Callback Timer_Callbacks[NUM_OF_TIMERS] = {0};


void Timer_init(uint8 TimerId, uint16 Prescaler, uint16 AutoReload) {
    TimerType* timer = TIMER_GET_PERIPHERAL(TimerId);
    timer ->CR1 = 0; //reset timer (control register)
    timer ->PSC = Prescaler;
    timer ->ARR = AutoReload;
    timer ->CNT = 0; // counter
    SET_BIT(timer -> EGR, EGR_UG); // Force update to load PSC & ARR (event generation register)
    timer ->SR = 0; // Clear the update flag that UG set (status refister)
}

void Timer_Start(uint8 TimerId) {
    TimerType* timer = TIMER_GET_PERIPHERAL(TimerId);
    SET_BIT(timer ->CR1, CR1_CEN);
}

void Timer_Stop(uint8 TimerId) {
    TimerType* timer = TIMER_GET_PERIPHERAL(TimerId);
    CLEAR_BIT(timer ->CR1, CR1_CEN);
}

/**
 *  Synchronous delay — blocks until timer overflows
 *  16 MHz HSI  ÷  (PSC+1 = 16000) = 1 kHz  →  1 tick = 1 ms
 *
 *  if ARR = 0 -> #ticks = 1, then #ticks = ARR + 1
 */
void Timer_DelayMs(uint8 TimerId, uint32 DelayMs) {
    TimerType* timer = TIMER_GET_PERIPHERAL(TimerId);
    timer->CR1 = 0; // Stop & reset
    timer->PSC = 15999U / 3; // division by 3 to handle proteus error (15999 not 16000 as we already add 1 to prevent div by 0 in calculating psc )
    timer->ARR = (uint16) (DelayMs - 1); // - 1 as 0 ARR accounts for 1 pulse 
    timer->CNT = 0;

    SET_BIT(timer->EGR, EGR_UG); // Load shadow registers
    timer->SR = 0; // Clear UIF caused by UG

    SET_BIT(timer->CR1, CR1_OPM); // One-pulse mode (only excutes the timer I gave him ones not continuous)
    SET_BIT(timer->CR1, CR1_CEN); // Start counting

    while (!READ_BIT(timer->SR, SR_UIF)) { // Update interrupt flag
        // Poll – CPU is blocked here
    }

    timer->SR = 0; // Clear UIF
    CLEAR_BIT(timer->CR1, CR1_CEN); // Stop counter
}


// Async means it doesn't freez the program until delay is finished
void Timer_DelayMsAsync(uint8 TimerId, uint32 DelayMs, Timer_Callback Callback) {
    TimerType *timer =(TimerType *)timer_base_addresses[TimerId - TIMER_2];
    uint8 index = TimerId - TIMER_2;
    uint8 irqNum = Timer_NvicIrq[index]; //from NVIC table

    Timer_Callbacks[index] = Callback; //setting callback pointer to function to trigger the handler function when we reach ARR

    timer->CR1 = 0; // Stop & reset
    timer->PSC = 15999U / 3;
    timer->ARR = (uint16) (DelayMs - 1);
    timer->CNT = 0;

    SET_BIT(timer->EGR, EGR_UG); // Load shadow registers
    timer->SR = 0; // Clear UIF caused by UG

    SET_BIT(timer->CR1, CR1_OPM); // One-pulse mode

    SET_BIT(timer->DIER, DIER_UIE); // Enable update interrupt (from timer's side)
    Nvic_EnableIrq(irqNum); // Enable interrupt (from NVIC's side) 

    SET_BIT(timer->CR1, CR1_CEN); // Start counting
}



static void Timer_HandleIrq(uint8 index) {
    TimerType *timer =(TimerType *)timer_base_addresses[index];


    if (READ_BIT(timer->SR, SR_UIF)) {
        timer->SR = 0; // Clear UIF
        CLEAR_BIT(timer->DIER, DIER_UIE); // Disable further IRQs
        CLEAR_BIT(timer->CR1, CR1_CEN); // Stop counter

        if (Timer_Callbacks[index] != 0) {
            Timer_Callbacks[index]();
        }
    }
}

void TIM2_IRQHandler(void) {
    Timer_HandleIrq(0);
}
void TIM3_IRQHandler(void) {
    Timer_HandleIrq(1);
}
void TIM4_IRQHandler(void) {
    Timer_HandleIrq(2);
}
void TIM5_IRQHandler(void) {
    Timer_HandleIrq(3);
}
