/*
 * gsd_config.h
 *
 *  Created on: Oct 29, 2019
 *      Author: dantonets
 *   This file contains defines for the GSD app build. 
 */

#ifndef GSD_CONFIG_H_
#define GSD_CONFIG_H_

#include "chars.h"

#define FW_MAJ_REV       03 		 // major rev for substantial changes or releases
#define FW_MIN_REV       01  		// less major revisions
#define FW_BUILD_NUMBER  00  	// build numbers used for test revisions within a minor rev

#define STRING_IT(x) # x
#define VERSION_TO_STRING(major, minor, build) "v" STRING_IT(major) "." STRING_IT(minor) "." STRING_IT(build)
#define FIRMWARE_VERSION_STRING \
    VERSION_TO_STRING(FW_MAJ_REV, FW_MIN_REV, FW_BUILD_NUMBER)


//lint -emacro(491,KNOB_FEATURE_ENABLED) // Suppers warning 491 "non-standard use of 'defined' preprocessor operator"
#define GSD_FEATURE_ENABLED(feature) \
    ((defined(feature ## _ENABLED) && (feature ## _ENABLED)) ? 1 : 0)

#define delay_ms(_x_) 	_delay_cycles((unsigned long)((8000 -1)*(_x_)))

///////////////////////////////////////////////////////////////////////////////////////////////
// The main defines for used HW and FW
///////////////////////////////////////////////////////////////////////////////////////////////

#define DEBUG_SERIAL_PORT_ENABLED			1
#define DEBUG_BEFORE_EEPROM_INIT_ENABLED	1

#if GSD_FEATURE_ENABLED(DEBUG_SERIAL_PORT)
#define DEBUGGING_MENU_ENABLED				1
#endif // GSD_FEATURE_ENABLED(DEBUG_SERIAL_PORT)
#define xCHK_MULTIPLE_SHOTS					0

#if GSD_FEATURE_ENABLED(DEBUG_SERIAL_PORT)
void	debugPortInit(void);
int uart_getchar(void);
unsigned long uart_getInt(void);
unsigned long uart_getHex(void); 
void debugPortDisable(void);
#endif //GSD_FEATURE_ENABLED(DEBUG_SERIAL_PORT)

void initPortJ(void);
void DelayMS(unsigned int ms);

#endif /* GSD_CONFIG_H_ */
