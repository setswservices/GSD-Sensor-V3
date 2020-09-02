/*
 * audio.c
 *
 *  Created on: Dec 2, 2019
 *      Author: dantonets
 */
#include "stdio.h"
#include "string.h"
#include "file.h"
#include "driverlib.h"
#include "gsd.h"

/********************************************************************************
I2C access to the LTC1662 based on:
Module		: I2C_SW
Author		: 05/04/2015, by KienLTb - https://kienltb.wordpress.com/
Description	: I2C software using bit-banging.

********************************************************************************/

/*-----------------------------------------------------------------------------*/
/* Local Macro definitions */
/*-----------------------------------------------------------------------------*/
 #define I2C_PORT		GPIO_PORT_P9

#define SCL        		GPIO_PIN1
#define SDA         	GPIO_PIN2

#define ACK 			0x00
#define NACK			0x01

#define TIME_DELAY 100
#define I2C_DELAY() __delay_cycles(TIME_DELAY)

extern gsd_setup_t gsd_setup;
extern uint16_t RAM_NoAUDIO_CNTS[];
extern uint16_t RAM_AUDIO_VAR0[];
extern uint16_t RAM_AUDIO_VAR1[];
extern uint16_t RAM_AUDIO_VAR01[];
extern uint16_t RAM_AUDIO_ADJ_VAR[];
extern uint16_t RAM_EVENTNo;
extern uint16_t RAM_AUDIO_CALB_VAR[];
extern uint16_t nRAM_VAR_LMT2, nRAM_VAR_LMT0;

#if GSD_FEATURE_ENABLED(DEBUGGING_MENU)
uint8_t audio_intr_flag = 0;
#endif // GSD_FEATURE_ENABLED(DEBUGGING_MENU)


/*--------------------------------------------------------------------------------
Function	: I2C_Writebit
Purpose		: Write bit to I2C bus
Parameters	: a bit need to write
Return		: None
--------------------------------------------------------------------------------*/
static void I2C_Writebit(uint16_t baseAddress, uint16_t scl, uint16_t sda, uint8_t bit)
{
	if(bit)
    		HWREG16(baseAddress + OFS_PAOUT) |= sda;  // SDA - > high 
	else
    		HWREG16(baseAddress + OFS_PAOUT) &= ~sda;  // SDA - > low 
	I2C_DELAY();
    	HWREG16(baseAddress + OFS_PAOUT) |= scl;  // SCL - > high 
	I2C_DELAY();
    	HWREG16(baseAddress + OFS_PAOUT) &= ~scl;  // SCL - > low 
}

#if 0
/*--------------------------------------------------------------------------------
Function	: I2C_Readbit
Purpose		: Read bit to I2C bus
Parameters	: None
Return		: unsigned char
--------------------------------------------------------------------------------*/
uint16_t I2C_Readbit(uint16_t baseAddress, uint16_t scl, uint16_t sda)
{
	uint16_t inputPinValue;
	//Let the slave driver data
    	HWREG16(baseAddress + OFS_PADIR) &= ~sda;  // SDA -> input
    	
    	HWREG16(baseAddress + OFS_PAOUT) |= scl;  // SCL - > high 
	I2C_DELAY();
       inputPinValue = HWREG16(baseAddress + OFS_PAIN) & (sda);
    	HWREG16(baseAddress + OFS_PAOUT) &= ~scl;  // SCL - > low 
    	HWREG16(baseAddress + OFS_PADIR) |= sda;  // SDA -> output
	
	return (inputPinValue >0)  1 : 0;

}
#endif

/*--------------------------------------------------------------------------------
Function	: I2C_WriteByte
Purpose		: Write a Byte to I2C bus
Parameters	: unsigned char Data
Return		: None
--------------------------------------------------------------------------------*/
static void I2C_WriteByte(uint16_t baseAddress, uint16_t scl, uint16_t sda, unsigned char Data)
{
  	unsigned char nBit;

	for(nBit = 0; nBit < 8; nBit++)
	{
		I2C_Writebit(baseAddress, scl, sda, ((Data & 0x80) != 0));
		Data <<= 1;
	}
//	I2C_Readbit(baseAddress, scl, sda); // Wait ACK/NACK
}

/*--------------------------------------------------------------------------------
Function	: I2C_WriteData
Purpose		: Write n Byte to I2C bus
Parameters	: Data 			- Pointer to Data need to write
			  DevideAddr	- Devide Address
			  Register		- Register Address
			  nLength		- Number of Byte need to write
Return		: None
--------------------------------------------------------------------------------*/
void LTC1662Setup(uint8_t *Data)
{

	uint16_t baseAddress9 =  __MSP430_BASEADDRESS_PORT9_R__;
	uint16_t baseAddress2 =  __MSP430_BASEADDRESS_PORT2_R__;
	uint16_t SCL_Pin = SCL;
	uint16_t SDA_Pin = SDA;
	uint16_t CS_Pin = GPIO_PIN2;
	
	
    // Shift by 8 if port is even (upper 8-bits)
    if((GPIO_PORT_P9 & 1) ^ 1) {
        SCL_Pin <<= 8;
	 SDA_Pin <<=8;	
    }

    if((GPIO_PORT_P2 & 1) ^ 1) {
        CS_Pin <<= 8;
    }


    HWREG16(baseAddress2 + OFS_PAOUT) &= ~CS_Pin;	  // CS -> low

    I2C_WriteByte(baseAddress9, SCL_Pin, SDA_Pin, Data[0]);  // First 8 bits is write
    I2C_WriteByte(baseAddress9, SCL_Pin, SDA_Pin, Data[1]);  // Second 8 bits is write

    HWREG16(baseAddress2 + OFS_PAOUT) |= CS_Pin;	  // CS -> high
}




void audioON(void)
{
/*
              BIC.B  #AUDIO_PWR_PIN,&AUDIO_OPORT  ;PWR ON=LO                       
              DLY 100ms
*/
	       GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN3);   
		DelayMS(50);
}

void audioOFF(void)
{
     GPIO_setAsOutputPin(    		GPIO_PORT_P1, GPIO_PIN3);  // Audio_ON
     GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN3);   
     GPIO_clearInterrupt( GPIO_PORT_P1, GPIO_PIN0);

}

void audioSetThs(void)
{
	uint8_t d[2];
	d[0] = (((gsd_setup.RAMn_DA01<<2) &0xFF00) >>8);
	d[0] |= 0x90;
	d[1] = ((gsd_setup.RAMn_DA01<<2) &0x00FF);
//	vPrintString("\t->DAC=["); vPrintString(psUInt8HexToString(d[0], prt_buf));
//	vPrintString(", "); vPrintString(psUInt8HexToString(d[1], prt_buf));
//	vPrintString("]"); vPrintEOL();
	LTC1662Setup(d);
#if 0
// Setup for old HW
	d[0] = 0x98;
//	d[0] = 0x18;
	d[1] = 0x80;
// Setup for new HW
//	d[0] = 0x98;
//	d[1] = 0x00;
	LTC1662Setup(d);
	DelayMS(200);
//	LTC1662Setup(d);
#endif //0
//	d[0] = 0xD0;
//	d[1] = 0x00;
	d[0] = (((gsd_setup.RAMn_DA01<<2) &0xFF00) >>8);
	d[0] |= 0xF0;
	d[1] = ((gsd_setup.RAMn_DA01<<2) &0x00FF);
	LTC1662Setup(d);
//	printf("Not implemented (see R10/R11)\r\n");
}

void adcInit(void)
{
    //Initialize the ADC12B Module
    /*
    * Base address of ADC12B Module
    * Use internal ADC12B bit as sample/hold signal to start conversion
    * USE MODOSC 5MHZ Digital Oscillator as clock source
    * Use default clock divider/pre-divider of 1
    * Not use internal channel
    */
   	ADC12_B_initParam initParam = {0};
    initParam.sampleHoldSignalSourceSelect = ADC12_B_SAMPLEHOLDSOURCE_SC;
//    initParam.clockSourceSelect = ADC12_B_CLOCKSOURCE_ADC12OSC;
    initParam.clockSourceSelect = ADC12_B_CLOCKSOURCE_MCLK;
    initParam.clockSourceDivider = ADC12_B_CLOCKDIVIDER_1;
    initParam.clockSourcePredivider = ADC12_B_CLOCKPREDIVIDER__1;
    initParam.internalChannelMap = ADC12_B_NOINTCH;
    ADC12_B_init(ADC12_B_BASE, &initParam);

    //Enable the ADC12B module
    ADC12_B_enable(ADC12_B_BASE);

//    ADC12_B_setResolution(ADC12_B_BASE, ADC12_B_RESOLUTION_10BIT);
    ADC12_B_setResolution(ADC12_B_BASE, ADC12_B_RESOLUTION_8BIT);

    /*
    * Base address of ADC12B Module
    * For memory buffers 0-7 sample/hold for 64 clock cycles
    * For memory buffers 8-15 sample/hold for 4 clock cycles (default)
    * Disable Multiple Sampling
    */
    ADC12_B_setupSamplingTimer(ADC12_B_BASE,
      ADC12_B_CYCLEHOLD_8_CYCLES, /* -- 5.8us per sample with output to TP11, or 108ms per 16K samples */
/*      ADC12_B_CYCLEHOLD_16_CYCLES,     -- 7.25us per sample with output to TP11, or 118ms per 16K samples */
      ADC12_B_CYCLEHOLD_4_CYCLES,
      ADC12_B_MULTIPLESAMPLESDISABLE);

//Configure Memory Buffer
    /*
    * Base address of the ADC12B Module
    * Configure memory buffer 0
    * Map input A8 to memory buffer 0
    * Vref+ = AVcc
    * Vref- = AVss
    * Memory buffer 0 is not the end of a sequence
    */
    ADC12_B_configureMemoryParam configureMemoryParam = {0};
    configureMemoryParam.memoryBufferControlIndex = ADC12_B_MEMORY_0;
    configureMemoryParam.inputSourceSelect = ADC12_B_INPUT_A8;
    configureMemoryParam.refVoltageSourceSelect = ADC12_B_VREFPOS_AVCC_VREFNEG_VSS;
    configureMemoryParam.endOfSequence = ADC12_B_NOTENDOFSEQUENCE;
    configureMemoryParam.windowComparatorSelect = ADC12_B_WINDOW_COMPARATOR_DISABLE;
    configureMemoryParam.differentialModeSelect = ADC12_B_DIFFERENTIAL_MODE_DISABLE;
    ADC12_B_configureMemory(ADC12_B_BASE, &configureMemoryParam);


    ADC12_B_clearInterrupt(ADC12_B_BASE, 0, ADC12_B_IFG0 );
    //Enable memory buffer 0 interrupt
//    ADC12_B_enableInterrupt(ADC12_B_BASE, ADC12_B_IE0, 0, 0);

	struct Timer_A_initContinuousModeParam initCntParam = {0};
	initCntParam.clockSource = TIMER_A_CLOCKSOURCE_EXTERNAL_TXCLK;
	initCntParam.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
	initCntParam.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE;  // 
    	initCntParam.timerClear = TIMER_A_DO_CLEAR;
    	initCntParam.startTimer = false;
	Timer_A_initContinuousMode(TIMER_A0_BASE, &initCntParam);		

    	struct Timer_A_initUpModeParam initUpParam = {0};
	initUpParam.clockSource = TIMER_A_CLOCKSOURCE_ACLK;
	initUpParam.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
	initUpParam.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_ENABLE;  
	initUpParam.captureCompareInterruptEnable_CCR0_CCIE = TIMER_A_CCIE_CCR0_INTERRUPT_DISABLE;
	initUpParam.timerClear = TIMER_A_DO_CLEAR;
/*              MOV    #7340,&CCA1R0       ;224ms WITH 32KHz ACLK  */
	initUpParam.timerPeriod = 7340; 
    	initUpParam.startTimer = false;
	Timer_A_initUpMode(TIMER_A1_BASE, &initUpParam);		
	
	
}
extern void adcCopy2FRAM2(void);
extern void ANALYZE_AUDIO_EVENT_FINAL(void);

void adcCopy2FRAM(unsigned long addr, uint16_t n)
{
	uint16_t baseAddress = __MSP430_BASEADDRESS_ADC12_B__;
	uint16_t baseAddress9 =  __MSP430_BASEADDRESS_PORT9_R__;
	uint16_t TP11_Pin = GPIO_PIN4;
	uint16_t idx =0;
	uint8_t data;

    while(1) {	
    	HWREG8(baseAddress + OFS_ADC12CTL0_L) |= ADC12ENC + ADC12SC;

    	while(HWREG8(baseAddress + OFS_ADC12CTL1_L) & ADC12BUSY);
	data = HWREG16(baseAddress + (OFS_ADC12MEM0 + ADC12_B_MEMORY_0))&0xFF;	
	__data20_write_char((unsigned long)(0x10000 + idx), data);
	idx++;
	if (idx >= NoAUDIO_SAMPLES_PER_CHANK) idx =0;
//    	HWREG16(baseAddress9 + OFS_PAOUT) ^= TP11_Pin;
    }
	
}

#if 0
void analyze_audio_event_final_c(void) 
{
	// Call capturing here..
	// And after that :
	
	NoAUDIO_CNT0 += NoAUDIO_CNT1;
	NoAUDIO_CNT1 = NoAUDIO_CNT2 + NoAUDIO_CNT3;
	NoAUDIO_CNT2 = NoAUDIO_CNT4 + NoAUDIO_CNT5;
// Sum = NoAUDIO_CNT0 + NoAUDIO_CNT1 + NoAUDIO_CNT2; 	
	NoAUDIO_CNT3 = 0;
	NoAUDIO_CNT4 = 0;
	NoAUDIO_CNT5 = 0;

	
	
}
#endif
/************************************************************
 * FUNCTION NAME: adcCapture                                                 		*
 *                                                                           				*
 *   Regs Modified     : SP,SR,r11,r12,r13,r14,r15                           	*
 *   Regs Used         : SP,SR,r11,r12,r13,r14,r15                           		*
 *   Local Frame Size  : 0 Args + 0 Auto + 0 Save = 0 byte                   *
 *   [ADK] assembled 12/6/2019. We not need use stack 			*
 *                                             in adcCopy2Fram2					*
 ************************************************************/

void adcCapture(void)
{
	uint16_t baseAddress = __MSP430_BASEADDRESS_ADC12_B__;


	    //Reset the ENC bit to set the starting memory address and conversion mode
    //sequence
    HWREG8(baseAddress + OFS_ADC12CTL0_L) &= ~(ADC12ENC);
    //Reset the bits about to be set
    HWREG16(baseAddress + OFS_ADC12CTL3) &= ~(ADC12CSTARTADD_31);
    HWREG16(baseAddress + OFS_ADC12CTL1) &= ~(ADC12CONSEQ_3);

    HWREG16(baseAddress + OFS_ADC12CTL3) |= ADC12_B_MEMORY_0;
    HWREG16(baseAddress + OFS_ADC12CTL1) |= ADC12_B_SINGLECHANNEL;

     adcCopy2FRAM2();

//     adcCopy2FRAM(addr, 	NoAUDIO_SAMPLES_PER_CHANK);

}

void audioInit(void)
{
	initAudioPort();
	audioON();
	audioSetThs();
	adcInit();
//	audio_int_enable();
}

void audioStart(void)
{
	ADC12_B_enable(ADC12_B_BASE);
	audio_int_enable();
}

void audioHandleAudioEvent(void)
{
	audio_int_disable();
	adcCapture();
	ADC12_B_disable(ADC12_B_BASE);
	ANALYZE_AUDIO_EVENT_FINAL();
}

void DSPLY_CNTRS(void)
{
	vPrintEOL();
#if xCHK_MULTIPLE_SHOTS
	vPrintString("# CNTRS (3 @ 224ms,4 @ 224ms=1.6 SECS):");
#else
	vPrintString("# CNTRS (3 @ 224ms,2 @ 224ms=1.1 SECS): 0x");
	vPrintString(psUInt16HexToString(RAM_NoAUDIO_CNTS[0], prt_buf)); vPrintString(", 0x");
	vPrintString(psUInt16HexToString(RAM_NoAUDIO_CNTS[1], prt_buf)); vPrintString(", 0x");
	vPrintString(psUInt16HexToString(RAM_NoAUDIO_CNTS[2], prt_buf)); vPrintString(" / 0x");
	vPrintString(psUInt16HexToString(RAM_NoAUDIO_CNTS[3], prt_buf)); vPrintString(", 0x");
	vPrintString(psUInt16HexToString(RAM_NoAUDIO_CNTS[4], prt_buf)); vPrintString("= 0x");
	vPrintString(psUInt16HexToString(RAM_NoAUDIO_CNTS[9], prt_buf)); vPrintString("\r\n");

#endif // xCHK_MULTIPLE_SHOTS
#if 0

			vPrintString("cntrs=[0x");
			vPrintString(psUInt16HexToString(RAM_NoAUDIO_CNTS[0], prt_buf)); vPrintString(", 0x");
			vPrintString(psUInt16HexToString(RAM_NoAUDIO_CNTS[1], prt_buf)); vPrintString(", 0x");
			vPrintString(psUInt16HexToString(RAM_NoAUDIO_CNTS[2], prt_buf)); vPrintString(", 0x");
			vPrintString(psUInt16HexToString(RAM_NoAUDIO_CNTS[3], prt_buf)); vPrintString(", 0x");
			vPrintString(psUInt16HexToString(RAM_NoAUDIO_CNTS[4], prt_buf)); vPrintString(", 0x");
			vPrintString(psUInt16HexToString(RAM_NoAUDIO_CNTS[5], prt_buf)); vPrintString("], sum=0x");
			vPrintString(psUInt16HexToString(RAM_NoAUDIO_CNTS[9], prt_buf)); vPrintString("\r\n");
#endif			
}

void DSPLY_CHAR(uint8_t ch)
{
	vPrintChar(ch);
}

void DSPLY_VARIANCE(void)
{
	vPrintString("\tVAR0=[0x");
	vPrintString(psUInt16HexToString(RAM_AUDIO_VAR0[0], prt_buf)); vPrintString(", 0x");
	vPrintString(psUInt16HexToString(RAM_AUDIO_VAR0[1], prt_buf)); 
	vPrintString("]"); vPrintEOL();
	vPrintString("\tVAR1=[0x");
	vPrintString(psUInt16HexToString(RAM_AUDIO_VAR1[0], prt_buf)); vPrintString(", 0x");
	vPrintString(psUInt16HexToString(RAM_AUDIO_VAR1[1], prt_buf)); 
	vPrintString("]"); vPrintEOL();
	vPrintString("\tVAR01=[0x");
	vPrintString(psUInt16HexToString(RAM_AUDIO_VAR01[0], prt_buf)); vPrintString(", 0x");
	vPrintString(psUInt16HexToString(RAM_AUDIO_VAR01[1], prt_buf)); 
	vPrintString("]"); vPrintEOL();
}

void DSPLY_ADJ_VAR(void)
{
	vPrintString("\tADJ_VAR=[0x");
	vPrintString(psUInt16HexToString(RAM_AUDIO_ADJ_VAR[0], prt_buf)); vPrintString(", 0x");
	vPrintString(psUInt16HexToString(RAM_AUDIO_ADJ_VAR[1], prt_buf)); 
	vPrintString("]"); vPrintEOL();
}

void DSPLY_EVENTNo(void)
{
	vPrintString("\tEVENT #=");
	vPrintString(psUInt16ToString(RAM_EVENTNo, prt_buf));
	vPrintEOL();
}

void DSPLY_VAR_LMT(void)
{
	vPrintString("\tVAR ALRM LMT=[0x");
	vPrintString(psUInt16HexToString(gsd_setup.RAMn_VAR_LMT0, prt_buf)); vPrintString(", 0x");
	vPrintString(psUInt16HexToString(gsd_setup.RAMn_VAR_LMT2, prt_buf)); vPrintString("]");
	vPrintEOL();
}

void DSPLY_MSG_ABOVE(void)
{
	vPrintString("\t --ALARM--"); vPrintEOL();
}
void DSPLY_MSG_BELOW(void)
{
	vPrintString("\t --REJECT--"); vPrintEOL();
}

void DSPLY_NEWMAX_VAR(void)
{
	vPrintString("CALB: NEW/MAX VAR=[0x");
	vPrintString(psUInt16HexToString(RAM_AUDIO_CALB_VAR[0], prt_buf)); vPrintString(", 0x");
	vPrintString(psUInt16HexToString(RAM_AUDIO_CALB_VAR[1], prt_buf)); vPrintString("]");
	vPrintEOL();
}
void SAVE_VAR_LMT(void)
{
	gsd_setup.RAMn_VAR_LMT0 = nRAM_VAR_LMT0;
	gsd_setup.RAMn_VAR_LMT2 = nRAM_VAR_LMT2;
	save_setup();
}


// Call from ASM routine(s) 
uint8_t getRAMn_DEBUG(void)
{
	return gsd_setup.RAMn_DEBUG;
}
uint8_t getRAMn_MODE(void)
{
	return gsd_setup.RAMn_MODE;
}
uint8_t getRAMn_SF(void)
{
	return gsd_setup.RAMn_ALARM_SF;
}
uint16_t getRAMn_VAR_LMT2(void)
{
	return gsd_setup.RAMn_VAR_LMT2;
}
uint16_t getRAMn_VAR_LMT0(void)
{
	return gsd_setup.RAMn_VAR_LMT0;
}

#if GSD_FEATURE_ENABLED(DEBUGGING_MENU)

int menuAudioPwr(void *menu, void *item, void *args)
{
	int c;
	initAudioPort();
	vPrintString("Enter '1' for power ON, any other - power OFF .. \r\n");

	while(1) 
	{
		delay_ms(2);

		if ((c = uart_getchar()) == '1') 
		{
			vPrintString("\tAudio power ON\r\n");
			audioON();
			break;
		}else
		if ( c != 0)  {
			vPrintString("\tAudio power OFF\r\n");
			audioOFF();
			break;
		}
	}
	return 0;
}

int menuAudioWriteThs(void *menu, void *item, void *args)
{
	uint8_t d[2];
// Setup for old HW
	d[0] = 0x98;
	d[1] = 0x40;
// Setup for new HW
//	d[0] = 0x98;
//	d[1] = 0x00;
	LTC1662Setup(d);
//	printf("Not implemented (see R10/R11)\r\n");
	return 0;
}

int menuAudioEvent(void *menu, void *item, void *args)
{
	int cnt = 0;
	Calendar currentTime;

	vPrintString("Hit ESC if no Audio event occured ..\r\n");
	audio_intr_flag = 0;
	adcInit();

	audio_int_enable();
	while(1) 
	{
		delay_ms(2);
		if (audio_intr_flag) 
		{
			audio_int_disable();
			adcCapture();

			ADC12_B_disable(ADC12_B_BASE);
/*			
			vPrintString("=>cnt=[0x");
			vPrintString(psUInt16HexToString(RAM_NoAUDIO_CNTS[0], prt_buf)); vPrintString(", 0x");
			vPrintString(psUInt16HexToString(RAM_NoAUDIO_CNTS[1], prt_buf)); vPrintString(", 0x");
			vPrintString(psUInt16HexToString(RAM_NoAUDIO_CNTS[2], prt_buf)); vPrintString(", 0x");
			vPrintString(psUInt16HexToString(RAM_NoAUDIO_CNTS[3], prt_buf)); vPrintString(", 0x");
			vPrintString(psUInt16HexToString(RAM_NoAUDIO_CNTS[4], prt_buf)); vPrintString(", 0x");
			vPrintString(psUInt16HexToString(RAM_NoAUDIO_CNTS[5], prt_buf)); vPrintString("], sum=0x");
			vPrintString(psUInt16HexToString(RAM_NoAUDIO_CNTS[9], prt_buf)); vPrintString("\r\n");
*/			
			ANALYZE_AUDIO_EVENT_FINAL();


			
			break;
		}
		if (uart_getchar() == CHAR_ESC) 
		{
			audio_int_disable();
			vPrintString("\tNo Audio..\r\n");
			break;
		}
		cnt++;
		if (cnt >= 500) {
			currentTime = RTC_C_getCalendarTime(RTC_C_BASE);
			printDateTime();
			cnt = 0;
		}		
	}
	return 0;
}

int menuAudioCapture(void *menu, void *item, void *args)
{
//	int cnt = 0;
	
	adcInit();
	adcCapture();
#if 0	
//	printf("Hit ESC for stop capturing ..\r\n");

	while(1) 
	{
//		delay_ms(2);
		if (uart_getchar() == CHAR_ESC) 
		{
			printf("\tStoped ..\r\n");
			break;
		}
        //Enable/Start sampling and conversion
        /*
         * Base address of ADC12B Module
         * Start the conversion into memory buffer 0
         * Use the single-channel, single-conversion mode
         */
        ADC12_B_startConversion(ADC12_B_BASE, ADC12_B_MEMORY_0, ADC12_B_SINGLECHANNEL);

        __bis_SR_register(LPM0_bits + GIE);     // LPM0, ADC12_B_ISR will force exit
        __no_operation();                       // For debugger
		cnt++;
		if (cnt >= 5000) {
			printf(".");
			cnt = 0;
		}
	}
#endif	
	return 0;

}
#endif //GSD_FEATURE_ENABLED(DEBUGGING_MENU)

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=ADC12_VECTOR
__interrupt
#elif defined(__GNUC__)
__attribute__((interrupt(ADC12_VECTOR)))
#endif
void ADC12_ISR(void)
{
  switch(__even_in_range(ADC12IV,12))
  {
    case  0: break;                         // Vector  0:  No interrupt
    case  2: break;                         // Vector  2:  ADC12BMEMx Overflow
    case  4: break;                         // Vector  4:  Conversion time overflow
    case  6: break;                         // Vector  6:  ADC12BHI
    case  8: break;                         // Vector  8:  ADC12BLO
    case 10: break;                         // Vector 10:  ADC12BIN
    case 12:                                // Vector 12:  ADC12BMEM0 Interrupt
#if 0
      if (ADC12_B_getResults(ADC12_B_BASE, ADC12_B_MEMORY_0) >= 0x7ff)
      {
    		//Set P1.0 LED on
    		/*

    		* Select Port 1
    		* Set Pin 0 to output high.
    		*/
    		GPIO_setOutputHighOnPin(
    			GPIO_PORT_P1,
    			GPIO_PIN0
    		);
      }
      else
      {
  		//Set P1.0 LED off
  		/*

  		* Select Port 1
  		* Set Pin 0 to output high.
  		*/
  		GPIO_setOutputLowOnPin(
  			GPIO_PORT_P1,
  			GPIO_PIN0
  		);
      }
#endif
	ADC12_B_getResults(ADC12_B_BASE, ADC12_B_MEMORY_0);
	GPIO_toggleOutputOnPin (    		GPIO_PORT_P9, GPIO_PIN4);  // TP11	
    	 __bic_SR_register_on_exit(LPM0_bits); // Exit active CPU
      break;                                // Clear CPUOFF bit from 0(SR)
    case 14: break;                         // Vector 14:  ADC12BMEM1
    case 16: break;                         // Vector 16:  ADC12BMEM2
    case 18: break;                         // Vector 18:  ADC12BMEM3
    case 20: break;                         // Vector 20:  ADC12BMEM4
    case 22: break;                         // Vector 22:  ADC12BMEM5
    case 24: break;                         // Vector 24:  ADC12BMEM6
    case 26: break;                         // Vector 26:  ADC12BMEM7
    case 28: break;                         // Vector 28:  ADC12BMEM8
    case 30: break;                         // Vector 30:  ADC12BMEM9
    case 32: break;                         // Vector 32:  ADC12BMEM10
    case 34: break;                         // Vector 34:  ADC12BMEM11
    case 36: break;                         // Vector 36:  ADC12BMEM12
    case 38: break;                         // Vector 38:  ADC12BMEM13
    case 40: break;                         // Vector 40:  ADC12BMEM14
    case 42: break;                         // Vector 42:  ADC12BMEM15
    case 44: break;                         // Vector 44:  ADC12BMEM16
    case 46: break;                         // Vector 46:  ADC12BMEM17
    case 48: break;                         // Vector 48:  ADC12BMEM18
    case 50: break;                         // Vector 50:  ADC12BMEM19
    case 52: break;                         // Vector 52:  ADC12BMEM20
    case 54: break;                         // Vector 54:  ADC12BMEM21
    case 56: break;                         // Vector 56:  ADC12BMEM22
    case 58: break;                         // Vector 58:  ADC12BMEM23
    case 60: break;                         // Vector 60:  ADC12BMEM24
    case 62: break;                         // Vector 62:  ADC12BMEM25
    case 64: break;                         // Vector 64:  ADC12BMEM26
    case 66: break;                         // Vector 66:  ADC12BMEM27
    case 68: break;                         // Vector 68:  ADC12BMEM28
    case 70: break;                         // Vector 70:  ADC12BMEM29
    case 72: break;                         // Vector 72:  ADC12BMEM30
    case 74: break;                         // Vector 74:  ADC12BMEM31
    case 76: break;                         // Vector 76:  ADC12BRDY
    default: break;
  }
}

//******************************************************************************
//
//This is the TIMER0_A3 interrupt vector service routine.
//
//******************************************************************************
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMER1_A1_VECTOR
__interrupt
#elif defined(__GNUC__)
__attribute__((interrupt(TIMER1_A1_VECTOR)))
#endif
void TIMER1_A1_ISR (void)
{
    //Any access, read or write, of the TAIV register automatically resets the
    //highest "pending" interrupt flag
    switch ( __even_in_range(TA1IV,14) ){
        case  0: break;                          //No interrupt
        case  2: break;                          //CCR1 not used
        case  4: break;                          //CCR2 not used
        case  6: break;                          //CCR3 not used
        case  8: break;                          //CCR4 not used
        case 10: break;                          //CCR5 not used
        case 12: break;                          //CCR6 not used
        case 14:                			   // overflow
            Timer_A_clearTimerInterrupt(TIMER_A1_BASE);
            GPIO_toggleOutputOnPin( GPIO_PORT_P9, GPIO_PIN4);
            break;
        default: break;
    }
}

