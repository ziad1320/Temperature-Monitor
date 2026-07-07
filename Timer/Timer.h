#ifndef TIMER_H
#define TIMER_H

#include "../lib/std_types.h"

#define NUM_OF_TIMERS 4U
#define TIMER_2 2U
#define TIMER_3 3U
#define TIMER_4 4U
#define TIMER_5 5U

#define CH1 1U
#define CH2 2U
#define CH3 3U
#define CH4 4U

//[pinter to function
typedef void (*Timer_Callback) (void);

//functions
void Timer_Init(uint8 TimerId, uint16 Prescaler, uint16 AutoReload);
void Timer_Start(uint8 TimerId);
void Timer_Stop(uint8 TimerId);
void Timer_DelayMs(uint8 TimerId, uint32 DelayMs);
void Timer_DelayMsAsync(uint8 TimerId, uint32 DelayMs, Timer_Callback Callback);

#endif