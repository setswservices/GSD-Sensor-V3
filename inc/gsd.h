/*
 * gsd.h
 *
 *  Created on:	Nov 2, 2019
 *  Updated 		Dec 18, 2019
 *      Author: dantonets
 */

#ifndef GSD_H_
#define GSD_H_

#include "gsd_version.h"
#include "gsd_config.h"

#define NoAUDIO_SAMPLES_PER_CHANK		(16*1024)

/* ========================
 *  FW startup states
 *========================*/
#define	GSD_RESET   	(0x01)
#define   GSD_WAKEUP	(0x02)

/*  === from EQU.ASM ====
  *  [ADK]  11/05/2019    Keep it "as is" ..
;STARTS @ 1CD0H
RAMn_SETUP              .equ  RAMn_BASE        ;1D00 (SEE DS FOR RAMn_SETUPx)
RAMn_TAGID              .equ  RAMn_SETUP+00H   ;WORD,    (KEEP ORDERED)
RAMn_FUNCTION           .equ  RAMn_SETUP+02H   ;BYTE,          
RAMn_MODE               .equ  RAMn_SETUP+03H   ;BYTE,          
RAMn_ROOMNo             .equ  RAMn_SETUP+04H   ;WORD           
RAMn_FLOORNo            .equ  RAMn_SETUP+06H   ;BYTE           
RAMn_ALRM_SF            .equ  RAMn_SETUP+07H   ;BYTE           
RAMn_RF_SLOTNo          .equ  RAMn_SETUP+08H   ;BYTE,          
RAMn_SLOT_DLYNo         .equ  RAMn_SETUP+09H   ;BYTE,         
RAMn_DA01               .equ  RAMn_SETUP+0AH   ;WORD, DETECT TH DA               
RAMn_VAR_LMT0           .equ  RAMn_SETUP+0CH   ;WORD  VAR ALRM LIMIT
RAMn_VAR_LMT2           .equ  RAMn_SETUP+0EH   ;WORD            
RAMn_ALRM0HR            .equ  RAMn_SETUP+10H   ;BYTE, RTCC WAKEUP ALARM0-HOURS, PWR OFF                     
RAMn_ALRM1HR            .equ  RAMn_SETUP+11H   ;BYTE, RTCC WAKEUP ALARM1-HOURS, PWR ON                      
RAMn_HBPM_INTERVAL      .equ  RAMn_SETUP+12H   ;BYTE, TYPE OF HB & PM          
RAMn_LED_ONFLG          .equ  RAMn_SETUP+13H   ;BYTE, LED DSPLY FLG
RAMn_DEBUG              .equ  RAMn_SETUP+14H   ;BYTE, DEBUG DSPLY FLG
RAMn_BEACON_DLY         .equ  RAMn_SETUP+15H   ;BYTE, RF BEACON OUT TIME
RAMn_STARTUP_FLG        .equ  RAMn_SETUP+16H   ;BYTE, PWRUP TO RUN OR MENU
RAMn_LED_DETECTFLG      .equ  RAMn_SETUP+17H   ;BYTE, LED DETECT DSPLY FLG
RAMn_ALRM0MIN           .equ  RAMn_SETUP+18H   ;BYTE, RTCC WAKEUP ALARM0-MINS, PWR OFF                     
RAMn_ALRM1MIN           .equ  RAMn_SETUP+19H   ;BYTE, RTCC WAKEUP ALARM1-MINS, PWR ON                      
RAMn_RFTOUT             .equ  RAMn_SETUP+1AH   ;WORD, 
RAMn_RF_FREQ            .equ  RAMn_SETUP+1CH   ;BYTE,
 =====  */
typedef struct {
	uint16_t		RAMn_TAGID;
	uint8_t		RAMn_FUNCTION;
	uint8_t		RAMn_MODE;
	uint16_t		RAMn_ROOMNo;
	uint8_t		RAMn_FLOORNo;
	uint8_t		RAMn_ALARM_SF;
	uint8_t		RAMn_RF_SLOTNo;
	uint8_t		RAMn_SLOT_DLYNo;
	uint16_t		RAMn_DA01;			// Detect Threshold, for LTC1662 DAC
	uint16_t		RAMn_VAR_LMT0;  	
	uint16_t		RAMn_VAR_LMT2;
	uint8_t		RAMn_ALRM0HR;	      // RTC Alarm setup hours for PWR OFF (to LPM3.5) in BCD
	uint8_t		RAMn_ALRM1HR;      //  RTC Alarm setup  hours for PWR ON (from PPM3.5) in BCD 		
	uint8_t		RAMn_HBPM_INTERVAL;  
	uint8_t		RAMn_LED_ONFLG;
	uint8_t		RAMn_DEBUG;
	uint8_t		RAMn_BEACON_DLY;
	uint8_t		RAMn_STARTUP_FLG;
	uint8_t		RAMn_LED_DETECTFLG;
	uint8_t		RAMn_ALRM0MIN;	// RTC Alarm setup Minutes for PWR OFF
	uint8_t		RAMn_ALRM1MIN;     // RTC Alarm setup minutes for PWR ON
	uint16_t		RAMn_RFTOUT;         // [ADK] 11/11/2019  RTC Alarm setup for start HB communitations [hours:minutes]
	uint8_t		RAMn_RF_FREQ;
// [ADK] 11/11/2019  add RTC setup for restore it at wakeup from LPM3.5
	uint8_t		RAMn_RTC_PACK[4];		// in the packet format
// [ADK] 03/31/2020   add FW version
	uint8_t		RAMn_FW_VERSION[4];
} gsd_setup_t;

/*  
From 16_run.asm

FUNCTION CODES
CODE_RUN_woHBPM     .equ  00H             ;RUN woHBPM  
CODE_RUN_wHB        .equ  01H             ;RUN wHB  
CODE_RUN_wPM        .equ  02H             ;RUN wPM  
CODE_RUN_wHBPM      .equ  04H             ;RUN wHB+PM
CODE_DEEP_SLEEP     .equ  80H             ;FUNCTION-WAITING FOR INSTALLATION  WITH NFC
CODE_RF_BEACON      .equ  10H             ;FUNCTION--SND OUT RF BEACON FOR 
*/

#define CODE_RUN_woHBPM			0x00
#define CODE_RUN_wHB				0x01
#define CODE_RUN_wPM				0x02
#define CODE_RUN_SHORT_DATA_OUT	0x04
#define CODE_RUN_WITH_ACK			0x08
#define CODE_RUN_LIVE_TEST			0x20
#define CODE_RUN_CALB_ALRM_TH 	0x40
#define CODE_RUN_FULL_DATA_OUT	0x80


#if 0
// [ADK]  11/18/2019  - it is  new format for the HB packet.
#define HB_RESET_PACKET	(0x11)
#define HB_PWR_OFF_PACKET	(0x13)
#define HB_PWR_ON_PACKET	(0x15)

#define HB_DEFAULT_TAGID	((uint16_t)0x8080)

typedef struct {
	uint16_t		HB_TAGID;		// TagId from EEPROM (0x8080 is a default value, not initialized)
	uint16_t		HB_RF_RX_ADD_LOW;	// Low part of Sensor Rx address. Bridge will use it for send a responce. (default = E7E7H)
	uint16_t		HB_RF_RX_ADD_HIGH;	// High part of Sensor Rx address. Bridge will use it for send a responce. (default = E7E7H)
	uint8_t		HB_PKT_TYPE;    
	/*
	  *   It packet format for the bi-directional communication between Sensor and Bridge
	  *	0x[1-6]x - HB from Sensor to the Bridge
	  *   0x[7-F]x - Respond for the HB packet from Bridge to the Sensor
	  *		0x11 - HB packet from the Sensor at Reset. Even TagId can be not initialized at very first Reset.
	  *		0x13 - HB packet from Sensor at PWR OFF  (HB FLG = F0H in old format)
	  *		0x15 - HB packet from Sensor at PWR ON   (HB FLG = F1H in old format)
	  *
	  *		0x7F - HB Packet responce from the Bridge for initial setup, include TagId. This packet only use for change the TagId
	 */
	 uint8_t		HB_SYS_FLG;			// System info
	 uint8_t		HB_WAIT_RESP_MIN;		// Duration (in minutes) when Sensor will switch to Rx mode and wait a response from the Bridge. (default - 1 minute)
	 uint8_t		RF_CH_NO;				// in hex, just low 8 bits
	 uint8_t		RF_SLOT;
	 uint8_t		RF_DLY;
	 uint8_t		POWER_OFF_HR;			// in BCD 
	 uint8_t		POWER_OFF_MIN;		// in BCD 
	 uint8_t		POWER_ON_HR;			// in BCD 
	 uint8_t		POWER_ON_MIN;		// in BCD 
	 uint8_t		RTC[4];				// RTC setup in the packed format.
} gsd_hb_packet_t;
#endif
/*
;************************ RF FOR HBEAT EVENT *******************
;HEART BEAT EVENT RF OUT                  (5/15/17)            *
;NOTE: DIFFERENT THAN ACOUSTIC RF OUT                          *           
;    : VARIANCE REPLACED WITH 1 RTCC ALARM TIMES-HR & MIN      *
;                                                              *
;RF FORMAT FOR HEART BEAT MESSAGE                              *           
;  BYTE #  -00: TAG ID          (2 BYTES)                      *           
; ----------------------------------------------*
;          -02: USB RF HB ECHO  (2 BYTES)                      *           
;                               (6666=RF ACK, 8888=RF SETUP)   *
; ----------------------------------------------*
;[ADK] 03/31/2020 - Replaced with:
;	     For packet with ALRM/REJECT/HB FLG = F1H:
;          -02: FW BUILD DATETIME (FIRST 2 BYTES, PACKED)  *           
;	     For ALRM/REJECT/HB FLG = 00H | 01H and MODE_FLG=04H:
;          -02: USB RF HB ECHO  (2 BYTES)                       *           
;                               (#packet with short audio data)     *
;	     Not in use othewise
; ----------------------------------------------*
;          -04: FUNCTION        (00=RUN woHBEAT+POWER MGMT(PM) *
;                               (01=RUN wHBEAT)                *
;                               (02=RUN wPM)                   *
;                               (04=RUN wHBEAT+POWER MGMT)     *
;                               (10=RF BEACON/MAPPING)         *
;                               (80=DEEP SLEEP)                *
;          -05: MODE FLG     (80=SND WF HOST, 40=CALB ALRM TH) *
;                            (04=SND DATA RF, 02=SND EVENT RF) * 
;				  (08= EXPECT ACK FROM THE BRIDGE)  * [ADK] 01/06/2020  
;                            (01=SND ALRM ONLY RF)             *                                               
;          -06: ALRM/REJECT/HB FLG (1 BYTE)                    *
;                                  (00=REJECT, 01=ALRM)        *
;                                  (81=DOUBLE SHOT)            *
;                                  (F0=HBEAT wAUDIO PWR OFF)   *
;                                  (F1=HBEAT wAUDIO PWR ON)    *
;          -07: ROOM DESCRIPTOR (1 BYTE)--ASCII CHAR           *           
;       #08-09: ROOM #          (2 BYTES--ROOM #               *           
;          -0A: RF SLOT#        (1 BYTE)--RF SLOT #            *           
;          -0B: SYSTEM FLG(BIT) (1 BYTE)  (BUILT-IN-TEST)      *
;                                  BIT#7: SHOT ALRM FLG-STICKY *                            
;                                  BIT#6: EEPROM_NOTCLRFLG     *
;                                  BIT#5:                      *                            
;                                  BIT#4:                      *                    
;                                  BIT#3:                      *                    
;                                  BIT#2: RFID ERR FLG         *                    
;                                  BIT#1: EEPROM ERR FLG       *                            
;                                  BIT#0: AUDIO_ERR FLG        *                            
;           0C: AUDIO POWER OFF TIME-HR            (00-23H)    *
;           0D: AUDIO POWER ON  TIME-HR=HBEAT TIME (00-23H)    *
;           0E: HB/PM INTERVAL                                 *
;                  PM: 1X=DAILY,2X=MON-FRI                     *
;                  HB: X1=DAILY,X2=MON/WED/FRI,X4=MONDAY ONLY  *
;           0F: CALB ALRM SCALE FACTOR (SF)                    *
; ----------------------------------------------*
;       #10-11: #RF XMTS (~# HBEATS)                           *
; ----------------------------------------------*
;[ADK] 03/31/2020 - Replaced with:
;	     For packet with ALRM/REJECT/HB FLG = F1H:
;       #10-11: FW BUILD DATETIME (LAST 2 BYTES, PACKED)           *
;	 All other cases:
;       #10-11: #RF XMTS (~# HBEATS)                           *
; ----------------------------------------------*
;       #12-13: AUDIO DETECTION THRESHOLD--D/A  (0800-1023)    *
;           14: 00H                                            *
;           15: 00H                                            *
;           16: AUDIO POWER OFF & HBEAT TIME: MIN    (00-59H)    *
;           17: AUDIO POWER ON & HBEAT TIME: MIN   (00-59H)    *
;       #18-1B: ALARM THRESHOLD (4 BYTES)                      *
;       #1C-1F: RTC             (4 BYTES)                      *
;      ----------           -----------                        *
;           20H=32              32 BYTES                       *
;***************************************************************
;	with Audio data: MODE_FLG = 0x04: 
;	1st packet: HB_ECHO = 0, ALARM/Reject packet  + pkt[7:28] := audio data (first 22 bytes), 
;      2nd packet HB_ECHO = 1, ALARM/Reject packet  + pkt[7:28] := audio data (second 22 bytes),  
;      3rd packet HB_ECHO = 2, ALARM/Reject packet  + pkt[7:28] := audio data (third 22 bytes)  
;
;
*/
#define HBEAT_wAUDIO_PWR_OFF		(0xF0)
#define HBEAT_wAUDIO_PWR_ON		(0xF1)
#define HBEAT_wAUDIO_PWR_OFF_3		(0xA0)
#define HBEAT_wAUDIO_PWR_ON_3		(0xA1)
#define AUDIO_DATA						(0xB5)
#define HBEAT_ACK						(0xC6)

#pragma pack(push, 2)
typedef struct {
	uint16_t		HB_TAGID;		// TagId from EEPROM (0x8000 is a default value, if not initialized)
	uint16_t		HB_ECHO;
	uint8_t		HB_FUNCTION;
	uint8_t		HB_MODE;
	uint8_t		HB_ALRM_FLG;
	uint8_t		HB_ROOM_DSCR;
	uint16_t		HB_ROOM_No;
	uint8_t		HB_RF_SLOT_No;
	uint8_t		HB_SYS_FLG;
	uint8_t		HB_PWROFF_HR;
	uint8_t		HB_PWRON_HR;
	uint8_t		HB_PM_INTRV;
	uint8_t		HB_CAL_ALRM_SCALE;
	uint16_t		HB_XMTS_No;
	uint16_t		HB_AUDIO_THRSH;
	uint8_t		HB_RF_DLY;				// [ADK] 11/18/2019:  New member, for RF delay setup with RF SLOT# 
	uint8_t		HB_WAIT_RESP_MIN;		// [ADK] 11/18/2019:  New member for setup wait a response from the Bridge (minutes, default =1)
	uint8_t		HB_PWROFF_MIN;
	uint8_t		HB_PWRON_MIN;
	uint16_t		HB_ALRM_THRSH_LOW;
	uint16_t		HB_ALRM_THRSH_HIGH;
	uint16_t		HB_RTC_LOW;
	uint16_t		HB_RTC_HIGH;
} gsd_hb_packet_t;
#pragma pack(pop)

#define RAM_FRAM_INFO	0x1800                
// 1800-19FF FOR FR59x9

void ResetHW(void);
void EnterLPM35_cmd(void);

#if GSD_FEATURE_ENABLED(DEBUG_SERIAL_PORT)
extern char prt_buf[32];

void vPrintString(char *str);
void vPrintChar(char c);
void vPrintEOL(void);
char *psUInt16ToString(uint16_t d, char *s);
char *psUInt16HexToString(uint16_t d, char *s);
char *psUInt8HexToString(uint8_t d, char *s);
#endif // GSD_FEATURE_ENABLED(DEBUG_SERIAL_PORT)


#if GSD_FEATURE_ENABLED(DEBUGGING_MENU)
void set_main_menu(void);
void gsd_print_menu(void);
int gsd_select_menu_item(int input);
#endif // GSD_FEATURE_ENABLED(DEBUGGING_MENU)


void init_ports(void);
void FlasLed0Red(void); 
void FlasLed0Green(void); 
void rtc_init(void);
void rtc_load(void);
void rtc_set_alarm(uint8_t bcd_hr, uint8_t bcd_min);
void rtc_enable_alarm(void);
void rtc_disable_alarm(void);
void rtc_set_fake_time(void);
void packet2calendar(uint8_t *packet);
uint8_t * calendar2packet(uint8_t *packet);
uint8_t  rtc_get_wday(void);
void printVersion(void);

// === nRF905 related ..
void initPortRF(void); 
void deinitPortRF(void); 
void nRF905_spi_init(void);
void nRF905_pwr_on(void);
void nRF905_pwr_off(void);
void nRF905_TxDone(void);
void nRF905_RxStart(void);
void nRF905_RxDone(void);
void nRF905_Rx(void);
void nRF905_SetRTC(void);
void nRF905_SetWkUpRTC(void);
void nRF905_SetDtWakeUp(uint8_t hr, uint8_t min);
void nRF905_sndRstHB(void);
uint8_t getSendWfFlag(void);
void nRF905_send_2nd(void);
void nRF905_send_3rd(void);
void nRF905_put_setup(void);


#if GSD_FEATURE_ENABLED(DEBUG_SERIAL_PORT)
void rtc_get(void);
void printDateTime(void);
#endif //GSD_FEATURE_ENABLED(DEBUG_SERIAL_PORT)

// === Audio related ..
void initAudioPort(void);
void audio_int_enable(void);
void audio_int_disable(void); 
void audioON(void);
void audioOFF(void);
void audioSetThs(void);
void adcInit(void);
void audioStart(void);
void audioHandleAudioEvent(void);

// === Tx Timer
void txTimerInit(void);
void txTimerSet(uint8_t cnt);
void txTimerStart(void);
void txTimerStop(void);


void setup_enter(void);
void load_setup(void);
void save_setup(void); 
void init_setup(void);
void init_version(void);
void put_setup(gsd_hb_packet_t  *hb_pkt);

void flash_led0_red(void);
void flash_led1_green(void);
void rx_led1_green(void);
void pwr_led0_red(void);
void rx_fault(void) ;


#if GSD_FEATURE_ENABLED(DATA_PORT)

void	dataPortInit(void);
void vDataOut(uint8_t  *data, uint16_t len);
void vWfDataOut(void);

#endif //GSD_FEATURE_ENABLED(DATA_PORT)
#if GSD_FEATURE_ENABLED(DEBUG_RTC_SETUP)
int dbgRtcSet(void);

#endif //GSD_FEATURE_ENABLED(DEBUG_RTC_SETUP)

#endif /* GSD_H_ */
