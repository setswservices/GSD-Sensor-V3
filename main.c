#include "stdio.h"
#include "string.h"
#include "file.h"
#include "driverlib.h"
#include "gsd.h"

//#include <msp430.h>
#if GSD_FEATURE_ENABLED(DEBUG_SERIAL_PORT)
	char prt_buf[32];
#endif //GSD_FEATURE_ENABLED(DEBUG_SERIAL_PORT)


const char Build_Date[] = __DATE__;
const char Build_Time[] = __TIME__;

/*********************************************
  *   GSD event flags
  *   All event awake the MSP430 from LPM3 
  *********************************************/
/* --------------------------------------------
  *  UART Event. Used for switch FW to the debug mode
  *-------------------------------------------*/
uint8_t  gsd_usrt_event = GSD_NO_EVENT;
 /*********************************************/
uint8_t  gsd_fw_startup = 0x00; 


static void WakeUpLPM35(void);


/**
 * main.c
 */
int main(void)
{
    WDT_A_hold(WDT_A_BASE); // stop watchdog
	// Determine whether we are coming out of an LPMx.5 or a regular RESET.
	if (SYSRSTIV == SYSRSTIV_LPM5WU) 
	{    // When woken up from LPM3.5, reinit
  		WakeUpLPM35();       	// LPMx.5 wakeup specific init code
  		gsd_fw_startup = GSD_WAKEUP;
//  		__enable_interrupt(); 	// The RTC interrupt should trigger now...
//  		while (1); 	// Forever loop after returning from RTC ISR.
  	}  else
  	{
  		gsd_fw_startup = GSD_RESET;
		
  	}

	initPortJ();
	// For check SMCLK on TP18
    	GPIO_setAsPeripheralModuleFunctionOutputPin(
       	GPIO_PORT_PJ,
       	GPIO_PIN0,
       	GPIO_PRIMARY_MODULE_FUNCTION
    	);
		// For check ACLK on TP16
    	GPIO_setAsPeripheralModuleFunctionOutputPin(
        	GPIO_PORT_PJ,
        	GPIO_PIN2,
        	GPIO_PRIMARY_MODULE_FUNCTION
    	);

    	//Set DCO frequency to 8 MHz
    	CS_setDCOFreq(CS_DCORSEL_0,CS_DCOFSEL_6);
    	//Set external clock frequency to 32.768 KHz
    	CS_setExternalClockSource(32768,0);
    	//Set ACLK=LFXT
    	CS_initClockSignal(CS_ACLK,CS_LFXTCLK_SELECT,CS_CLOCK_DIVIDER_1);
    	//Set SMCLK = DCO with frequency divider of 1
    	CS_initClockSignal(CS_SMCLK,CS_DCOCLK_SELECT,CS_CLOCK_DIVIDER_1);
    	//Set MCLK = DCO with frequency divider of 1
    	CS_initClockSignal(CS_MCLK,CS_DCOCLK_SELECT,CS_CLOCK_DIVIDER_1);
    	//Start XT1 with no time out
    	CS_turnOnLFXT(CS_LFXT_DRIVE_0);

    	/*
     	  * Disable the GPIO power-on default high-impedance mode to activate
     	  * previously configured port settings
     	  */
    	PMM_unlockLPM5();

	init_ports();
	initPortRF();

	return 0;
}

//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/      Local functions             _/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

static void WakeUpLPM35(void)
{
	RTC_C_disableInterrupt(RTC_C_BASE, (RTCOFIE + RTCTEVIE + RTCAIE + RTCRDYIE));
    	RTC_C_clearInterrupt(RTC_C_BASE, (RTC_C_TIME_EVENT_INTERRUPT + RTC_C_CLOCK_ALARM_INTERRUPT + RTC_C_CLOCK_READ_READY_INTERRUPT + RTC_C_OSCILLATOR_FAULT_INTERRUPT)); 
}
void DelayMS(uint16_t ms)
{
	while(ms--) {
		delay_ms(1);
	}
}

