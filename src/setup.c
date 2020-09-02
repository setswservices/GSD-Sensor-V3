/*
 * setup.c
 *
 *  Created on: Dec 30, 2019
 *      Author: dantonets
 */
#include "stdio.h"
#include "string.h"
#include "file.h"
#include "driverlib.h"
#include "gsd.h"

extern gsd_setup_t	gsd_setup;
#define LETTER_A      0x41
#define LETTER_Z      0x5A


static void default_setup(void)
{
/* ==== from GIT 
              .word     0080H               ;RAMn_TAGID  (WORD)
              .byte     01H                 ;FUNCTION FLG 
                                         ;00=RUN, 01=RUN wHB, 02=RUN wPM, 04=RUN wHB+PM
                                         ;80=DEEP SLEEP, 10=RF BEACON
              .byte     01H                 ;MODE FLG    ; WAS 04H 
                                         ;80=SND WF HW, 40=CALB ALRM TH, 04=SND TEST DATA RF
                                         ;02=SND EVENT RF, 01=SND ALRM ONLY RF
              .word     04D2H               ;ROOM #       (04D2=1234)          
              .byte     'A'                 ;FLOOR DESCRIPTOR--ASCII CHAR
              .byte     03H                 ;CALB ALARM SF                     
              .byte     01H                 ;RF SLOT # (00=NO DLY) 		; was 00H                       
              .byte     00H                 ;RF SLOT DLY # x10ms (00=NO DLY)   
              .word     0225H               ;AUDIO D/A TH                      
              .word     0000H               ;AUDIO VAR LMT0  (LSW)             
              .word     0018H               ;AUDIO VAR LMT2  (MSW)             
              .byte     24H                 ;RTCC ALRM0 HR---AUDIO POWER OFF  7:00 PM
              .byte     24H                 ;RTCC ALRM1 HR---AUDIO POWER ON   7:00 AM
              .byte     24H                 ;HB/PM INTERVAL
              .byte     0FFH                 ;LED REJECT/ALRM ON FLG  (00=OFF)
              .byte     0FFH                 ;DEBUG FLG  (NOT USED)
              .byte     03H                 ;RF BEACON DLY BETWEEN XMTS x 1SEC
              .byte     01H                 ;STARTUP FLG (00=MENU, 01=RUN)       
              .byte     01H                 ;LED DETECT ON FLG (00=OFF)               
              .byte     00H                 ;RTCC ALRM0 MIN---AUDIO POWER OFF  7:00 PM
              .byte     24H                 ;RTCC ALRM1 MIN---AUDIO POWER ON   7:00 AM
              .word     6000H               ;RAMn_RFTOUT x 100us ~ 2 SECS (WORD)
              .byte     01H                 ;RAMn_RF_FREQ (02=434.7, 01=433.2, 00=430.0)
=== */              
	gsd_setup.RAMn_TAGID 		= 0x8000;
	gsd_setup.RAMn_FUNCTION		= 0x1;
	gsd_setup.RAMn_MODE		= 0x1;
	gsd_setup.RAMn_ROOMNo		= 0x4D2;
	gsd_setup.RAMn_FLOORNo		= 'A';
	gsd_setup.RAMn_ALARM_SF		= 0x3;
	gsd_setup.RAMn_RF_SLOTNo	= 0x00;
	gsd_setup.RAMn_SLOT_DLYNo	= 0x00;
	gsd_setup.RAMn_DA01			= 0x225;	// Detect Threshold, for LTC1662 DAC
	gsd_setup.RAMn_VAR_LMT0		= 0x0000;  	
	gsd_setup.RAMn_VAR_LMT2		= 0x0018;
	gsd_setup.RAMn_ALRM0HR		= 0x17;	      // RTC Alarm setup hours for PWR OFF (to LPM3.5) in BCD
	gsd_setup.RAMn_ALRM1HR		= 0x06;      //  RTC Alarm setup  hours for PWR ON (from PPM3.5) in BCD 		
	gsd_setup.RAMn_HBPM_INTERVAL=0x11;  
	gsd_setup.RAMn_LED_ONFLG	= 0xFF;
	gsd_setup.RAMn_DEBUG		= 0xFF;
	gsd_setup.RAMn_BEACON_DLY	= 0x03;
	gsd_setup.RAMn_STARTUP_FLG	= 0x01;
	gsd_setup.RAMn_LED_DETECTFLG= 0xFF;
	gsd_setup.RAMn_ALRM0MIN		= 0x00;	// RTC Alarm setup Minutes for PWR OFF
	gsd_setup.RAMn_ALRM1MIN		= 0x00;     // RTC Alarm setup minutes for PWR ON
	gsd_setup.RAMn_RFTOUT		= 0x6000;         // [ADK] 11/11/2019  RTC Alarm setup for start HB communitations [hours:minutes]
	gsd_setup.RAMn_RF_FREQ		= 0x01;
// [ADK] 11/11/2019  add RTC setup for restore it at wakeup from LPM3.5
//	uint8_t		RAMn_RTC_PACK[4];		// in the packet format
	gsd_setup.RAMn_FW_VERSION[0] = (uint8_t)(GSD_FW_VERSION&0xff);
	gsd_setup.RAMn_FW_VERSION[1] = (uint8_t)((GSD_FW_VERSION>>8)&0xff);
	gsd_setup.RAMn_FW_VERSION[2] = (uint8_t)((GSD_FW_VERSION>>16)&0xff);
	gsd_setup.RAMn_FW_VERSION[3] = (uint8_t)((GSD_FW_VERSION>>24)&0xff);
}

static void vMoveCursorToBeginInput(void ) 
{
	uint8_t len = strlen(prt_buf);
	while(len--) vPrintChar('\b');
}

static void setup(void) 
{
	unsigned long inData;
	uint8_t inByte, updFlg = 0, rc;

	if (*((uint16_t *)RAM_FRAM_INFO) == 0xFFFF) {
		vPrintString("INFOD is empty"); vPrintEOL();
		default_setup();
	}else{
		vPrintString("INFOD:");
		vPrintString(psUInt16HexToString(*((uint16_t *)RAM_FRAM_INFO), prt_buf));
		vPrintEOL();
	}

	vPrintString("ID(~8000)="); vPrintString(psUInt16HexToString(gsd_setup.RAMn_TAGID, prt_buf)); vMoveCursorToBeginInput();
	rc = uart_getHexTo(&inData);
	if (rc) {
		inData &=0x7FFF;
		if (inData != gsd_setup.RAMn_TAGID) { gsd_setup.RAMn_TAGID = inData; updFlg =1; }
	}
	
	vPrintString("ROOM #(XXXX)="); vPrintString(psUInt16ToString(gsd_setup.RAMn_ROOMNo, prt_buf)); vMoveCursorToBeginInput();
	rc = uart_getHexTo(&inData);
	if (rc) {
		if (inData != gsd_setup.RAMn_ROOMNo) { gsd_setup.RAMn_ROOMNo = inData;  updFlg =1; }
	}
	
	vPrintString("ROOM DESCRIPTOR(ASCII CHAR)="); vPrintChar(gsd_setup.RAMn_FLOORNo); vPrintChar('\b');
	inByte = uart_getByte();
	if (inByte >= LETTER_A && inByte <= LETTER_Z)
	{
		if (inByte != gsd_setup.RAMn_FLOORNo)  { gsd_setup.RAMn_FLOORNo = inByte;  updFlg =1; }
	}
       vPrintEOL();

	vPrintString("DAC Threshold="); vPrintString(psUInt16HexToString(gsd_setup.RAMn_DA01, prt_buf)); vMoveCursorToBeginInput();
	rc = uart_getHexTo(&inData);
	if (rc) {
//	vPrintString("\t-> "); vPrintString(psUInt16HexToString(inData, prt_buf)); vPrintEOL();
	
		if (inData != gsd_setup.RAMn_DA01) { gsd_setup.RAMn_DA01= inData;  updFlg =1; }
	}
	
		
	vPrintString("FUNCTION:(00=RUN woHBPM, 01=RUN wHB, 02=RUN wPM, 04=RUN wHB+PM)"); vPrintEOL();
	vPrintString("         (80=DEEP SLEEP Icc=12 uA, 10=RF BEACON)="); vPrintString(psUInt8HexToString(gsd_setup.RAMn_FUNCTION, prt_buf)); vMoveCursorToBeginInput();
	rc = uart_getHexTo(&inData);
	if (rc) {
		inData &=0xFF;
		if (inData != gsd_setup.RAMn_FUNCTION) { gsd_setup.RAMn_FUNCTION= inData;  updFlg =1; }
	}
	   
	vPrintString("RUN MODE (80=SND WF HW, 40=CALB ALRM TH, 20=PNNL MODE"); vPrintEOL();
	vPrintString("          04=SND 96BYTE WF RF, 02=SND EVENT RF, 01=SND ALRM ONLY RF)="); vPrintString(psUInt8HexToString(gsd_setup.RAMn_MODE, prt_buf)); vMoveCursorToBeginInput();
	rc = uart_getHexTo(&inData);
	if (rc) {
		inData &=0xFF;
		if (inData != gsd_setup.RAMn_MODE) { gsd_setup.RAMn_MODE= inData; updFlg =1; }
	}

	vPrintString("DEBUG DSPLY FLG (00=NO DSPLY)="); vPrintString(psUInt8HexToString(gsd_setup.RAMn_DEBUG, prt_buf)); vMoveCursorToBeginInput();
	rc = uart_getHexTo(&inData);
	if (rc) {
		inData &=0xFF;
		if (inData != gsd_setup.RAMn_DEBUG) { gsd_setup.RAMn_DEBUG= inData;  updFlg =1; }
	}

	vPrintString("LED ALRM/REJECT DSPLY FLG (00=NO RED/GRN FLASH)="); vPrintString(psUInt8HexToString(gsd_setup.RAMn_LED_ONFLG, prt_buf)); vMoveCursorToBeginInput();
	rc = uart_getHexTo(&inData);
	if (rc) {
		inData &=0xFF;
		if (inData != gsd_setup.RAMn_LED_ONFLG) { gsd_setup.RAMn_LED_ONFLG= inData;  updFlg =1; }
	}
	
	vPrintString("LED DETECT DSPLY FLG (00=NO GRN FLASH)="); vPrintString(psUInt8HexToString(gsd_setup.RAMn_LED_DETECTFLG, prt_buf)); vMoveCursorToBeginInput();
	rc = uart_getHexTo(&inData);
	if (rc) {
		inData &=0xFF;
		if (inData != gsd_setup.RAMn_LED_DETECTFLG) { gsd_setup.RAMn_LED_DETECTFLG= inData;  updFlg =1; }
	}

	vPrintString("VARIANCE ALRM LMT(~0018 0000)="); 
	psUInt16HexToString(gsd_setup.RAMn_VAR_LMT2, prt_buf);
	psUInt16HexToString(gsd_setup.RAMn_VAR_LMT0, prt_buf + 4);
	vPrintString(prt_buf); vMoveCursorToBeginInput();

	rc = uart_getHexTo(&inData);
	if (rc) {
		uint16_t lmt2, lmt0;
		lmt2 = (inData >> 16);
		lmt0 = (inData&0xFFFF);
		if (lmt2 != gsd_setup.RAMn_VAR_LMT2) { gsd_setup.RAMn_VAR_LMT2 = lmt2;  updFlg =1; }
		if (lmt0 != gsd_setup.RAMn_VAR_LMT0) { gsd_setup.RAMn_VAR_LMT0 = lmt0;  updFlg =1; }
	}

	vPrintString("VARIANCE ALRM CALB SCALE FACTOR(~03)="); vPrintString(psUInt8HexToString(gsd_setup.RAMn_ALARM_SF, prt_buf)); vMoveCursorToBeginInput();
	rc = uart_getHexTo(&inData);
	if (rc) {
		inData &=0xFF;
		if (inData != gsd_setup.RAMn_ALARM_SF) { gsd_setup.RAMn_ALARM_SF= inData;  updFlg =1; }
	}

	vPrintString("RF SLOT #(00=NO DLY)="); vPrintString(psUInt8HexToString(gsd_setup.RAMn_RF_SLOTNo, prt_buf)); vMoveCursorToBeginInput();
	rc = uart_getHexTo(&inData);
	if (rc) {
		inData &=0xFF;
		if (inData != gsd_setup.RAMn_RF_SLOTNo) { gsd_setup.RAMn_RF_SLOTNo= inData;  updFlg =1; }
	}

	vPrintString("RF SLOT DLY x10ms(00=NO DLY,~01)="); vPrintString(psUInt8HexToString(gsd_setup.RAMn_SLOT_DLYNo, prt_buf)); vMoveCursorToBeginInput();
	rc = uart_getHexTo(&inData);
	if (rc) {
		inData &=0xFF;
		if (inData != gsd_setup.RAMn_SLOT_DLYNo) { gsd_setup.RAMn_SLOT_DLYNo= inData;  updFlg =1; }
	}

	vPrintString("RF BEACON DLY x1SECs(~03)="); vPrintString(psUInt8HexToString(gsd_setup.RAMn_BEACON_DLY, prt_buf)); vMoveCursorToBeginInput();
	rc = uart_getHexTo(&inData);
	if (rc) {
		inData &=0xFF;
		if (inData != gsd_setup.RAMn_BEACON_DLY) { gsd_setup.RAMn_BEACON_DLY= inData;  updFlg =1; }
	}

	vPrintString("POWER MANAGEMENT-AUDIO OFF HR (24 HR FORMAT)="); vPrintString(psUInt8HexToString(gsd_setup.RAMn_ALRM0HR, prt_buf)); vMoveCursorToBeginInput();
	rc = uart_getHexTo(&inData);
	if (rc) {
		inData &=0xFF;
		if (inData != gsd_setup.RAMn_ALRM0HR) {
			gsd_setup.RAMn_ALRM0HR= inData;  updFlg =1; 
// TODO: [ADK]  12/30/2019  we need add setup for this field !!	
//			gsd_setup.RAMn_ALRM0MIN = 0; 
		}
	}
	vPrintString("                -AUDIO OFF MIN [00-59]="); vPrintString(psUInt8HexToString(gsd_setup.RAMn_ALRM0MIN, prt_buf)); vMoveCursorToBeginInput();
	rc = uart_getHexTo(&inData);
	if (rc) {
		inData &=0xFF;
		if (inData != gsd_setup.RAMn_ALRM0MIN) { gsd_setup.RAMn_ALRM0MIN= inData;  updFlg =1; }
	}
 
	vPrintString("POWER MANAGEMENT-AUDIO ON & HBEAT HR  (24 HR FORMAT)="); vPrintString(psUInt8HexToString(gsd_setup.RAMn_ALRM1HR, prt_buf)); vMoveCursorToBeginInput();
	rc = uart_getHexTo(&inData);
	if (rc) {
		inData &=0xFF;
		if (inData != gsd_setup.RAMn_ALRM1HR) { gsd_setup.RAMn_ALRM1HR= inData;  updFlg =1; }
	}

	vPrintString("                -AUDIO ON [00-59]="); vPrintString(psUInt8HexToString(gsd_setup.RAMn_ALRM1MIN, prt_buf)); vMoveCursorToBeginInput();
	rc = uart_getHexTo(&inData);
	if (rc) {
		inData &=0xFF;
		if (inData != gsd_setup.RAMn_ALRM1MIN) { gsd_setup.RAMn_ALRM1MIN= inData;  updFlg =1; }
	}

	vPrintString("PM & HBEAT INTERVAL:"); vPrintEOL();
	vPrintString("  PM (1X=DAILY,2X=MON-FRI,NO WEEKENDS)"); vPrintEOL();
//	vPrintString("  HB (X1=DAILY,X2=MON/WED/FRI,X4=MONDAY ONLY)"); vPrintEOL();
	vPrintString("PM & HBEAT SETTING (~24)="); vPrintString(psUInt8HexToString(gsd_setup.RAMn_HBPM_INTERVAL, prt_buf)); vMoveCursorToBeginInput();
	rc = uart_getHexTo(&inData);
	if (rc) {
		inData &=0xFF;
		if (inData != gsd_setup.RAMn_HBPM_INTERVAL) { gsd_setup.RAMn_HBPM_INTERVAL= inData;  updFlg =1; }
	}

	vPrintString("RF FREQ (02=434.7, 01=433.2, 00=430.0)="); vPrintString(psUInt8HexToString(gsd_setup.RAMn_RF_FREQ, prt_buf)); vMoveCursorToBeginInput();
	rc = uart_getHexTo(&inData);
	if (rc) {
		inData &=0xFF;
		if (inData != gsd_setup.RAMn_RF_FREQ) { gsd_setup.RAMn_RF_FREQ= inData;  updFlg =1; }
	}
	
	vPrintString("STARTUP MODE FLG (00=MENU,01=RUN)="); vPrintString(psUInt8HexToString(gsd_setup.RAMn_STARTUP_FLG, prt_buf)); vMoveCursorToBeginInput();
	rc = uart_getHexTo(&inData);
	if (rc) {
		inData &=0xFF;
		if (inData != gsd_setup.RAMn_STARTUP_FLG) { gsd_setup.RAMn_STARTUP_FLG= inData;  updFlg =1; }
	}

	if (updFlg) 
		save_setup();
}

void put_setup(gsd_hb_packet_t  *hb_pkt)
{
	uint8_t updFlg = 0;
	uint8_t prevMode = gsd_setup.RAMn_MODE; 

	
	if (hb_pkt->HB_TAGID == gsd_setup.RAMn_TAGID)
	{

/*  [ADK]  06/19/2020   Excluded for now  ====
		if (gsd_setup.RAMn_FW_VERSION[0] != 0x0b)  
		{
			// Old version not supported change for ID.
			vPrintString("\t\t->NewID:"); vPrintString(psUInt16HexToString(hb_pkt->HB_ECHO, prt_buf)); vPrintEOL();
//			vPrintString("\t\t->Check for new ID"); vPrintEOL();
			
		}
=====  */		
// vPrintString("\t\t->Got a new Setup"); vPrintEOL();
		if (hb_pkt->HB_MODE 			!= gsd_setup.RAMn_MODE) 			{ gsd_setup.RAMn_MODE = hb_pkt->HB_MODE; updFlg = 1; }
// vPrintString("\t\t->Room des: pkt="); vPrintChar(hb_pkt->HB_ROOM_DSCR); vPrintString(", setup="); vPrintChar(gsd_setup.RAMn_FLOORNo);vPrintEOL();
		if (hb_pkt->HB_ROOM_DSCR	!= gsd_setup.RAMn_FLOORNo) 			{ gsd_setup.RAMn_FLOORNo = hb_pkt->HB_ROOM_DSCR; updFlg = 1; }
		if (hb_pkt->HB_ROOM_No		!= gsd_setup.RAMn_ROOMNo) 			{ gsd_setup.RAMn_ROOMNo = hb_pkt->HB_ROOM_No; updFlg = 1; }
		if (hb_pkt->HB_RF_SLOT_No	!= gsd_setup.RAMn_RF_SLOTNo) 		{ gsd_setup.RAMn_RF_SLOTNo = hb_pkt->HB_RF_SLOT_No; updFlg = 1; }
		if (hb_pkt->HB_SYS_FLG		!= gsd_setup.RAMn_STARTUP_FLG) 		{ gsd_setup.RAMn_STARTUP_FLG = hb_pkt->HB_SYS_FLG; updFlg = 1; }
		if (hb_pkt->HB_PM_INTRV		!= gsd_setup.RAMn_HBPM_INTERVAL) 	{ gsd_setup.RAMn_HBPM_INTERVAL = hb_pkt->HB_PM_INTRV; updFlg = 1; }
		if (hb_pkt->HB_AUDIO_THRSH	!= gsd_setup.RAMn_DA01) 				{ gsd_setup.RAMn_DA01 = hb_pkt->HB_AUDIO_THRSH; updFlg = 1; }
		if (hb_pkt->HB_RF_DLY		!= gsd_setup.RAMn_SLOT_DLYNo) 		{ gsd_setup.RAMn_SLOT_DLYNo = hb_pkt->HB_RF_DLY; updFlg = 1; }
		if ((prevMode&CODE_RUN_CALB_ALRM_TH) == 0)
		{
			// Do not update Alarm Threshold if we turn out of the Calibration mode
			if (hb_pkt->HB_ALRM_THRSH_LOW	!= gsd_setup.RAMn_VAR_LMT0) 		{ gsd_setup.RAMn_VAR_LMT0 = hb_pkt->HB_ALRM_THRSH_LOW; updFlg = 1; }
			if (hb_pkt->HB_ALRM_THRSH_HIGH	!= gsd_setup.RAMn_VAR_LMT2) 		{ gsd_setup.RAMn_VAR_LMT2 = hb_pkt->HB_ALRM_THRSH_HIGH; updFlg =1; }
		}
		if (updFlg) { 
			save_setup();
//			if ((gsd_setup.RAMn_MODE&CODE_RUN_LIVE_TEST) == CODE_RUN_LIVE_TEST) 
			ResetHW();  // For apply new settings ..	
		}

	}else{
		vPrintString("\t***ERROR***: Uncorrect TAGID ("); vPrintString(psUInt16HexToString(hb_pkt->HB_TAGID, prt_buf));
		vPrintString("), HB pkt ignored.");
		vPrintEOL();
	}

}

void load_setup(void) 
{
	memcpy(&gsd_setup, RAM_FRAM_INFO, sizeof(gsd_setup_t));
}

void save_setup(void) 
{
	memcpy(RAM_FRAM_INFO, &gsd_setup, sizeof(gsd_setup_t));
}

void init_setup(void)
{

	if (*((uint16_t *)RAM_FRAM_INFO) == 0xFFFF) {
//		vPrintString("INFOD is empty"); vPrintEOL();
		default_setup();
	}else{
		load_setup();
		init_version();
	}
}
void init_version(void)
{
	uint8_t upd = 0;
	if (gsd_setup.RAMn_FW_VERSION[0] != (uint8_t)(GSD_FW_VERSION&0xff))
		{ gsd_setup.RAMn_FW_VERSION[0] = (uint8_t)(GSD_FW_VERSION&0xff); upd = 1; }

	if (gsd_setup.RAMn_FW_VERSION[1] != (uint8_t)((GSD_FW_VERSION>>8)&0xff))
		{ gsd_setup.RAMn_FW_VERSION[1] = (uint8_t)((GSD_FW_VERSION>>8)&0xff); upd =1; }
	
	if (gsd_setup.RAMn_FW_VERSION[2] != (uint8_t)((GSD_FW_VERSION>>16)&0xff))
		{ gsd_setup.RAMn_FW_VERSION[2] = (uint8_t)((GSD_FW_VERSION>>16)&0xff); upd =1; }
	
	if (gsd_setup.RAMn_FW_VERSION[3] != (uint8_t)((GSD_FW_VERSION>>24)&0xff))
		{ gsd_setup.RAMn_FW_VERSION[3] = (uint8_t)((GSD_FW_VERSION>>24)&0xff); upd =1; }

	if (upd) 
		save_setup();
}

void setup_enter(void) 
{
	uint16_t cnt = 0;    
	int c = 0;

	uart_cleanRXData();
	
	while(cnt < 2000 ) 
	{
		c = 0;
		delay_ms(1);
		if (c = uart_getchar()) 
		{
			if (c == CNTRL_C || c == CNTRL_O) { // For compatibility with ASM version
				vPrintString("\tSetup mode"); vPrintEOL();
				setup();
				break;
			}else
			if (c == CNTRL_P ) { 
				uint8_t *pswd = 0xff80;
				vPrintString("\t:");
				vPrintString(psUInt8HexToString(pswd[0], prt_buf));
				vPrintString(psUInt8HexToString(pswd[1], prt_buf));
				vPrintString(psUInt8HexToString(pswd[2], prt_buf));
				vPrintString(psUInt8HexToString(pswd[3], prt_buf));
				vPrintString(psUInt8HexToString(pswd[4], prt_buf));
				vPrintString(psUInt8HexToString(pswd[5], prt_buf));
				vPrintString(psUInt8HexToString(pswd[6], prt_buf));
				vPrintString(psUInt8HexToString(pswd[7], prt_buf));
				vPrintEOL();
				pswd[0] = 0xff; pswd[1] = 0xff; pswd[2] = 0xff; pswd[3] = 0xff;
				pswd[4] = 0xff; pswd[5] = 0xff; pswd[6] = 0xff; pswd[7] = 0xff;
				break;
			}else
#if GSD_FEATURE_ENABLED(DEBUG_RTC_SETUP)
			if (c == CNTRL_S ) { // For compatibility with ASM version
//				vPrintString("\tSetup mode"); vPrintEOL();
				dbgRtcSet();
				break;
			}else
#endif //GSD_FEATURE_ENABLED(DEBUG_RTC_SETUP)
#if GSD_FEATURE_ENABLED(DEBUG_DEEP_SLEEP)
			if (c == CNTRL_X ) { 
				unsigned long inData;
				uint8_t rc;

				vPrintString("\t== Deep Sleep =="); vPrintEOL();
				vPrintString("Enter number of minutes for stay in the deep sleep [01-59]: "); vPrintEOL();
				rc = uart_getHexTo(&inData);
				if (rc == 0) {
					vPrintString("\t=== Forever ===="); vPrintEOL();
					vPrintString("\tFor wake up: Power cycle, or Reset"); vPrintEOL();
					RTC_C_disableInterrupt(RTC_C_BASE, (RTCOFIE + RTCTEVIE + RTCAIE + RTCRDYIE));
       				RTC_C_clearInterrupt(RTC_C_BASE, (RTC_C_TIME_EVENT_INTERRUPT + RTC_C_CLOCK_ALARM_INTERRUPT + RTC_C_CLOCK_READ_READY_INTERRUPT + RTC_C_OSCILLATOR_FAULT_INTERRUPT)); 
				} else{
					vPrintString("\tDisconnect Serial Console and MSP-FET Programmer."); vPrintEOL();
					rtc_set_fake_time();
					rtc_set_alarm(0x00, (uint8_t)(inData&0xFF));
					rtc_enable_alarm();
				}
				EnterLPM35_cmd();
				break;
			}else
#endif //GSD_FEATURE_ENABLED(DEBUG_DEEP_SLEEP)
				cnt++;
		}else
			cnt++;
	}
	vPrintString("\tRun mode"); vPrintEOL();
}
