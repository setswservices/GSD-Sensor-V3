/*
 * gsd_config.h
 *
 *  Created on: Oct 29, 2019
 *      Author: dantonets
 *   This file contains defines for the GSD app build. 
 */

#ifndef GSD_CONFIG_H_
#define GSD_CONFIG_H_

typedef unsigned char uint8_t;
#include "chars.h"

#define FW_MAJ_REV       03 		 // major rev for substantial changes or releases
#define FW_MIN_REV       1  		// less major revisions
#define FW_BUILD_NUMBER  00  	// build numbers used for test revisions within a minor rev

#define STRING_IT(x) # x
#define VERSION_TO_STRING(major, minor, build) "v" STRING_IT(major) "." STRING_IT(minor) "." STRING_IT(build)
#define FIRMWARE_VERSION_STRING \
    VERSION_TO_STRING(FW_MAJ_REV, FW_MIN_REV, FW_BUILD_NUMBER)


//lint -emacro(491,KNOB_FEATURE_ENABLED) // Suppers warning 491 "non-standard use of 'defined' preprocessor operator"
#define GSD_FEATURE_ENABLED(feature) \
    ((defined(feature ## _ENABLED) && (feature ## _ENABLED)) ? 1 : 0)

//lint -emacro(491,MUSH_FEATURE_ENABLED) // Suppers warning 491 "non-standard use of 'defined' preprocessor operator"
#define GSD_VERSION_EQU(version) 	\
    ((defined(GSD_FW_VERSION_CMP) && (v ## version == GSD_FW_VERSION_CMP)) ? 1 : 0)


#define delay_ms(_x_) 	_delay_cycles((unsigned long)((8000 -1)*(_x_)))
/* ========================
 *  Event states
 *========================*/
#define GSD_NO_EVENT		(0x00)
#define GSD_HAVE_EVENT		(0x01)

// RAM_EXTENDED     .equ  10000H              ;10000H-23FFFF=82K FOR FR5989    
#define RAM_EXTENDED     ((unsigned long)0x10000)
#ifdef __ASM__
#undef RAM_EXTENDED
#endif
#define GSD_MAX_TRY_ACK			(3)
// No_LED_PULSES .equ    05                  ;# 10ms ON/OFF TO MAKE 200ms DURATION
#define No_LED_PULSES 	(05)             


///////////////////////////////////////////////////////////////////////////////////////////////
// The main defines for used HW and FW
///////////////////////////////////////////////////////////////////////////////////////////////

#define DEBUG_SERIAL_PORT_ENABLED			1
#define GSD_USE_PRINTF_ENABLED				0
#define DEBUG_BEFORE_EEPROM_INIT_ENABLED	0
#define AUDIO_FRAM_CLEANUP_ENABLED			1
#define DATA_PORT_ENABLED						1
#define HBEAT_ACK_PACKET_ENABLED				1
/* ====================================
  *  [ADK]  01/14/2020: LPM4 - 0.5uA for the CPU. We can't use it becouse the can't control +V_Audio. [ADK] 02/10/2020  - We can control GPIO pins, it "locked" 
  *                               LPM3 - 0.7uA - we can control GPIO pins.
  *  					 Use  LPM3 for 01/14/2020 
*/
#define LPM4_ENABLED							0
/* ====================================
  * [ADK] 01/15/2020   Disable seep mode. For show only
*/
#define SLEEP_MODE_ENABLED					1
#define SLEEP_TIME_SETUP_FROM_AGGREGATOR_ENABLED		1

/* ====================================
  *  [ADK]  02/10/2020: Use this a debug mode for test deep sleep ans wakeup over weekend:
  *                               - Hit Ctrl-C, then Ctrl-S - and setup RTC to a Sunday with time 23:57
  *  					 -  Setup Alarm OFF to 23:59
  *                               -  Setup Alarm ON to 00:02
*/
#define DEBUG_RTC_SETUP_ENABLED		0

/* ====================================
  *  [ADK]  06/19/2020: Use this a debug mode for set the HW to the deep sleep
  *                               - Hit Ctrl-C, then Ctrl-X 
*/
#define DEBUG_DEEP_SLEEP_ENABLED		1

#if GSD_FEATURE_ENABLED(DEBUG_SERIAL_PORT)
#define DEBUGGING_MENU_ENABLED				1
#endif // GSD_FEATURE_ENABLED(DEBUG_SERIAL_PORT)
#define xCHK_MULTIPLE_SHOTS					0
#define DUMP_NRF905_CFG_ENABLED				0

#if GSD_FEATURE_ENABLED(DEBUG_SERIAL_PORT)
void	debugPortInit(void);
int uart_getchar(void);
void uart_cleanRXData(void);
unsigned long uart_getInt(void);
unsigned long  uart_getHex(void); 
void debugPortDisable(void);
uint8_t uart_getByte(void);
uint8_t uart_getHexTo(unsigned long *pOut);
#endif //GSD_FEATURE_ENABLED(DEBUG_SERIAL_PORT)

void initPortJ(void);
void DelayMS(unsigned int ms);

#endif /* GSD_CONFIG_H_ */
