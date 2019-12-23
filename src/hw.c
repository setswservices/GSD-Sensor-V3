/*
 * hw.c
 *
 *  Created on: Oct 15, 2019
 *      Author: dantonets
 */

#include "driverlib.h"
#include "gsd.h"

// [ADK] 11/13/2019   for debuuging ..
#define delay_ms(_x_) 	_delay_cycles((unsigned long)((8000 -1)*(_x_)))

extern uint8_t  gsd_audio_event;


void init_ports(void)
{
/* ========================
	PORT 1:
	P1.0			AUDIO_EVENT, external clock for TA0 (for count pulses), external pull-up
	P1.1			USB Detect, input, *not use in this version*
	P1.2			AUDIO_EVENT, external clock for TA0 (for count pulses) (same with P1.0)
	P1.3			Audio_On, output
	P1.4			RFID_Busy, input interrupt, external pull-up
	P1.5			EEPROM_ON, output
	P1.6			RFID SDA (UCB0SDA)
	P1.7			RFID SCL (UCB0SCL)

=========================*/	


/* ========================
	PORT 2:
	P2.0, P2.1	Debug UART (J1) controlled in EUSCI_A_UART module
	P2.2  		Audio CS, output, high
	P2.3			NC
	P2.4			NC
	P2.5			NC
	P2.6			LED Alarm Red, output, high
	P2.7			LED Reject Green, output, high
=========================*/	

	GPIO_setAsOutputPin(    		GPIO_PORT_P2, GPIO_PIN2 + GPIO_PIN6 + GPIO_PIN7);
	GPIO_setOutputHighOnPin (	GPIO_PORT_P2, GPIO_PIN2 + GPIO_PIN6 + GPIO_PIN7);

/* ========================
	PORT 3:
	P3.0			RF_SCK (UCB1CLK)
	P3.1			RF_MOSI (UCB1SIMO)
	P3.2			RF_MISO (UCB1SOMI)
	P3.3			??
	P3.4			Host_Tx (UCA1TXD), *not use in this version*
	P3.5			Host_Rx (UCA1RXD), *not use in this version*
	P3.6			RF_DataRdy, input interrupt, ** not use yet**
	P3.7			RF_CD, input interrupt, ** not use yet**

=========================*/	

/* ========================
	PORT 4:
	P4.0			SCL_EE (UCB1SCL)
	P4.1			SDA_EE (UCB1SDA)
	P4.2			NC
	P4.3			RF_On, output
=========================*/	

/* ========================
	PORT 5:
	P5.0			RF_TX_EN, output
	P5.1			RF_TRX_CE, output
	P5.2			RF_PWRUP, output
	P5.3			RF_CS (UCB1STE)

=========================*/	


/* ========================
	PORT 9:
	P9.0			Audio_Analog, input, ADC A8
	P9.1			Audio_SCL, output, bit-bang I2C
	P9.2			Audio_SDA, output, bit-bang I2C
	P9.4			TP11, output,  use for meagure the freq of ADC for Audio
=========================*/	

}

void initPortRF(void) 
{
	GPIO_setAsOutputPin(    		GPIO_PORT_P4, GPIO_PIN3);  // RF-on
       GPIO_setOutputLowOnPin(GPIO_PORT_P4, GPIO_PIN3);  // OFF regulator ..
//       GPIO_setOutputHighOnPin(GPIO_PORT_P4, GPIO_PIN3);  // ON regulator ..
	GPIO_setAsOutputPin(    		GPIO_PORT_P5, GPIO_PIN0 + GPIO_PIN1 + GPIO_PIN2  + GPIO_PIN3);  // RF_TX_EN, RF_TX_CE, RF_PWRUP, RF_CS
	GPIO_setOutputLowOnPin(    	GPIO_PORT_P5, GPIO_PIN0 + GPIO_PIN1 + GPIO_PIN2);  // RF_TX_EN, RF_TX_CE, RF_PWRUP
       GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN3);   // Set RF_CS low , we have just one SPI device ..

#if 0
	/*  -- If use SPI HW :
    	  * Select Port 3
    	  * Set Pin(s) 0,1, and 2 to input Primary Module Function, (UCB1CLK, UCB1SIMO, UCB1SOMI).
         */
    GPIO_setAsPeripheralModuleFunctionInputPin(
        GPIO_PORT_P3,
        GPIO_PIN0 + GPIO_PIN1 + GPIO_PIN2,
        GPIO_PRIMARY_MODULE_FUNCTION
    );
//     GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN3);

    /* Not need for 3PIN mode..  UCB1STE - RF_CS	 
    GPIO_setAsPeripheralModuleFunctionOutputPin(
        GPIO_PORT_P5,
        GPIO_PIN3,
        GPIO_SECONDARY_MODULE_FUNCTION
    );
*/    
#else
	/*  -- If use for bitbang, or test pins .. */ 

     GPIO_setAsOutputPin(    		GPIO_PORT_P3, GPIO_PIN0 + GPIO_PIN1 /* + GPIO_PIN2  + GPIO_PIN3 */);  // RF_SCK, RF_MOSI
     GPIO_setAsInputPin /*WithPullDownResistor */(GPIO_PORT_P3, GPIO_PIN2);   // RF_MISO 
     GPIO_setAsInputPin /*WithPullDownResistor */(GPIO_PORT_P3, GPIO_PIN6);   // RF_DataReady 
     GPIO_setAsInputPin /*WithPullDownResistor */(GPIO_PORT_P3, GPIO_PIN7);   // RF_CD 
	 
#endif     
}

void deinitPortRF(void) 
{
	GPIO_setAsInputPin(    		GPIO_PORT_P4, GPIO_PIN3);  // RF-on
	GPIO_setAsInputPin(    		GPIO_PORT_P5, GPIO_PIN0 + GPIO_PIN1 + GPIO_PIN2 + GPIO_PIN3);  // RF_TX_EN, RF_TX_CE, RF_PWRUP, UCB1STE
	GPIO_setAsInputPin(			GPIO_PORT_P3, GPIO_PIN0 + GPIO_PIN1 + GPIO_PIN2);

}

void initAudioPort(void)
{
     GPIO_setAsOutputPin(    		GPIO_PORT_P9, GPIO_PIN2+ GPIO_PIN1);  // Audio_SDA, Audio_SCL
     GPIO_setAsOutputPin(    		GPIO_PORT_P1, GPIO_PIN3);  // Audio_ON
     GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN3);   // Set Audio_ON high for OFF!! ..

     GPIO_setAsOutputPin(    		GPIO_PORT_P2, GPIO_PIN2);  // Audio_CS
     
//     GPIO_setAsInputPin(    		GPIO_PORT_P9, GPIO_PIN0);  // Audio_Analog

    //Enable P1.0 w/o (we have R9) internal resistance as pull-Up resistance
    GPIO_setAsInputPin/*WithPullUpResistor*/( GPIO_PORT_P1, GPIO_PIN0);

    //P1.0 Hi/Lo edge 
    GPIO_selectInterruptEdge(GPIO_PORT_P1, GPIO_PIN0, GPIO_HIGH_TO_LOW_TRANSITION);

#if GSD_FEATURE_ENABLED(DEBUGGING_MENU)
     GPIO_setAsOutputPin(    		GPIO_PORT_P9, GPIO_PIN4);  // TP11
     GPIO_setOutputLowOnPin(    		GPIO_PORT_P9, GPIO_PIN4);  // TP11
#endif // GSD_FEATURE_ENABLED(DEBUGGING_MENU)

    //Set P9.1 as Ternary Module Function Output.
    /*
    * Select Port 9
    * Set Pin 0 to output Ternary Module Function, (A8, C8, VREF+, VeREF+).
    */
    GPIO_setAsPeripheralModuleFunctionOutputPin(
    	GPIO_PORT_P9,
    	GPIO_PIN0,
    	GPIO_TERNARY_MODULE_FUNCTION
    );

    //Set P1.2 as Ternary Module Function Output.
    /*
    * Select Port 1
    * Set Pin 2 to input Secodnary Module Function, (TA0CLK).
    */
    GPIO_setAsPeripheralModuleFunctionInputPin(
    	GPIO_PORT_P1,
    	GPIO_PIN2,
    	GPIO_SECONDARY_MODULE_FUNCTION
    );

	

}

void deinitAudioPort(void)
{
     GPIO_setAsInputPin(    		GPIO_PORT_P9, GPIO_PIN2+ GPIO_PIN1);  // Audio_SDA, Audio_SCL
     GPIO_setAsInputPin(    		GPIO_PORT_P1, GPIO_PIN3);  // Audio_ON
     GPIO_setAsInputPin(    		GPIO_PORT_P2, GPIO_PIN2);  // Audio_CS
     
//     GPIO_setAsInputPin(    		GPIO_PORT_P9, GPIO_PIN0);  // Audio_Analog
}

void initPortJ(void)
{
    // LFXT Setup
    //Set PJ.4 and PJ.5 as Primary Module Function Input.
    /*
    * Select Port J
    * Set Pin 4, 5 to input Primary Module Function, LFXT.
    */
    GPIO_setAsPeripheralModuleFunctionInputPin(
        GPIO_PORT_PJ,
        GPIO_PIN4 + GPIO_PIN5,
        GPIO_PRIMARY_MODULE_FUNCTION
    );
}

void audio_int_enable(void) 
{
	
    //P1.0 IFG cleared
    GPIO_clearInterrupt(GPIO_PORT_P1, GPIO_PIN0  );

    //P1.0 interrupt enabled
    GPIO_enableInterrupt( GPIO_PORT_P1, GPIO_PIN0);
}

void audio_int_disable(void) 
{
	
    //P1.0 IFG cleared
    GPIO_clearInterrupt(GPIO_PORT_P1, GPIO_PIN0  );

    //P1.0 interrupt disabled
    GPIO_disableInterrupt( GPIO_PORT_P1, GPIO_PIN0);
}

//******************************************************************************
//
//This is the PORT1_VECTOR interrupt vector service routine
//
//******************************************************************************
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=PORT1_VECTOR
__interrupt
#elif defined(__GNUC__)
__attribute__((interrupt(PORT1_VECTOR)))
#endif
void Port_1 (void)
{
    uint16_t irq;	

    LPM3_EXIT;
    	__no_operation();                         // For debugger
    irq = GPIO_getInterruptStatus(GPIO_PORT_P1, GPIO_PIN0);
	if ((irq & GPIO_PIN0) == GPIO_PIN0) {
		GPIO_clearInterrupt( GPIO_PORT_P1, GPIO_PIN0);
		gsd_audio_event = GSD_HAVE_EVENT;
	}
}

