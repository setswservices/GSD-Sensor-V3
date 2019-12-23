/*
 * rtc_c.c
 *
 *  Created on: Nov 5, 2019
 *      Author: dantonets
 */

#include "stdio.h"
#include "string.h"
#include "file.h"
#include "driverlib.h"
#include "gsd.h"

#define TM_WDAY_SUN 0
#define TM_WDAY_MON 1
#define TM_WDAY_TUE 2
#define TM_WDAY_WED 3
#define TM_WDAY_THU 4
#define TM_WDAY_FRI 5
#define TM_WDAY_SAT 6

#define TM_MON_JAN  0
#define TM_MON_FEB  1
#define TM_MON_MAR  2
#define TM_MON_APR  3
#define TM_MON_MAY  4
#define TM_MON_JUN  5
#define TM_MON_JUL  6
#define TM_MON_AUG  7
#define TM_MON_SEP  8
#define TM_MON_OCT  9
#define TM_MON_NOV 10
#define TM_MON_DEC 11

extern uint8_t  gsd_rtc_event;

static   Calendar currentTime;
static RTC_C_configureCalendarAlarmParam  alrm_params;

//#pragma PERSISTENT(TM_MON_DAYS_ACCU)
const static int16_t TM_MON_DAYS_ACCU[12] = {
    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};
//#pragma PERSISTENT(week_day)
const static char* week_day[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};   
#if GSD_FEATURE_ENABLED(DEBUGGING_MENU)
static uint8_t rtc_alarm_intr_flag = 0;
static uint8_t rtc_minutes_prev = 0;
#endif // GSD_FEATURE_ENABLED(DEBUGGING_MENU)

 
static int tm_is_leap_year(unsigned year) {
    return ((year & 3) == 0) && ((year % 400 == 0) || (year % 100 != 0));
}

// The "Doomsday" the the day of the week of March 0th,
// i.e the last day of February.
// In common years January 3rd has the same day of the week,
// and on leap years it's January 4th.
static int tm_doomsday(int year) {
    int result;
    result  = TM_WDAY_TUE;
    result += year;       // I optimized the calculation a bit:
    result += year >>= 2; // result += year / 4
    result -= year /= 25; // result += year / 100
    result += year >>= 2; // result += year / 400
    return result;
}

static void tm_get_wyday(void)
{
    int yday;
    uint16_t wday;	
    int mon = RTC_C_convertBCDToBinary(RTC_C_BASE, currentTime.Month -1);
    int mday = RTC_C_convertBCDToBinary(RTC_C_BASE, currentTime.DayOfMonth);
    int year = RTC_C_convertBCDToBinary(RTC_C_BASE, currentTime.Year);
    int is_leap_year = tm_is_leap_year(year);
    // How many days passed since Jan 1st?
    yday = TM_MON_DAYS_ACCU[mon] + mday + (mon <= TM_MON_FEB ? 0 : is_leap_year) - 1;
    // Which day of the week was Jan 1st of the given year?
    int jan1 = tm_doomsday(year) - 2 - is_leap_year;
    // Now just add these two values.
    wday = (uint16_t)((jan1 + yday) % 7);
    currentTime.DayOfWeek = RTC_C_convertBinaryToBCD(RTC_C_BASE, wday);
}

void rtc_init(void) 
{
    //Start RTC Clock
    RTC_C_disableInterrupt(RTC_C_BASE, (RTCOFIE + RTCTEVIE + RTCAIE + RTCRDYIE));
    RTC_C_clearInterrupt(RTC_C_BASE, (RTC_C_TIME_EVENT_INTERRUPT + RTC_C_CLOCK_ALARM_INTERRUPT + RTC_C_CLOCK_READ_READY_INTERRUPT + RTC_C_OSCILLATOR_FAULT_INTERRUPT)); 
    RTC_C_startClock(RTC_C_BASE);
}

void rtc_load(void) 
{
	RTC_C_holdClock (RTC_C_BASE);
	RTC_C_initCalendar(RTC_C_BASE, &currentTime, RTC_C_FORMAT_BCD);
    	RTC_C_startClock(RTC_C_BASE);
}

// All rtc_set_*() need imput in BCD!
void rtc_set_sec(uint8_t sec)
	{ currentTime.Seconds = sec; }
void rtc_set_min(uint8_t min)
	{ currentTime.Minutes = min; }
void rtc_set_hours(uint8_t hours) 
	{ currentTime.Hours = hours; }
void rtc_set_day(uint8_t day)
	{ currentTime.DayOfMonth = day; }
void rtc_set_dow(uint8_t day)
	{ currentTime.DayOfWeek= day; }
void rtc_set_month(uint8_t month) 
	{ currentTime.Month = month; }
void rtc_set_year(uint16_t year) 
	{ currentTime.Year= year; }
void rtc_enable_alarm(void) 
	{ 	RTC_C_enableInterrupt(RTC_C_BASE, RTCAIE); }
void rtc_disable_alarm(void) 
{
		RTC_C_disableInterrupt(RTC_C_BASE, RTCAIE);
		RTC_C_clearInterrupt(RTC_C_BASE, RTC_C_CLOCK_ALARM_INTERRUPT);
}


void rtc_set_alarm(uint8_t bcd_hr, uint8_t bcd_min)
{
	memset(&alrm_params, 0, sizeof(RTC_C_configureCalendarAlarmParam));
	alrm_params.dayOfWeekAlarm = 0x80;
	alrm_params.dayOfMonthAlarm= 0x80;
	alrm_params.hoursAlarm= 0x80;
	alrm_params.minutesAlarm= 0x80;
	
	RTC_C_configureCalendarAlarm(RTC_C_BASE, &alrm_params);
	RTC_C_disableInterrupt(RTC_C_BASE, (RTCOFIE + RTCTEVIE + RTCAIE + RTCRDYIE));
       RTC_C_clearInterrupt(RTC_C_BASE, (RTC_C_TIME_EVENT_INTERRUPT + RTC_C_CLOCK_ALARM_INTERRUPT + RTC_C_CLOCK_READ_READY_INTERRUPT + RTC_C_OSCILLATOR_FAULT_INTERRUPT)); 
	alrm_params.hoursAlarm= bcd_hr;
	alrm_params.minutesAlarm= bcd_min;
	RTC_C_configureCalendarAlarm(RTC_C_BASE, &alrm_params);


}
	

#if GSD_FEATURE_ENABLED(DEBUG_SERIAL_PORT)
// Print from the currentTime!!
void printDateTime(void)
{
	vPrintEOL();
	vPrintString("\t");
	vPrintString(psUInt16ToString(RTC_C_convertBCDToBinary(RTC_C_BASE, currentTime.Month), prt_buf));
	vPrintString("/");
	vPrintString(psUInt16ToString(RTC_C_convertBCDToBinary(RTC_C_BASE, currentTime.DayOfMonth), prt_buf));
	vPrintString("/");
	vPrintString(psUInt16ToString(RTC_C_convertBCDToBinary(RTC_C_BASE, currentTime.Year), prt_buf));
	vPrintString(" ");

	vPrintString(week_day[RTC_C_convertBCDToBinary(RTC_C_BASE, currentTime.DayOfWeek)]);
	vPrintString(" ");
	
	vPrintString(psUInt16ToString(RTC_C_convertBCDToBinary(RTC_C_BASE, currentTime.Hours), prt_buf));
	vPrintString(":");
	vPrintString(psUInt16ToString(RTC_C_convertBCDToBinary(RTC_C_BASE, currentTime.Minutes), prt_buf));
	vPrintString(":");
	vPrintString(psUInt16ToString(RTC_C_convertBCDToBinary(RTC_C_BASE, currentTime.Seconds), prt_buf));
       vPrintEOL();

}

void rtc_get(void)
{
	currentTime = RTC_C_getCalendarTime(RTC_C_BASE);
	if (rtc_minutes_prev != currentTime.Minutes) {
		printDateTime();
		rtc_minutes_prev = currentTime.Minutes;
	}
}
#endif //GSD_FEATURE_ENABLED(DEBUG_SERIAL_PORT)

#if GSD_FEATURE_ENABLED(DEBUGGING_MENU)


int menuRtcPrint(void *menu, void *item, void *args)
{
	currentTime = RTC_C_getCalendarTime(RTC_C_BASE);
	printDateTime();

	return 0;
}

int menuRtcSet(void *menu, void *item, void *args)
{
	unsigned long tmp;
	uint16_t  bin_val, bcd_val;	

	vPrintEOL();
	vPrintString("Enter"); vPrintEOL();
	vPrintString("\tYear: ");

	tmp = uart_getInt();
	bin_val = (uint16_t)(tmp&0xFFFF);
	bcd_val = RTC_C_convertBinaryToBCD (RTC_C_BASE, bin_val);
	rtc_set_year(bcd_val);

	while(1) 
	{
		vPrintString("\tMonth [1-12]: ");
		tmp = uart_getInt();
		bin_val = (uint16_t)(tmp&0xFFFF);
		if (bin_val >= 1 && bin_val <=12) break;  
	}
	bcd_val = RTC_C_convertBinaryToBCD (RTC_C_BASE, bin_val);
	rtc_set_month(bcd_val);
	
	while(1) 
	{
		vPrintString("\tDay [1-31]: ");
		tmp = uart_getInt();
		bin_val = (uint16_t)(tmp&0xFFFF);
		if (bin_val >= 1 && bin_val <=31) break;  
	}
	bcd_val = RTC_C_convertBinaryToBCD (RTC_C_BASE, bin_val);
	rtc_set_day(bcd_val);
	
	while(1) 
	{
		vPrintString("\tHours [0-23]: ");
		tmp = uart_getInt();
		bin_val = (uint16_t)(tmp&0xFFFF);
		if (bin_val <=23) break;  
	}
	bcd_val = RTC_C_convertBinaryToBCD (RTC_C_BASE, bin_val);
	rtc_set_hours(bcd_val);

	while(1) 
	{
		vPrintString("\tMinutes [0-59]: ");
		tmp = uart_getInt();
		bin_val = (uint16_t)(tmp&0xFFFF);
		if (bin_val <=59) break;  
	}
	bcd_val = RTC_C_convertBinaryToBCD (RTC_C_BASE, bin_val);
	rtc_set_min(bcd_val);

	tm_get_wyday();
	currentTime.Seconds = 0;

	vPrintEOL();
	vPrintString("\tUse CRTL_L for load RTC with the following date/time:"); 
	printDateTime();
	
	return 0;
}

int menuRtcLoad(void *menu, void *item, void *args)
{
	rtc_load();
	return 0;
}
/*
	NOTE: Setting the alarm
	Before setting an initial alarm, all alarm registers including the AE bits should be cleared.
	
	To prevent potential erroneous alarm conditions from occurring, the alarms should be
	disabled by clearing the RTCAIE, RTCAIFG, and AE bits before writing initial or new time
	values to the RTC time registers.
*/	
int menuRtcSetAlrm(void *menu, void *item, void *args)
{
	unsigned long tmp;
	uint16_t  bin_val, bcd_val;	

	memset(&alrm_params, 0, sizeof(RTC_C_configureCalendarAlarmParam));
	alrm_params.dayOfWeekAlarm = 0x80;
	alrm_params.dayOfMonthAlarm= 0x80;
	alrm_params.hoursAlarm= 0x80;
	alrm_params.minutesAlarm= 0x80;
	
	RTC_C_configureCalendarAlarm(RTC_C_BASE, &alrm_params);
	RTC_C_disableInterrupt(RTC_C_BASE, (RTCOFIE + RTCTEVIE + RTCAIE + RTCRDYIE));
       RTC_C_clearInterrupt(RTC_C_BASE, (RTC_C_TIME_EVENT_INTERRUPT + RTC_C_CLOCK_ALARM_INTERRUPT + RTC_C_CLOCK_READ_READY_INTERRUPT + RTC_C_OSCILLATOR_FAULT_INTERRUPT)); 
	
	vPrintEOL();
	vPrintString("Eneter alarm\r\n\tDay of week: ");
	tmp = uart_getInt();
	if (tmp) {
		bin_val = (uint16_t)(tmp&0xFFFF);
		bcd_val = RTC_C_convertBinaryToBCD (RTC_C_BASE, bin_val);
		alrm_params.dayOfWeekAlarm = bcd_val;
	}else
		alrm_params.dayOfWeekAlarm = 0x80;
	
	vPrintString("\tDay of month: ");
	tmp = uart_getInt();
	if (tmp) {
		bin_val = (uint16_t)(tmp&0xFFFF);
		bcd_val = RTC_C_convertBinaryToBCD (RTC_C_BASE, bin_val);
		alrm_params.dayOfMonthAlarm= bcd_val;
	}else
		alrm_params.dayOfMonthAlarm= 0x80;
	
	vPrintString("\tHours: ");
	tmp = uart_getInt();
	if (tmp) {
		bin_val = (uint16_t)(tmp&0xFFFF);
		bcd_val = RTC_C_convertBinaryToBCD (RTC_C_BASE, bin_val);
		alrm_params.hoursAlarm= bcd_val;
	}else
		alrm_params.hoursAlarm= 0x80;
	
	vPrintString("\tMinutes: ");
	tmp = uart_getInt();
	if (tmp) {
		bin_val = (uint16_t)(tmp&0xFFFF);
		bcd_val = RTC_C_convertBinaryToBCD (RTC_C_BASE, bin_val);
		alrm_params.minutesAlarm= bcd_val;
	}else
		alrm_params.minutesAlarm= 0x80;

	rtc_alarm_intr_flag =0;
	RTC_C_configureCalendarAlarm(RTC_C_BASE, &alrm_params);

	vPrintEOL();
	vPrintString("Use CNTRL_B for checkout the alarm event");
	vPrintEOL();
	return 0;

}

int menuRtcLPM(void *menu, void *item, void *args)
{

	RTC_C_enableInterrupt(RTC_C_BASE, RTCAIE);
	return 1;

}

int menuRtcChkAlrm(void *menu, void *item, void *args)
{
	int cnt = 0;

	vPrintString("Hit ESC if no RTC alarm occured .."); vPrintEOL();

	RTC_C_enableInterrupt(RTC_C_BASE, RTCAIE);
	while(1) 
	{
		delay_ms(2);
		if (rtc_alarm_intr_flag) 
		{
			RTC_C_disableInterrupt(RTC_C_BASE, RTCAIE);
			RTC_C_clearInterrupt(RTC_C_BASE, RTC_C_CLOCK_ALARM_INTERRUPT);
			vPrintString("\tGot ALARM!"); vPrintEOL();
			break;
		}
		if (uart_getchar() == CHAR_ESC) 
		{
			RTC_C_disableInterrupt(RTC_C_BASE, RTCAIE);
			RTC_C_clearInterrupt(RTC_C_BASE, RTC_C_CLOCK_ALARM_INTERRUPT);
			vPrintString("\tNo ALARM .."); vPrintEOL();
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
#endif // GSD_FEATURE_ENABLED(DEBUGGING_MENU)

/* *** Date/Time convert routines ****
There are two Date/Time formats use in the FW in additions to the BCD Calendar:
	- I2C EEPROM Format
	- Packet format for send/receive to/from outside. 
**** From 17_rtc.ASM *****
;RAMx_I2C:
;	   +06=DAY WEEK		(0-6)	SUNDAY=00	*
;        +05=YEAR			(0-99)	BCD FORMATS	*                                                                               
;        +04=MONTH		(1-12)				*                                                                               
;        +03=DAY			(1-31)				*                                                                               
;        +02=HOUR			(0-23)				*                                                                               
;        +01=MINUTES		(0-59)				*                                                                               
;        +00=SECS			(0-59)				*                                                                               
;                                                              		*
;PACKET FORMAT (INCLUDE MONTH & YEAR TO 2063)	*
;       +03:     (YR5 YR4 YR3 DY4 DY3 DY2 DY1 DY0)		*                                                              
;       +02:     (YR2 YR1 YR0 HR4 HR3 HR2 HR1 HR0)		*                                                                          
;       +01:     (MO3 MO2 MN5 MN4 MN3 MN2 MN1 MN0)	*                                                                          
;       +00:     (MO1 MO0 SC5 SC4 SC3 SC2 SC1 SC0)		*                                                                          
;                                                              			*
-------------------------------------------- */
// Need load RTC from currentTime *after* call for this function.
void eeprom2calendar(uint8_t *eeprom)
{
	if (eeprom == NULL)  return;
	rtc_set_sec(eeprom[0]);
	rtc_set_min(eeprom[1]);
	rtc_set_hours(eeprom[2]);
	rtc_set_day(eeprom[3]);
	rtc_set_month(eeprom[4]);
	rtc_set_year((uint16_t)(0x2000 + eeprom[5]));
	rtc_set_dow(eeprom[6]);
}

// Need load RTC Alarm from alrm_params *after* call for this function, 
// and enable alarms.
void eeprom2alarm(uint8_t *eeprom)
{
	if (eeprom == NULL)  return;
	memset(&alrm_params, 0, sizeof(RTC_C_configureCalendarAlarmParam));
	alrm_params.dayOfWeekAlarm = 0x80;
	alrm_params.dayOfMonthAlarm= 0x80;
	alrm_params.hoursAlarm= 0x80;
	alrm_params.minutesAlarm= 0x80;
	
	alrm_params.dayOfWeekAlarm = eeprom[ 6];
	alrm_params.dayOfMonthAlarm = eeprom[3];
	alrm_params.hoursAlarm= eeprom[2];
	alrm_params.minutesAlarm= eeprom[1];

}

// Need load currentTime from RTC *before* call this function. 
uint8_t *calendar2eeprom(uint8_t *eeprom)
{
	if (eeprom == NULL)  return NULL;
	eeprom[0] = currentTime.Seconds;
	eeprom[1] = currentTime.Minutes;
	eeprom[2] = currentTime.Hours;
	eeprom[3] = currentTime.DayOfMonth;
	eeprom[4] = currentTime.Month;
	eeprom[5] = currentTime.Year;
	eeprom[6] = currentTime.DayOfWeek;
	
	return eeprom;
}

// Need load RTC from currentTime *after* call for this function.
void packet2calendar(uint8_t *packet)
{
	uint16_t  bin_val, bcd_val;	

	if (packet == NULL) return;
	bin_val = (packet[0]&0x3F);
	bcd_val = RTC_C_convertBinaryToBCD (RTC_C_BASE, bin_val);
	rtc_set_sec((uint8_t)(bcd_val&0xFF));

	bin_val = (packet[1]&0x3F);
	bcd_val = RTC_C_convertBinaryToBCD (RTC_C_BASE, bin_val);
	rtc_set_min((uint8_t)(bcd_val&0xFF));

	bin_val = (packet[2]&0x1F);
	bcd_val = RTC_C_convertBinaryToBCD (RTC_C_BASE, bin_val);
	rtc_set_hours((uint8_t)(bcd_val&0xFF));

	bin_val = (packet[2]&0x1F);
	bcd_val = RTC_C_convertBinaryToBCD (RTC_C_BASE, bin_val);
	rtc_set_hours((uint8_t)(bcd_val&0xFF));
	
	bin_val = (packet[3]&0x1F);
	bcd_val = RTC_C_convertBinaryToBCD (RTC_C_BASE, bin_val);
	rtc_set_day((uint8_t)(bcd_val&0xFF));

	bin_val = (((packet[0]&0xC0) >>6) |(((packet[1]&0xC0 )>>4))) ;
	bcd_val = RTC_C_convertBinaryToBCD (RTC_C_BASE, bin_val);
	rtc_set_month((uint8_t)(bcd_val&0xFF));

	bin_val = (((packet[ 2]&0xE0) >>5) |(((packet[3]&0xE0 )>>2)));
	bin_val += 2000;
	bcd_val = RTC_C_convertBinaryToBCD (RTC_C_BASE, bin_val);
	rtc_set_year(bcd_val);

	tm_get_wyday();
	
}

// Need load currentTime from RTC *before* call this function. 
uint8_t * calendar2packet(uint8_t *packet)
{
	uint16_t  bin_val;
	if (packet == NULL)  return NULL;
	
	bin_val = RTC_C_convertBCDToBinary(RTC_C_BASE, currentTime.Seconds);
	packet[0] = (uint8_t)(bin_val&0x3F);

	bin_val = RTC_C_convertBCDToBinary(RTC_C_BASE, currentTime.Minutes);
	packet[1] = (uint8_t)(bin_val&0x3F);

	bin_val = RTC_C_convertBCDToBinary(RTC_C_BASE, currentTime.Hours);
	packet[2] = (uint8_t)(bin_val&0x1F);

	bin_val = RTC_C_convertBCDToBinary(RTC_C_BASE, currentTime.DayOfMonth);
	packet[3] = (uint8_t)(bin_val&0x1F);

	bin_val = RTC_C_convertBCDToBinary(RTC_C_BASE, currentTime.Month);
	packet[0] |= (uint8_t)((bin_val&0x03) << 6);
	packet[1] |= (uint8_t)((bin_val&0x0C) << 4);

	bin_val = RTC_C_convertBCDToBinary(RTC_C_BASE, currentTime.Year);
	if (bin_val > 2000) bin_val-=2000;
	packet[2] |= (uint8_t)((bin_val&0x07) << 5);
	packet[3] |= (uint8_t)((bin_val&0x38) << 2);

	return packet;
	
}

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=RTC_VECTOR
__interrupt
#elif defined(__GNUC__)
__attribute__((interrupt(RTC_VECTOR)))
#endif
void RTC_ISR (void)
{
	LPM3_EXIT;
	 switch (__even_in_range(RTCIV, 16)) {
        case RTCIV_NONE: break;  //No interrupts
        case RTCIV_RTCOFIFG: break;  //RTCOFIFG
        case RTCIV_RTCRDYIFG:         //RTCRDYIFG
                __no_operation();
                break;
        case RTCIV_RTCTEVIFG:         //RTCEVIFG
                __no_operation();
                break;
        case RTCIV_RTCAIFG:         //RTCAIFG
        		gsd_rtc_event = GSD_HAVE_EVENT;
//#if GSD_FEATURE_ENABLED(DEBUGGING_MENU)
 //       		rtc_alarm_intr_flag = 1;
//#endif //GSD_FEATURE_ENABLED(DEBUGGING_MENU)
                break;
        case RTCIV_RT0PSIFG: break; //RT0PSIFG
        case RTCIV_RT1PSIFG: break; //RT1PSIFG

        default: break;
    }
}

