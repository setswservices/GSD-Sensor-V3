/*
 * debugPort.c
 *
 *  Created on: Oct 29, 2019
 *      Author: dantonets
 */
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "file.h"
#include "ctype.h"
#include "driverlib.h"
#include "gsd_version.h"
#include "gsd_config.h"
#include "chars.h"

extern uint8_t  gsd_uart_event;

#if GSD_FEATURE_ENABLED(DEBUG_SERIAL_PORT)
static uint8_t RXData = 0;

#if GSD_FEATURE_ENABLED(GSD_USE_PRINTF)
static int   dbg_uart_open(const char *path, unsigned flags, int llv_fd)
{
	if ((flags & O_RDWR == O_RDWR) || (flags & O_WRONLY == O_WRONLY)) {
		return 3;
	}
	return (-1);
}

static int  dbg_uart_close(int dev_fd)
{
	return 0;
}

static int  dbg_uart_read(int dev_fd, char *buf, unsigned count)
{
	return 0;
}

static  off_t dbg_uart_lseek(int dev_fd, off_t offset, int origin)
{
	return (-1);
}

static int dbg_uart_unlink(const char *path)
{
	return (-1);
}

static int dbg_uart_rename(const char *old_name, const char *new_name)
{
	return (-1);
}
#endif // GSD_FEATURE_ENABLED(GSD_USE_PRINTF)
static  int dbg_uart_write(int dev_fd, const char *buf, unsigned count)
{
	unsigned idx = 0;
	while(idx < count) {
		EUSCI_A_UART_transmitData(EUSCI_A0_BASE,  buf[idx++]);
	}
	return count;
}


void	debugPortInit(void)
{

    // Configure UART pins
    //Set P2.0 and P2.1 as Secondary Module Function Input.
    /*
    * Select Port 2d
    * Set Pin 0, 1 to input Secondary Module Function, (UCA0TXD/UCA0SIMO, UCA0RXD/UCA0SOMI).
    */
    GPIO_setAsPeripheralModuleFunctionInputPin(
    GPIO_PORT_P2,
    GPIO_PIN0 + GPIO_PIN1,
    GPIO_PRIMARY_MODULE_FUNCTION
    );


    // Configure UART
    EUSCI_A_UART_initParam param = {0};
    param.selectClockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK;
#if GSD_VERSION_EQU(53885a92)
    param.clockPrescalar = 52;
    param.firstModReg = 1;
    param.secondModReg = 73;
#else
    param.clockPrescalar = 4;
    param.firstModReg = 5;
    param.secondModReg = 85;
#endif //GSD_VERSION_EQU(53885a92)
    param.parity = EUSCI_A_UART_NO_PARITY;
    param.msborLsbFirst = EUSCI_A_UART_LSB_FIRST;
    param.numberofStopBits = EUSCI_A_UART_ONE_STOP_BIT;
    param.uartMode = EUSCI_A_UART_MODE;
    param.overSampling = EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION;
    

    if (STATUS_FAIL == EUSCI_A_UART_init(EUSCI_A0_BASE, &param)) {
        return;
    }

    EUSCI_A_UART_enable(EUSCI_A0_BASE);

    EUSCI_A_UART_clearInterrupt(EUSCI_A0_BASE,  EUSCI_A_UART_RECEIVE_INTERRUPT);

    // Enable USCI_A0 RX interrupt
    EUSCI_A_UART_enableInterrupt(EUSCI_A0_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);                     // Enable interrupt
 }

void debugPortDisable(void)
{
	EUSCI_A_UART_disableInterrupt(EUSCI_A0_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
	EUSCI_A_UART_disable(EUSCI_A0_BASE);
	GPIO_setAsInputPin(GPIO_PORT_P2, GPIO_PIN0);
	GPIO_setAsInputPin(GPIO_PORT_P2, GPIO_PIN1);
	
	GPIO_setAsInputPin(GPIO_PORT_P3, GPIO_PIN4);
	GPIO_setAsInputPin(GPIO_PORT_P3, GPIO_PIN5);
}

void vPrintString(char *str)
{
	dbg_uart_write(NULL, str, strlen(str));
}

void vPrintChar(char c)
{
	char tmp[2];
	tmp[0] = c;
	dbg_uart_write(NULL, tmp, 1);
}

void vPrintEOL(void)
{
	dbg_uart_write(NULL, "\r\n", 2);
}

char *psUInt16ToString(uint16_t d, char *s)
{
	char tmp[32];
	uint8_t idx = 30;
	
	if (s == NULL)  return NULL;
	memset(tmp, 0x0, 32);
	if (d == 0) {
		tmp[ idx--] = '0';
	}else{
		while(d != 0) {
			tmp[idx--] = (d % 10) + '0';
			d/=10;
		}
	}
	strcpy(s, tmp + idx +1);
	return s;
 }

char *psUInt16HexToString(uint16_t d, char *s)
{
	char tmp[5];
	uint8_t idx = 0;
	
	if (s == NULL)  return NULL;
	memset(tmp, 0x0, 5);
	for(idx =0; idx <4; idx++) {
		tmp[ idx] = "0123456789abcdef"[((0xF000 & d) >> 12)&15];
		d<<=4;
	}
	strcpy(s, tmp);
	return s;

}

char *psUInt8HexToString(uint8_t d, char *s)
{
	char tmp[3];
	uint8_t idx = 0;
	
	if (s == NULL)  return NULL;
	memset(tmp, 0x0, 3);
	for(idx =0; idx <2; idx++) {
		tmp[ idx] = "0123456789abcdef"[((0xF0 & d) >> 4)&15];
		d<<=4;
	}
	strcpy(s, tmp);
	return s;

}

int uart_getchar(void)
{
    int ch; 	
    if (RXData == 0)  return 0;	
    ch = RXData;
    RXData = 0;	
    return ch;
}

void uart_cleanRXData(void)
{
    RXData = 0;	
}
	
unsigned long uart_getInt(void) 
{	
	uint8_t inBuf[9];
	uint8_t idx =0;
	int c;
	unsigned long tmp;

//	memset(inBuf, 0x20, 8);
	while(1) {
		delay_ms(1);
		c = uart_getchar();
		if (c == 0) continue; 
		if (c == CHAR_ESC)  return 0;
		else
		if (c == 0x08) {
			if (idx) {
				idx--;	
				inBuf[idx] = '\0';
				vPrintString("\b \b");
			}
		}else
		if (c == 0x0D) {
			inBuf[idx] = '\0';
			vPrintEOL();
			tmp = strtoul((char *)inBuf, NULL, 10);
			return tmp;
		}else
		if (_isdigit(c)) {
			vPrintChar(c);
			inBuf[ idx] = (uint8_t)(c&0xFF);
			idx++;
			if (idx == 8) {
				inBuf[ idx] = '\0';
				tmp = strtoul((char *)inBuf, NULL, 10);
				return tmp;
			}
		}
	}
}

unsigned long uart_getHex(void) 
{	
	uint8_t inBuf[9];
	uint8_t idx =0;
	int c;
	unsigned long tmp;

//	memset(inBuf, 0x20, 8);
	while(1) {
		delay_ms(1);
		c = uart_getchar();
		if (c == 0) continue; 
		if (c == CHAR_ESC)  return 0;
		else
		if (c == 0x08) {
			if (idx) {
				idx--;	
				inBuf[idx] = '\0';
				vPrintString("\b");
			}
		}else
		if (c == 0x0D) {
		    inBuf[idx] = '\0';
			vPrintEOL();
			tmp = strtoul((char *)inBuf, NULL, 16);
			return tmp;
		}else
		if (_isxdigit(c)) {
			vPrintChar(c);
			inBuf[ idx] = (uint8_t)(c&0xFF);
			idx++;
			if (idx == 8) {
				inBuf[ idx] = '\0';
				tmp = strtoul((char *)inBuf, NULL, 16);
				return tmp;
			}
		}
	}
}

uint8_t uart_getHexTo(unsigned long *pOut) 
{	
	uint8_t inBuf[9];
	uint8_t idx =0;
	int c;

	while(1) {
		delay_ms(1);
		c = uart_getchar();
		if (c == 0) continue; 
		if (c == CHAR_ESC)  return idx;
		else
		if (c == 0x08) {
			if (idx) {
				idx--;	
				inBuf[idx] = '\0';
				vPrintString("\b");
			}
		}else
		if (c == 0x0D) {
		    inBuf[idx] = '\0';
			vPrintEOL();
			*pOut = strtoul((char *)inBuf, NULL, 16);
			return idx;
		}else
		if (_isxdigit(c)) {
			vPrintChar(c);
			inBuf[ idx] = (uint8_t)(c&0xFF);
			idx++;
			if (idx == 8) {
				inBuf[ idx] = '\0';
				*pOut = strtoul((char *)inBuf, NULL, 16);
				return idx;
			}
		}
	}
	return 0;
}

uint8_t uart_getByte(void) 
{	
	uint8_t inBuf;
	int c;

	while(1) {
		delay_ms(1);
		c = uart_getchar();
		if (c == 0) continue; 
		if (c == CHAR_ESC)  return 0;
		inBuf = (c&0xFF);
		return inBuf;
	}
}


//******************************************************************************
//
//This is the USCI_A0 interrupt vector service routine.
//
//******************************************************************************
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCI_A0_VECTOR
__interrupt
#elif defined(__GNUC__)
__attribute__((interrupt(USCI_A0_VECTOR)))
#endif
void USCI_A0_ISR(void)
{
  LPM3_EXIT;
  switch(__even_in_range(UCA0IV,USCI_UART_UCTXCPTIFG))
  {
    case USCI_NONE: break;
    case USCI_UART_UCRXIFG:
      RXData = EUSCI_A_UART_receiveData(EUSCI_A0_BASE);
      if (gsd_uart_event == GSD_NO_EVENT) 
	  gsd_uart_event = GSD_HAVE_EVENT;
      break;
    case USCI_UART_UCTXIFG: break;
    case USCI_UART_UCSTTIFG: break;
    case USCI_UART_UCTXCPTIFG: break;
  }
}

#endif // GSD_FEATURE_ENABLED(DEBUG_SERIAL_PORT)


