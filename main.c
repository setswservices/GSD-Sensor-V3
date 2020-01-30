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
  *  Audio Event. 
  *-------------------------------------------*/
volatile uint8_t  gsd_audio_event = GSD_NO_EVENT;
/*********************************************/
/* --------------------------------------------
  *  UART Event. Used for switch FW to the debug mode
  *-------------------------------------------*/
volatile uint8_t  gsd_uart_event = GSD_NO_EVENT;
/*********************************************/
/* --------------------------------------------
  *  RTC Event. Used for switch FW to the LPM4
  *-------------------------------------------*/
volatile uint8_t  gsd_rtc_event = GSD_NO_EVENT;
/*********************************************/
/* --------------------------------------------
  *  Tx Done Event.
  *-------------------------------------------*/
volatile uint8_t  gsd_tx_done_event = GSD_NO_EVENT;
/*********************************************/
/* --------------------------------------------
  *  Rx Timeout Expired Event.
  *-------------------------------------------*/
volatile uint8_t  gsd_rx_timer_event = GSD_NO_EVENT;
/*********************************************/
/* --------------------------------------------
  *  Got Rx Packet Event.
  *-------------------------------------------*/
volatile uint8_t  gsd_rx_packet_event = GSD_NO_EVENT;
/*********************************************/


uint8_t  gsd_fw_startup = 0x00; 
uint8_t  gsd_tx_packet_type = 0x00;
/*********************************************
  *   GSD setup
  *   This is a RAM copy for setup from INFO-D 
  *********************************************/
gsd_setup_t	gsd_setup;
#if GSD_FEATURE_ENABLED(HBEAT_ACK_PACKET)
static uint8_t  gsd_try_ack_packets_cnt = 0;
#endif //GSD_FEATURE_ENABLED(HBEAT_ACK_PACKET)

/*********************************************
  *   Audio analysis variables 
  *   
  *********************************************/
uint16_t RAM_NoAUDIO_CNTS[10];   // [9] = RAM_AUDIO         .space 01H + RAM_ALRM_FLG      .space 01H
//uint16_t RAM_EVENTNo;
uint16_t RAM_BKGD_VAR[2];
// RAM_ALRM_FLG      .space 01H
uint8_t RAM_ALRM_FLG;

// RAM_AUDIO_VAR0     .space 04H
extern uint16_t RAM_AUDIO_VAR0[];
// RAM_AUDIO_VAR1     .space 04H
extern uint16_t RAM_AUDIO_VAR1[];
//RAM_AUDIO_VAR01    .space 04H   ;KEEP ORDER
extern uint16_t RAM_AUDIO_VAR01[]; 
extern uint16_t RAM_AUDIO_ADJ_VAR[];
extern uint16_t RAM_EVENTNo;
extern uint16_t RAM_NoALRMS;

#pragma PERSISTENT(audio_samples_0)
#pragma location = 0x10000 // memory address in FRAM2
uint8_t audio_samples_0[NoAUDIO_SAMPLES_PER_CHANK];	
#pragma PERSISTENT(audio_samples_1)
#pragma location = 0x14000 // memory address in FRAM2
uint8_t audio_samples_1[NoAUDIO_SAMPLES_PER_CHANK];	
#pragma PERSISTENT(audio_samples_2)
#pragma location = 0x18000 // memory address in FRAM2
uint8_t audio_samples_2[NoAUDIO_SAMPLES_PER_CHANK];	
#pragma PERSISTENT(audio_samples_3);
#pragma location = 0x1c000 // memory address in FRAM2
uint8_t audio_samples_3[NoAUDIO_SAMPLES_PER_CHANK];	
#pragma PERSISTENT(audio_samples_4);
#pragma location = 0x20000 // memory address in FRAM2
uint8_t audio_samples_4[NoAUDIO_SAMPLES_PER_CHANK];	
#pragma PERSISTENT(audio_samples_5);
#pragma location = 0xBFC0 // memory address in FRAM
uint8_t audio_samples_5[NoAUDIO_SAMPLES_PER_CHANK];	


static void EnterLPM3(void);
static void EnterLPM35(void);
static void WakeUpLPM35(void);
#if GSD_FEATURE_ENABLED(AUDIO_FRAM_CLEANUP)
static void	Fram2Clear(uint8_t *addr, uint16_t size);
#endif //GSD_FEATURE_ENABLED(AUDIO_FRAM_CLEANUP)


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
	pwr_led0_red();
	initPortRF();
	audioOFF();
	gsd_audio_event = GSD_NO_EVENT;
	__enable_interrupt();
	rtc_init();
	rxTimerInit();

#if GSD_FEATURE_ENABLED(DEBUG_BEFORE_EEPROM_INIT)
	memset(&gsd_setup, 0x0, sizeof(gsd_setup_t));
	gsd_setup.RAMn_LED_ONFLG = 0xFF;
	gsd_setup.RAMn_DEBUG = 0xFF;
	gsd_setup.RAMn_TAGID = 0x0010;
	gsd_setup.RAMn_MODE = 0x01;
	gsd_setup.RAMn_VAR_LMT2 = 0x0018;	// MSW
	gsd_setup.RAMn_VAR_LMT0 = 0;			// LSW
	gsd_setup.RAMn_ROOMNo = 0x04D2;		// 1234
	gsd_setup.RAMn_FLOORNo = 'A';
#else
	init_setup();
#endif //GSD_FEATURE_ENABLED(DEBUG_BEFORE_EEPROM_INIT)

#if GSD_FEATURE_ENABLED(AUDIO_FRAM_CLEANUP)
	Fram2Clear(audio_samples_0, sizeof(audio_samples_0));
	Fram2Clear(audio_samples_1, sizeof(audio_samples_1));
	Fram2Clear(audio_samples_2, sizeof(audio_samples_2));
	Fram2Clear(audio_samples_3, sizeof(audio_samples_3));
	Fram2Clear(audio_samples_4, sizeof(audio_samples_4));
#endif // GSD_FEATURE_ENABLED(AUDIO_FRAM_CLEANUP)

	RAM_NoAUDIO_CNTS[ 9] = 0;
	RAM_EVENTNo = 0;
	RAM_NoALRMS = 0;
	RAM_BKGD_VAR[0] = 0;
	RAM_BKGD_VAR[1] = 0;

#if GSD_FEATURE_ENABLED(DEBUG_SERIAL_PORT)
	debugPortInit();
	vPrintEOL();
	vPrintString(FIRMWARE_VERSION_STRING);
	vPrintString(" Build ");
	vPrintString(Build_Date);
	vPrintString(" ");
	vPrintString(Build_Time);
	vPrintEOL();
	vPrintString("Start from ");
	vPrintString((gsd_fw_startup == GSD_WAKEUP)? "wake up" : "reset");
	vPrintEOL();
#endif GSD_FEATURE_ENABLED(DEBUG_SERIAL_PORT)

/*********************************************
  *   Start Audio system 
  *   
  *********************************************/
	audioInit();
    __no_operation();                         // For debugger

// [ADK]  02/01/2020  becouse we always send the HB packet..   audioStart();
/*********************************************
  *   Send HB_ON package
  *   The FW expect a response during the Rx Timeout.
  *********************************************/
	gsd_tx_packet_type = HBEAT_wAUDIO_PWR_ON_3;
	nRF905_sndRstHB();

    	while (1)
    	{
		if (gsd_audio_event) {
			vPrintString("\tAudio event");
			vPrintEOL();
			audioHandleAudioEvent();	
			gsd_tx_packet_type = RAM_ALRM_FLG;
			nRF905_sndRstHB();
			if (gsd_tx_packet_type == 1 /* ALARM */)
				flash_led0_red();
			else
				flash_led1_green();
			gsd_audio_event = GSD_NO_EVENT;
			
#if GSD_FEATURE_ENABLED(HBEAT_ACK_PACKET)
			gsd_try_ack_packets_cnt = 0;
#endif //GSD_FEATURE_ENABLED(HBEAT_ACK_PACKET)
			continue;
		}
    		if (gsd_tx_done_event) {
			vPrintString("\tTx Done event");
			vPrintEOL();
			nRF905_TxDone();
			gsd_tx_done_event = GSD_NO_EVENT;
			if ((gsd_tx_packet_type == HBEAT_wAUDIO_PWR_ON_3) || (gsd_tx_packet_type == HBEAT_wAUDIO_PWR_OFF_3))
			{
				nRF905_RxStart();
				rxTimerSet(0x6);
				rxTimerStart();
			}else{
				if (getSendWfFlag() == 0x01)  // we done with send a 1st packet with the WF data ..
					nRF905_send_2nd();
				else
				if (getSendWfFlag() == 0x02)  // we done with send a 2nd packet with the WF data ..
					nRF905_send_3rd();
				else
				if (getSendWfFlag() == 0x03)  {// we done with send a 3rd packet with the WF data ..
#if GSD_FEATURE_ENABLED(HBEAT_ACK_PACKET)
					if ((gsd_setup.RAMn_MODE&0x08) == 0x08 && gsd_tx_packet_type == 1 /* ALARM */) {
						// Try read the ACK packet ..
						nRF905_RxStart();
						rxTimerSet(0x6);
						rxTimerStart();
					}
#endif // GSD_FEATURE_ENABLED(HBEAT_ACK_PACKET)
#if GSD_FEATURE_ENABLED(DATA_PORT)
					if ((gsd_setup.RAMn_MODE&0x80) == 0x80) 
						vWfDataOut();
#endif //GSD_FEATURE_ENABLED(DATA_PORT)
#if GSD_FEATURE_ENABLED(HBEAT_ACK_PACKET)
					if ((gsd_setup.RAMn_MODE&0x08) == 0) 
#endif // GSD_FEATURE_ENABLED(HBEAT_ACK_PACKET)
					audioStart();
				}
			}
			continue;
    		}
		if (gsd_rx_packet_event)
		{
			vPrintString("\tGot Rx Packet event");
			vPrintEOL();
			gsd_rx_packet_event = GSD_NO_EVENT;
			rxTimerStop();
			nRF905_Rx();
			nRF905_SetRTC();
			nRF905_RxDone();
			nRF905_pwr_off();
			rx_led1_green();
			if (gsd_tx_packet_type == HBEAT_wAUDIO_PWR_OFF_3)
			{
				EnterLPM35();
    				__no_operation();                         // For debugger, 'll *never* reach this point
			}
			gsd_tx_packet_type = 0xFF;
			audioStart();
			continue;
		}
    		if (gsd_rx_timer_event) {
			vPrintString("\tRx Timer Expired event");
			vPrintEOL();
			gsd_rx_timer_event = GSD_NO_EVENT;
			rxTimerStop();
			nRF905_RxDone();
			nRF905_pwr_off();
			if (gsd_tx_packet_type == HBEAT_wAUDIO_PWR_ON_3)
			{
				vPrintString("\tGoing to deep sleep for 1 hour");
				vPrintEOL(); 				vPrintEOL();
				rtc_set_fake_time();
				nRF905_SetDtWakeUp(0x01, 0x00);
// Sleep 1 minute, for debugging				nRF905_SetDtWakeUp(0x00, 0x01);
				EnterLPM35();
    				__no_operation();                         // For debugger, 'll *never* reach this point
			}
			
#if GSD_FEATURE_ENABLED(HBEAT_ACK_PACKET)
			if ((gsd_setup.RAMn_MODE&0x08) == 0x08 && gsd_tx_packet_type == 1 /* ALARM */) {
				gsd_try_ack_packets_cnt++;
				if (GSD_MAX_TRY_ACK >= gsd_try_ack_packets_cnt) 
				{
					nRF905_sndRstHB();
					continue;
				}
			}
#endif //GSD_FEATURE_ENABLED(HBEAT_ACK_PACKET)
			
			audioStart();
			continue;
    		}
    		if (gsd_rtc_event) {
			vPrintString("\tRTC Alarm event");
			vPrintEOL();
			gsd_rtc_event = GSD_NO_EVENT;
			gsd_tx_packet_type = HBEAT_wAUDIO_PWR_OFF_3;
			audio_int_disable();
			audioOFF();
			nRF905_sndRstHB();
			continue;
    		}
    		if (gsd_uart_event) 
    		{
			int c;	
			gsd_uart_event = GSD_NO_EVENT;
			vPrintString("\tUART event"); 
			vPrintEOL();
			setup_enter();	
			gsd_uart_event = GSD_NO_EVENT;
    		}
		GPIO_setOutputHighOnPin(GPIO_PORT_P9, GPIO_PIN2);
		DelayMS(1);
		EnterLPM3();
    	}
	return 0;
}

//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/      Local functions             _/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

static void EnterLPM3(void)
{  

	__bis_SR_register(LPM3_bits | GIE);
    	__no_operation();                         // For debugger
}

static void EnterLPM35(void)
{  
#if GSD_FEATURE_ENABLED(DEBUG_SERIAL_PORT)
	debugPortDisable();
#endif // GSD_FEATURE_ENABLED(DEBUG_SERIAL_PORT)
	GPIO_setAsInputPin(    		GPIO_PORT_P2, GPIO_PIN2 + GPIO_PIN6 + GPIO_PIN7);
	audio_int_disable();
	audioOFF();

	GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN_ALL8);
	GPIO_setOutputHighOnPin (GPIO_PORT_P1, GPIO_PIN5);      // Turn-off EEPROM
	GPIO_setOutputHighOnPin (GPIO_PORT_P1, GPIO_PIN3);      // Turn-off Audio
	GPIO_setOutputHighOnPin (GPIO_PORT_P1, GPIO_PIN6);      // SDA_NFC -> high
    	GPIO_setOutputHighOnPin (GPIO_PORT_P1, GPIO_PIN4);      // RFID_Busy -> high
	GPIO_setOutputLowOnPin (GPIO_PORT_P1, GPIO_PIN0+GPIO_PIN1+GPIO_PIN2+GPIO_PIN7);

	GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN_ALL8);
	GPIO_setOutputLowOnPin (GPIO_PORT_P2, GPIO_PIN_ALL8);

	GPIO_setAsOutputPin(GPIO_PORT_PB, GPIO_PIN_ALL16);
	GPIO_setOutputLowOnPin (GPIO_PORT_PB, GPIO_PIN_ALL16);

	GPIO_setAsOutputPin(GPIO_PORT_PC, GPIO_PIN_ALL16);
	GPIO_setOutputLowOnPin (GPIO_PORT_PC, GPIO_PIN_ALL16);

	GPIO_setAsOutputPin(GPIO_PORT_PD, GPIO_PIN_ALL16);
	GPIO_setOutputLowOnPin (GPIO_PORT_PD, GPIO_PIN_ALL16);

	GPIO_setAsOutputPin(GPIO_PORT_PE, GPIO_PIN_ALL16);
	GPIO_setOutputLowOnPin (GPIO_PORT_PE, GPIO_PIN_ALL16);

//	GPIO_setOutputHighOnPin (GPIO_PORT_P1, GPIO_PIN5);      // Turn-off EEPROM
//	GPIO_setOutputHighOnPin (GPIO_PORT_P1, GPIO_PIN3);      // Turn-off Audio

	GPIO_setOutputHighOnPin (GPIO_PORT_P4, GPIO_PIN1);      // SDA_EE -> high
//	GPIO_setOutputHighOnPin (GPIO_PORT_P1, GPIO_PIN6);      // SDA_NFC -> high
//    GPIO_setOutputHighOnPin (GPIO_PORT_P1, GPIO_PIN4);      // RFID_Busy -> high
	GPIO_setOutputHighOnPin (GPIO_PORT_P2, GPIO_PIN6+GPIO_PIN7);      // Turn-off LEDs


	nRF905_SetWkUpRTC();


#if GSD_FEATURE_ENABLED(LPM4)
	PMM_turnOffRegulator();

	// Enter LPM3.5 mode with interrupts enabled. Note that this operation does  
	// not return. The LPM3.5 will exit through a RESET event, resulting in a  
	// re-start of the code.
	__bis_SR_register(LPM4_bits | GIE);
#else

	EnterLPM3();

	HWREG16(WDT_A_BASE + OFS_WDTCTL) |= WDTHOLD;  //Will reset MSP430,WDTIFG flag is set
#endif // GSD_FEATURE_ENABLED(LPM4)

}

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

#if GSD_FEATURE_ENABLED(AUDIO_FRAM_CLEANUP)
static void	Fram2Clear(uint8_t *addr, uint16_t size)
{
	uint16_t idx;
	uint8_t zero = 0xA5;
	
	for(idx=0; idx < size; idx++)
	__data20_write_char((unsigned long)(&addr[ idx]), zero);	
}
#endif // GSD_FEATURE_ENABLED(AUDIO_FRAM_CLEANUP)

