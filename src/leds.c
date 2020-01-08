/*
 * leds.c
 *
 *  Created on: Jan 7, 2020
 *      Author: dantonets
 */

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "file.h"
#include "ctype.h"
#include "driverlib.h"
#include "gsd.h"
#include "chars.h"

extern gsd_setup_t gsd_setup;

void flash_led0_red(void) 
{
	if (gsd_setup.RAMn_LED_ONFLG) {
		uint8_t idx = No_LED_PULSES;
		while(idx--) {
			GPIO_setOutputLowOnPin (GPIO_PORT_P2, GPIO_PIN6);
			DelayMS(200);
			GPIO_setOutputHighOnPin (GPIO_PORT_P2, GPIO_PIN6);
			DelayMS(200);
		}
	}
}

void flash_led1_green(void) 
{
	if (gsd_setup.RAMn_LED_ONFLG) {
		uint8_t idx = No_LED_PULSES;
		while(idx--) {
			GPIO_setOutputLowOnPin (GPIO_PORT_P2, GPIO_PIN7);
			DelayMS(200);
			GPIO_setOutputHighOnPin (GPIO_PORT_P2, GPIO_PIN7);
			DelayMS(200);
		}
	}
}

void rx_led1_green(void) 
{
	if (gsd_setup.RAMn_LED_ONFLG) {
		uint8_t idx = 2;
		while(idx--) {
			GPIO_setOutputLowOnPin (GPIO_PORT_P2, GPIO_PIN7);
			DelayMS(200);
			GPIO_setOutputHighOnPin (GPIO_PORT_P2, GPIO_PIN7);
			DelayMS(200);
		}
	}
}

