/*
 * timer_a3.c
 *
 *  Created on: Dec 19, 2019
 *      Author: dantonets
 */

#include "stdio.h"
#include "string.h"
#include "file.h"
#include "driverlib.h"
#include "gsd.h"

/*********************************************
  *   Timer used for control the Rx timeout for nRF905. 
  *   If Rx timeout expired, the FW used a default values.
  *	Timer interrupt rate is exactly 0.25Hz (4 secs) = [32kHz/FFFFh]/2. 
  *   Use the TAIV interrupt vector generator.
  *   TimerA3 use ACLK that enabled in the LPM3 mode.
  *********************************************/
	

static uint8_t  rxTimerCnt;
extern uint8_t  gsd_rx_timer_event;

void rxTimerInit(void)
{
	
//Init timer in continuous mode sourced by ACLK
	Timer_A_clearTimerInterrupt(TIMER_A3_BASE);

    Timer_A_initContinuousModeParam param = {0};
    param.clockSource = TIMER_A_CLOCKSOURCE_ACLK;
    param.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    param.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_ENABLE;
    param.timerClear = TIMER_A_DO_CLEAR;
    param.startTimer = false;
    Timer_A_initContinuousMode(TIMER_A3_BASE, &param);
    rxTimerCnt = 1;

}

void rxTimerStart(void)
{
    Timer_A_startCounter(TIMER_A3_BASE, TIMER_A_CONTINUOUS_MODE);
}

void rxTimerSet(uint8_t cnt)
{
    rxTimerCnt = cnt;
}

void rxTimerStop(void)
{
    Timer_A_stop(TIMER_A3_BASE);
    Timer_A_clear(TIMER_A3_BASE);
     rxTimerCnt = 1;
}

//******************************************************************************
//
//This is the TIMER_A3 interrupt vector service routine.
//
//******************************************************************************
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMER3_A1_VECTOR
__interrupt
#elif defined(__GNUC__)
__attribute__((interrupt(TIMER3_A1_VECTOR)))
#endif
void TIMER3_A1_ISR (void)
{
    //Any access, read or write, of the TAIV register automatically resets the
    //highest "pending" interrupt flag
    LPM3_EXIT;
     	__no_operation();                         // For debugger
   switch ( __even_in_range(TA3IV,14) ){
        case  0: break;                          //No interrupt
        case  2: break;                          //CCR1 not used
        case  4: break;                          //CCR2 not used
        case  6: break;                          //CCR3 not used
        case  8: break;                          //CCR4 not used
        case 10: break;                          //CCR5 not used
        case 12: break;                          //CCR6 not used
        case 14:
            // Set event Tx Timeout Expired // overflow
            if (rxTimerCnt > 0) rxTimerCnt--;
	     if (rxTimerCnt <= 0) 
	     	{
    			Timer_A_stop(TIMER_A3_BASE);
	     		gsd_rx_timer_event = GSD_HAVE_EVENT;
	     	}
            break;
        default: break;
    }
}

