/*
 * dataPort.c
 *
 *  Created on: Jan 2, 2020
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

#if GSD_FEATURE_ENABLED(DATA_PORT)

//RAM_FRAM          .equ  0BFC0H            ;BFC0 + 4000=FFC0 (EXTRA 16K)
#define RAM_FRAM    0x0BFC0           

void	dataPortInit(void)
{

    /*
    * Select Port 3
    * Set Pin 4, 5 to input Primary Module Function, (UCA1TXD/UCA1SIMO, UCA1RXD/UCA1SOMI).
    */
    GPIO_setAsPeripheralModuleFunctionInputPin(
    GPIO_PORT_P3,
    GPIO_PIN4 + GPIO_PIN5,
    GPIO_PRIMARY_MODULE_FUNCTION
    );


    // Configure UART
    EUSCI_A_UART_initParam param = {0};
    param.selectClockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK;
    param.clockPrescalar = 4;
    param.firstModReg = 5;
    param.secondModReg = 85;
    param.parity = EUSCI_A_UART_NO_PARITY;
    param.msborLsbFirst = EUSCI_A_UART_LSB_FIRST;
    param.numberofStopBits = EUSCI_A_UART_ONE_STOP_BIT;
    param.uartMode = EUSCI_A_UART_MODE;
    param.overSampling = EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION;
    

    if (STATUS_FAIL == EUSCI_A_UART_init(EUSCI_A1_BASE, &param)) {
        return;
    }

    EUSCI_A_UART_enable(EUSCI_A1_BASE);

    EUSCI_A_UART_clearInterrupt(EUSCI_A1_BASE,  EUSCI_A_UART_RECEIVE_INTERRUPT);

    // Enable USCI_A1 RX interrupt
    // EUSCI_A_UART_enableInterrupt(EUSCI_A1_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);                     // Enable interrupt
 }

static  int data_uart_write(int dev_fd, const char *buf, unsigned count)
{
	unsigned idx = 0;
	while(idx < count) {
		EUSCI_A_UART_transmitData(EUSCI_A1_BASE,  buf[idx++]);
	}
	return count;
}

static  int data_port_write_fram(int dev_fd, unsigned long fram, unsigned count)
{
	unsigned idx = 0;
	while(idx < count) {
		EUSCI_A_UART_transmitData(EUSCI_A1_BASE,  __data20_read_char(fram + idx));
		idx++;
	}
	return count;
}

void vDataOut(uint8_t  *data, uint16_t len)
{
	data_uart_write(0, (char *)data, len);
}

static void vMoveCursorBack(void ) 
{
	uint8_t len = strlen(prt_buf);
	while(len--) vPrintChar('\b');
}


void vWfDataOut(void)
{
	uint16_t idx;
	unsigned long fram2 = RAM_EXTENDED;
	uint8_t  *fram = (uint8_t *)RAM_FRAM;

	dataPortInit();

	vPrintString("> ");
	for(idx = 0; idx < 80; fram2+= 1024, idx++)
	{
		data_port_write_fram(0, fram2, 1024);
		vPrintString(psUInt16ToString(idx, prt_buf));
		vMoveCursorBack();
	}
	
	for(idx = 0; idx < 16; fram+= 1024, idx++)
	{
		data_uart_write(0, fram, 1024);
		vPrintString(psUInt16ToString((idx + 80), prt_buf));
		vMoveCursorBack();
	}

	vPrintEOL(); vPrintEOL();

	// de-init data port..
	EUSCI_A_UART_disable(EUSCI_A1_BASE);
	GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_PIN4 + GPIO_PIN5 );

}

#endif // GSD_FEATURE_ENABLED(DATA_PORT)

