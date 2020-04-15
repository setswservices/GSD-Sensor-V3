/*
 * nrf905.c
 *
 *  Created on: Nov 12, 2019
 *      Author: dantonets
 */

#include "stdio.h"
#include "string.h"
#include "file.h"
#include "driverlib.h"
#include "gsd.h"

// _/_/_/ Commands (see page 17, section 6.2) _/_/_/
#define nRF905_W_CONFIG_CMD				(0x00)
#define nRF905_R_CONFIG_CMD				(0x10)
#define nRF905_W_TX_PAYLOAD_CMD			(0x20)
#define nRF905_W_TX_ADDR_CMD				(0x22)
#define nRF905_R_TX_ADDR_CMD				(0x23)
#define nRF905_R_RX_PAYLOAD_CMD			(0x24)

#define nRF905_MODE_STANDBY				(0x10)
#define nRF905_MODE_RX						(0x40)
#define nRF905_MODE_TX						(0x80)



//#pragma pack(push, 4)
typedef struct {
	uint16_t			ch_no;		// 0 - use default value	(108)	
	uint8_t			hfreq_pll;	// 0 - 433MHz
	uint8_t			pa_pwr;		// 2 - +6dBm
	uint8_t			rx_red_pwr;
	uint8_t			auto_retran;
	uint8_t			rx_afw;		// 4 - 4 byte
	uint8_t			tx_afw;		// 4 - 4 bytes
	uint8_t 			rx_pw;		// 0x20 - use default value	(32)
	uint8_t 			tx_pw;		// 0x20- use default value	(32)
	uint16_t 			rx_address_low;
	uint16_t 			rx_address_high;	// 0 - use defailt address	(0xe7e7e7e7)
	uint8_t 			xof;			// 0 - use default value	(20MHz)
	uint8_t			crc_en;
	uint8_t			crc_mode;
} nRF905Config_t, *pnRF905Config;

typedef struct {
	uint8_t cmd;
	uint8_t  cfg[12];	
} nRF905_cfg_buf;
// #pragma pack(pop)

extern gsd_setup_t gsd_setup;
extern uint8_t  gsd_tx_done_event;
extern uint8_t  gsd_rx_packet_event;
extern uint8_t  gsd_tx_packet_type;

#pragma PERSISTENT(nRF905_mode)
static uint8_t nRF905_mode;
// [ADK] Uset it unlil we'll have acces to the  EEPROM ..
#pragma PERSISTENT(dt_power_on)
static uint8_t dt_power_on[2];     // [0] = HR, [1]=MIN

#pragma PERSISTENT(nRF905_cfg)
static nRF905_cfg_buf nRF905_cfg;
#pragma PERSISTENT(nRF905_conf)
static nRF905Config_t nRF905_conf;
/* static */ uint8_t *spi_rx_ptr;
/* static */ uint8_t spi_rx_idx;
//#pragma DATA_ALIGN(xbuf, 2)
static uint8_t  send_wf_flag = 0;
static uint16_t  rf_dly = 400;
static uint16_t  xmitsNo = 0;

uint8_t xbuf[33];
#if GSD_FEATURE_ENABLED(DEBUGGING_MENU)
static void nRF905_DumpConfig(uint8_t *show_cfg);
#endif // GSD_FEATURE_ENABLED(DEBUGGING_MENU)
static void nRF905_Configure(void);

// #pragma FUNCTION_OPTIONS ( send_byte, "--opt_level=0" )
static void sendByte(uint8_t *byte)
{
	uint8_t idx = 7;
	uint16_t baseAddress = __MSP430_BASEADDRESS_PORT3_R__;
	uint16_t SCK_Pin = GPIO_PIN0;
	uint16_t MOSI_Pin = GPIO_PIN1;
	
    // Shift by 8 if port is even (upper 8-bits)
    if((GPIO_PORT_P3 & 1) ^ 1) {
        SCK_Pin <<= 8;
	 MOSI_Pin <<=8;	
    }

    	HWREG16(baseAddress + OFS_PAOUT) &= ~(SCK_Pin | MOSI_Pin);
//    	HWREG16(baseAddress + OFS_PAOUT) &= ~MOSI_Pin;

	while(1)
	{
		if (((*byte) >> idx)&0x01)
    			HWREG16(baseAddress + OFS_PAOUT) |= MOSI_Pin ;
		else 
    			HWREG16(baseAddress + OFS_PAOUT) &= (~MOSI_Pin);

    		HWREG16(baseAddress + OFS_PAOUT) ^= SCK_Pin;
    		_delay_cycles(2);
    		HWREG16(baseAddress + OFS_PAOUT) ^= SCK_Pin;
 //   		HWREG16(baseAddress + OFS_PAOUT) &= ~MOSI_Pin;
		if (idx-- == 0) break;
	
	}
    		HWREG16(baseAddress + OFS_PAOUT) &= ~(SCK_Pin | MOSI_Pin);
//    	HWREG16(baseAddress + OFS_PAOUT) &= ~SCK_Pin;
//    	HWREG16(baseAddress + OFS_PAOUT) &= ~MOSI_Pin;

}

static uint8_t readByte(void)
{
	uint8_t idx = 8, data = 0x00;
	uint16_t baseAddress = __MSP430_BASEADDRESS_PORT3_R__;
	uint16_t SCK_Pin = GPIO_PIN0;
	uint16_t MISO_Pin = GPIO_PIN2;
	
    // Shift by 8 if port is even (upper 8-bits)
    if((GPIO_PORT_P3 & 1) ^ 1) {
        SCK_Pin <<= 8;
	 MISO_Pin <<=8;	
    }

    	HWREG16(baseAddress + OFS_PAOUT) &= ~SCK_Pin;

	while(idx)
	{
		idx--;
    		HWREG16(baseAddress + OFS_PAOUT) ^= SCK_Pin;
    		_delay_cycles(1);
		if (HWREG16(baseAddress + OFS_PAIN) & (MISO_Pin))
    			data |= (1<<idx);
    		HWREG16(baseAddress + OFS_PAOUT) ^= SCK_Pin;
	
	}
    	HWREG16(baseAddress + OFS_PAOUT) &= ~SCK_Pin;
	return data;	
}

static void transactionDone(void)
{
#if 0
	uint16_t baseAddress = __MSP430_BASEADDRESS_PORT5_R__;
	uint16_t CS_Pin = GPIO_PIN3;
    // Shift by 8 if port is even (upper 8-bits)

    if((GPIO_PORT_P5 & 1) ^ 1) {
        CS_Pin <<= 8;
    }
    HWREG16(baseAddress + OFS_PAOUT) ^= CS_Pin;
    _delay_cycles(1);
    HWREG16(baseAddress + OFS_PAOUT) ^= CS_Pin;
#endif
       GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN3);   // Set RF_CS high 
       GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN3);   // Set RF_CS low 

}
	
static void readBytes(uint16_t len) 
{
	spi_rx_idx = 0;
	while(1) {
		if (spi_rx_idx >= len) break;
		spi_rx_idx++;	
		spi_rx_ptr[ spi_rx_idx] = readByte();
	}
}

static void WriteBytes(uint16_t len) 
{
	spi_rx_idx = 0;
	while(1) {
		if (spi_rx_idx >= len) break;
		sendByte(&spi_rx_ptr[ spi_rx_idx]);
		spi_rx_idx++;	
	}
}

static void nRF905_ReadConfig(void)
{
	//USCI_B1 TX buffer ready?
//    	while (!EUSCI_B_SPI_getInterruptStatus(EUSCI_B1_BASE, EUSCI_B_SPI_TRANSMIT_INTERRUPT)) ;

	nRF905_cfg.cmd = nRF905_R_CONFIG_CMD;
	spi_rx_ptr = (uint8_t *)&nRF905_cfg.cmd;
/* [ADK] 11/15/2019   -- use bitbang
	//Transmit CMD to nRF905
   	EUSCI_B_SPI_transmitData(EUSCI_B1_BASE, cmd);
*/
	transactionDone();
	sendByte(spi_rx_ptr);
	spi_rx_ptr = (uint8_t *)&nRF905_cfg.cfg;
	readBytes(10);
	transactionDone();
}

static void nRF905_WriteConfig(void)
{
	nRF905_cfg.cfg[0]= nRF905_W_CONFIG_CMD;
	spi_rx_ptr = (uint8_t *)&nRF905_cfg.cfg;
	transactionDone();
	WriteBytes(11);
	transactionDone();
}

static void nRF905_ReadData(void)
{
	nRF905_cfg.cmd = nRF905_R_RX_PAYLOAD_CMD;
	spi_rx_ptr = (uint8_t *)&nRF905_cfg.cmd;

 	transactionDone();
	sendByte(spi_rx_ptr);
	spi_rx_ptr = xbuf;
	readBytes(33);
	transactionDone();
	
}

static void nRF905_pwr_on(void)
{

     	GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN0);
     	GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN1);
	GPIO_setOutputHighOnPin (	GPIO_PORT_P4, GPIO_PIN3);  // RF_on -> HIGH
	delay_ms(1);

	// to Standby mode ..
	GPIO_setOutputLowOnPin(    	GPIO_PORT_P5, GPIO_PIN1);  //  RF_TX_CE -> LOW
	GPIO_setOutputLowOnPin(    	GPIO_PORT_P5, GPIO_PIN0);  // RF_TX_EN -> LOW
	GPIO_setOutputHighOnPin(    	GPIO_PORT_P5, GPIO_PIN2);  //  RF_PWRUP  -> HIGH
	//Need  delay 3 msec before switch to RX/TX modes
	delay_ms(3);

}

static void nRF905_TxStart(void)
{
	GPIO_setOutputLowOnPin(    	GPIO_PORT_P5, GPIO_PIN1);  //  RF_TX_CE -> LOW
	GPIO_setOutputHighOnPin(    	GPIO_PORT_P5, GPIO_PIN0);  // RF_TX_EN -> HIGH
	GPIO_setOutputHighOnPin(    	GPIO_PORT_P5, GPIO_PIN2);  //  RF_PWRUP  -> HIGH
	//Need  delay 3 msec before switch to RX/TX modes
	delay_ms(3);
}


static void load_wf(uint8_t *dst, unsigned long src )
{
	uint8_t idx;
	for (idx=0; idx < 21; idx++) 
	{
		*dst = __data20_read_char(src);
		dst++; src++;
	}
}

	
void nRF905_pwr_off(void)
{
	GPIO_setOutputLowOnPin(    	GPIO_PORT_P5, GPIO_PIN0 + GPIO_PIN1 + GPIO_PIN2);  // RF_TX_EN, RF_TX_CE, RF_PWRUP
	GPIO_setOutputLowOnPin (	GPIO_PORT_P4, GPIO_PIN3);

}

void nRF905_TxDone(void)
{
	uint8_t rc;
	
	GPIO_disableInterrupt( GPIO_PORT_P3, GPIO_PIN6);
	GPIO_setOutputLowOnPin(    	GPIO_PORT_P5, GPIO_PIN0);  //  RF_TX_EN -> LOW
	GPIO_setOutputLowOnPin(    	GPIO_PORT_P5, GPIO_PIN2);  //  RF_PWRUP  -> LOW
	nRF905_mode = nRF905_MODE_STANDBY;
	do {
 		delay_ms(1);
 		rc = GPIO_getInputPinValue(GPIO_PORT_P3, GPIO_PIN6);   // Until DR -> LOW
	}while(rc);
}

void nRF905_RxStart(void)
{
//	uint8_t cd;

	GPIO_selectInterruptEdge( GPIO_PORT_P3, GPIO_PIN6, GPIO_LOW_TO_HIGH_TRANSITION);
    	GPIO_clearInterrupt( GPIO_PORT_P3, GPIO_PIN6);

	nRF905_mode = nRF905_MODE_RX;

	GPIO_setOutputHighOnPin(    	GPIO_PORT_P5, GPIO_PIN2);  //  RF_PWRUP  -> HIGH
	nRF905_Configure();
 	GPIO_setOutputLowOnPin(    	GPIO_PORT_P5, GPIO_PIN0);  // RF_TX_EN -> LOW
	GPIO_enableInterrupt( GPIO_PORT_P3, GPIO_PIN6);
	GPIO_setOutputHighOnPin(    	GPIO_PORT_P5, GPIO_PIN1);  //  RF_TX_CE -> HIGH

	
/*
	do {
		cd = GPIO_getInputPinValue(GPIO_PORT_P3, GPIO_PIN7);
		if (cd) { 
			vPrintString("Got CD .."); vPrintEOL();
			break;
		}
		delay_ms(1);
	} while(1);
*/

}

void nRF905_RxDone(void)
{
    	GPIO_clearInterrupt( GPIO_PORT_P3, GPIO_PIN6);
	GPIO_disableInterrupt( GPIO_PORT_P3, GPIO_PIN6);
	GPIO_setOutputLowOnPin(    	GPIO_PORT_P5, GPIO_PIN1);  //  RF_TX_CE -> LOW
	GPIO_setOutputLowOnPin(    	GPIO_PORT_P5, GPIO_PIN2);  //  RF_PWRUP  -> LOW
	nRF905_mode = nRF905_MODE_STANDBY;

}

void nRF905_Rx(void)
{
#if 0
//#if GSD_FEATURE_ENABLED(DEBUG_SERIAL_PORT)
	char prt[32];
	uint8_t idx;
#endif //GSD_FEATURE_ENABLED(DEBUG_SERIAL_PORT)

	nRF905_ReadData();
	nRF905_RxDone();

#if 0
//#if GSD_FEATURE_ENABLED(DEBUG_SERIAL_PORT)
	for (idx =0; idx < 6; idx++) {
		sprintf(prt, "%02x%02x%02x%02x%02x", xbuf[(idx*5) +1], xbuf[(idx*5) +2], xbuf[(idx*5) +3], xbuf[(idx*5) +4], xbuf[(idx*5) +5]);
		printf("\t%s\r\n", prt);
	}
#endif //GSD_FEATURE_ENABLED(DEBUG_SERIAL_PORT)
}

void nRF905_SetRTC(void)
{
	gsd_hb_packet_t *hb_pkt = (gsd_hb_packet_t *)&xbuf[1];
	uint8_t upd_flg = 0;

//      vPrintString("\t->RX PKT FUNC:");  vPrintString(psUInt8HexToString(hb_pkt->HB_FUNCTION, prt_buf)); vPrintEOL();	
//      vPrintString("\t->RX PKT MODE:");  vPrintString(psUInt8HexToString(hb_pkt->HB_MODE, prt_buf)); vPrintEOL();	
//      vPrintString("\t->RX PKT FLG:");  vPrintString(psUInt8HexToString(hb_pkt->HB_ALRM_FLG, prt_buf)); vPrintEOL();	

	if (hb_pkt->HB_ALRM_FLG == HBEAT_ACK) return;
	
	packet2calendar((uint8_t *)&hb_pkt->HB_RTC_LOW);
	printDateTime();
	rtc_load();

#if GSD_FEATURE_ENABLED(SLEEP_TIME_SETUP_FROM_AGGREGATOR)
#else
/*=============*/
hb_pkt->HB_PWRON_HR = gsd_setup.RAMn_ALRM1HR;
hb_pkt->HB_PWRON_MIN = gsd_setup.RAMn_ALRM1MIN;
hb_pkt->HB_PWROFF_HR = gsd_setup.RAMn_ALRM0HR;
hb_pkt->HB_PWROFF_MIN = gsd_setup.RAMn_ALRM0MIN;
//============= *
#endif // GSD_FEATURE_ENABLED(SLEEP_TIME_SETUP_FROM_AGGREGATOR)
	
#if GSD_FEATURE_ENABLED(SLEEP_MODE)
#if GSD_FEATURE_ENABLED(DEBUG_SERIAL_PORT)
		vPrintString("\tAlarm: ON=[0x");
		vPrintString(psUInt8HexToString(hb_pkt->HB_PWRON_HR, prt_buf)); vPrintString(", 0x");
		vPrintString(psUInt8HexToString(hb_pkt->HB_PWRON_MIN, prt_buf)); vPrintString("]"); vPrintEOL();
		vPrintString("\tAlarm: OFF=[0x");
		vPrintString(psUInt8HexToString(hb_pkt->HB_PWROFF_HR, prt_buf)); vPrintString(", 0x");
		vPrintString(psUInt8HexToString(hb_pkt->HB_PWROFF_MIN, prt_buf)); vPrintString("]"); vPrintEOL();
#endif //GSD_FEATURE_ENABLED(DEBUG_SERIAL_PORT)
// [ADK] 11/18/2019  Configure Alarm for PWR off ...
	rtc_set_alarm(hb_pkt->HB_PWROFF_HR, hb_pkt->HB_PWROFF_MIN);
	dt_power_on[0] =  hb_pkt->HB_PWRON_HR;
	dt_power_on[1]=  hb_pkt->HB_PWRON_MIN;
#else // GSD_FEATURE_ENABLED(SLEEP_MODE)
		vPrintString("\tAlarm: Disabled"); vPrintEOL();
#endif // GSD_FEATURE_ENABLED(SLEEP_MODE)

	if ( gsd_setup.RAMn_ALRM0HR != hb_pkt->HB_PWROFF_HR) { 	gsd_setup.RAMn_ALRM0HR = hb_pkt->HB_PWROFF_HR; upd_flg = 1;}
	if ( gsd_setup.RAMn_ALRM0MIN != hb_pkt->HB_PWROFF_MIN) { gsd_setup.RAMn_ALRM0MIN = hb_pkt->HB_PWROFF_MIN; upd_flg =1; }
	if ( gsd_setup.RAMn_ALRM1HR != hb_pkt->HB_PWRON_HR) { gsd_setup.RAMn_ALRM1HR = hb_pkt->HB_PWRON_HR; upd_flg = 1; }
	if ( gsd_setup.RAMn_ALRM1MIN != hb_pkt->HB_PWRON_MIN) { gsd_setup.RAMn_ALRM1MIN = hb_pkt->HB_PWRON_MIN; upd_flg = 1; }
	
	if (upd_flg)
		save_setup();

#if GSD_FEATURE_ENABLED(SLEEP_MODE)
	rtc_enable_alarm();
#endif //GSD_FEATURE_ENABLED(SLEEP_MODE)
}

void nRF905_put_setup(void)
{
	gsd_hb_packet_t *hb_pkt = (gsd_hb_packet_t *)&xbuf[1];
	if ((gsd_setup.RAMn_MODE&CODE_RUN_LIVE_TEST) == CODE_RUN_LIVE_TEST) 
		gsd_tx_packet_type = hb_pkt->HB_ALRM_FLG;

	if (gsd_tx_packet_type == HBEAT_wAUDIO_PWR_ON_3)
	{
		put_setup(hb_pkt);
	}
}

void nRF905_SetWkUpRTC(void)
{
#if GSD_FEATURE_ENABLED(SLEEP_MODE)
      if ((gsd_setup.RAMn_HBPM_INTERVAL & 0x20) == 0x20) {
// -- the UART disabled at this time		vPrintString("\tAlarm mode - no weekend"); vPrintEOL();
#if GSD_FEATURE_ENABLED(DEBUG_RTC_SETUP)
		rtc_set_alarm_monday(dt_power_on[0], dt_power_on[1]);
#else
	 	if (rtc_get_wday() == 0x05)
		{
// -- the UART disabled at this time			vPrintString("\t\tToday is Friday .."); vPrintEOL();
			rtc_set_alarm_monday(dt_power_on[0], dt_power_on[1]);
	  	}
		else	
			rtc_set_alarm(dt_power_on[0], dt_power_on[1]);
#endif //GSD_FEATURE_ENABLED(DEBUG_RTC_SETUP)
		
	  }
	  else	
		rtc_set_alarm(dt_power_on[0], dt_power_on[1]);
	rtc_enable_alarm();
#endif //GSD_FEATURE_ENABLED(SLEEP_MODE)
}
void nRF905_SetDtWakeUp(uint8_t hr, uint8_t min)
{
	dt_power_on[0] = hr; dt_power_on[1] = min;
}

static void nRF905_Configure(void)
{
	uint8_t *cfg = &(nRF905_cfg.cfg[1]);
//	uint8_t tmp;

	memset(cfg, 0x00, 10);
	
// Set Channel #
	if (nRF905_conf.ch_no) {
		cfg[ 0] = 	(uint8_t)(nRF905_conf.ch_no&0x00FF);
	}
// bits[7:6]  not used
//	[5]
	cfg[1] |= (uint8_t)(((nRF905_conf.auto_retran)&0x01) <<5);

// [4]
	cfg[1] |= (uint8_t)(((nRF905_conf.hfreq_pll)&0x01) <<4);
// [3:2]
	cfg[1] |= (uint8_t)(((nRF905_conf.pa_pwr)&0x03) <<2);
//[1]
	cfg[1] |= (uint8_t)(((nRF905_conf.rx_red_pwr)&0x01) <<1);
//[0]
	cfg[ 1] |=(uint8_t)(((nRF905_conf.ch_no&0x0100)>>8)&0x01);

// bit[7] not used
// [6:4]
	cfg[2] |= (uint8_t)(((nRF905_conf.tx_afw)&0x07) <<4 );
// bit [3] not used
// [2:0]
	cfg[2] |= (uint8_t)(((nRF905_conf.rx_afw)&0x07));

// bits[7:6] not used
// [5:0]
	cfg[3] =  (uint8_t)(((nRF905_conf.rx_pw)&0x3f));
// bits[7:6] not used
// [5:0]
	cfg[4] =  (uint8_t)(((nRF905_conf.tx_pw)&0x3f));

	cfg[ 5] = (nRF905_conf.rx_address_low &0xff);
	cfg[ 6] = ((nRF905_conf.rx_address_low>>8) &0xff);
	cfg[ 7] = (nRF905_conf.rx_address_high &0xff);
	cfg[ 8] = ((nRF905_conf.rx_address_high>>8) &0xff);

#if 1
/* [ADK]  11/27/2019  It a copy from the RF.ASM:
             .byte     11011000B           ;CRC_MODE=16,CRC_EN=ON,XOF[2:0]=16MHz
                                         ;uPCLKEN=OFF,UPCLKFREQ[2:0]
             it seems bits are reverted if compare with nRF905 DS                            
*/
	cfg[9] = 0xD8;
#else 
// [7:5]
// [4:2]
	cfg[9] |= (uint8_t)(((nRF905_conf.xof)&0x07) <<2 );
// [ 1]
	cfg[9] |= (uint8_t)(((nRF905_conf.crc_en)&0x01) <<1 );
// [0]
	cfg[9] |= (uint8_t)(((nRF905_conf.crc_mode)&0x01));
#endif

#if GSD_FEATURE_ENABLED(DUMP_NRF905_CFG)
	vPrintString("\r\n\tnRF905 Configuration register> [");
	vPrintString(psUInt8HexToString(cfg[0], prt_buf)); vPrintString(" ");
	vPrintString(psUInt8HexToString(cfg[1], prt_buf)); vPrintString(" ");
	vPrintString(psUInt8HexToString(cfg[2], prt_buf)); vPrintString(" ");
	vPrintString(psUInt8HexToString(cfg[3], prt_buf)); vPrintString(" ");
	vPrintString(psUInt8HexToString(cfg[4], prt_buf)); vPrintString(" ");
	vPrintString(psUInt8HexToString(cfg[5], prt_buf)); vPrintString(" ");
	vPrintString(psUInt8HexToString(cfg[6], prt_buf)); vPrintString(" ");
	vPrintString(psUInt8HexToString(cfg[7], prt_buf)); vPrintString(" ");
	vPrintString(psUInt8HexToString(cfg[8], prt_buf)); vPrintString(" ");
	vPrintString(psUInt8HexToString(cfg[9], prt_buf)); vPrintString("]"); vPrintEOL();
#endif //GSD_FEATURE_ENABLED(DUMP_NRF905_CFG)

	nRF905_WriteConfig(); // the xcfg[0] used for the command W_CONFIG ..

#if GSD_FEATURE_ENABLED(DUMP_NRF905_CFG)

	memset(nRF905_cfg.cfg, 0x00, 12);
	nRF905_ReadConfig();  // the xcfg[0] used for read the STATUS_REGISTER  ..
	nRF905_DumpConfig(&(nRF905_cfg.cfg[1]));
	
#endif // GSD_FEATURE_ENABLED(DUMP_NRF905_CFG)
}

uint8_t getSendWfFlag(void)
{ 
	return send_wf_flag;
}

void nRF905_sndRstHB(void)
{
	gsd_hb_packet_t  *hb_pkt;

	send_wf_flag = 0;
	
	if (!(gsd_tx_packet_type == 0 || gsd_tx_packet_type == 1 ||
		gsd_tx_packet_type == HBEAT_wAUDIO_PWR_OFF_3 ||
		gsd_tx_packet_type == HBEAT_wAUDIO_PWR_ON_3 
	       )	
	) return;

	nRF905_pwr_on();
	
	rf_dly = (gsd_setup.RAMn_SLOT_DLYNo* gsd_setup.RAMn_RF_SLOTNo *10);
	if (rf_dly < 300) rf_dly+=300;


	DelayMS(rf_dly);

	nRF905_conf.ch_no = 108;
	nRF905_conf.auto_retran = 0;
	nRF905_conf.hfreq_pll = 0;
	nRF905_conf.pa_pwr = 3;
	nRF905_conf.rx_red_pwr = 0;
	nRF905_conf.rx_afw = 4;
	nRF905_conf.tx_afw = 4;
	nRF905_conf.rx_pw = 32;
	nRF905_conf.tx_pw = 32;
	nRF905_conf.rx_address_low = 0xE7E7;
	nRF905_conf.rx_address_high = 0xE7E7;
	nRF905_conf.crc_mode = 1;
	nRF905_conf.crc_en = 1;
	nRF905_conf.xof = 3;  // 16MHz, not default 8MHz

	nRF905_conf.rx_address_high = (gsd_setup.RAMn_TAGID&0xFF);
	nRF905_conf.rx_address_high <<= 8;
	nRF905_conf.rx_address_high |= ((gsd_setup.RAMn_TAGID >> 8)&0xFF);
	nRF905_Configure();

	DelayMS(100);

	memset(xbuf, 0x00, 33);
	xbuf[0] = nRF905_W_TX_PAYLOAD_CMD;
	hb_pkt = (gsd_hb_packet_t  *)&xbuf[1];

	spi_rx_ptr = xbuf;

	nRF905_TxStart();

	hb_pkt->HB_TAGID = gsd_setup.RAMn_TAGID;
	hb_pkt->HB_MODE = gsd_setup.RAMn_MODE;
	hb_pkt->HB_ALRM_FLG = gsd_tx_packet_type;
	
//  vPrintString("\t->PKT:"); vPrintString(psUInt8HexToString(gsd_tx_packet_type, prt_buf)); vPrintEOL();
      vPrintString("\t->ID:");  vPrintString(psUInt16HexToString(hb_pkt->HB_TAGID, prt_buf)); vPrintEOL();	
      vPrintString("\t->PKT MODE:");  vPrintString(psUInt8HexToString(gsd_setup.RAMn_MODE, prt_buf)); vPrintEOL();	

	if ((gsd_tx_packet_type == 0 || gsd_tx_packet_type == 1))
	{
		if ((gsd_setup.RAMn_MODE&CODE_RUN_SHORT_DATA_OUT) == CODE_RUN_SHORT_DATA_OUT)
		{
			send_wf_flag = 0x1; 
			// We'll send 3 packets with 24bytes from the FRAM every  
		}else{
			send_wf_flag = 0x3; 
			// will skip sending other packets, and start audio ...
		}
	}

	if (gsd_tx_packet_type == HBEAT_wAUDIO_PWR_ON_3 || gsd_tx_packet_type == HBEAT_wAUDIO_PWR_OFF_3)
	{
		if (gsd_tx_packet_type == HBEAT_wAUDIO_PWR_ON_3)
			xmitsNo = 1;
	
		hb_pkt->HB_ROOM_DSCR	= gsd_setup.RAMn_FLOORNo;
		hb_pkt->HB_ROOM_No		= gsd_setup.RAMn_ROOMNo;
		hb_pkt->HB_RF_SLOT_No	= gsd_setup.RAMn_RF_SLOTNo;
		hb_pkt->HB_SYS_FLG		= gsd_setup.RAMn_STARTUP_FLG;
		hb_pkt->HB_PWROFF_HR	= gsd_setup.RAMn_ALRM0HR;
		hb_pkt->HB_PWRON_HR	= gsd_setup.RAMn_ALRM1HR;
		hb_pkt->HB_PM_INTRV		= gsd_setup.RAMn_HBPM_INTERVAL;
		hb_pkt->HB_AUDIO_THRSH	= gsd_setup.RAMn_DA01;
		hb_pkt->HB_RF_DLY		= gsd_setup.RAMn_SLOT_DLYNo;
		hb_pkt->HB_PWROFF_MIN	= gsd_setup.RAMn_ALRM0MIN;
		hb_pkt->HB_PWRON_MIN	= gsd_setup.RAMn_ALRM1MIN;
		hb_pkt->HB_ALRM_THRSH_LOW	= gsd_setup.RAMn_VAR_LMT0;
		hb_pkt->HB_ALRM_THRSH_HIGH	= gsd_setup.RAMn_VAR_LMT2;
		}

	if (gsd_tx_packet_type == HBEAT_wAUDIO_PWR_ON_3)
	{
		hb_pkt->HB_ECHO = (uint16_t)(GSD_FW_VERSION&0xFFFF);
		hb_pkt->HB_XMTS_No = (uint16_t)((GSD_FW_VERSION>>16)&0xFFFF);
	}
	else	
		hb_pkt->HB_XMTS_No	= xmitsNo++;


	if (send_wf_flag == 0x01) {
		load_wf(&hb_pkt->HB_ROOM_DSCR, RAM_EXTENDED); // Load WF's 1st 24 bytes
	}
	
 	transactionDone();
	WriteBytes(33);  // Load 32 bytes to the TX_PAYLOAD
	transactionDone();

    	GPIO_selectInterruptEdge( GPIO_PORT_P3, GPIO_PIN6, GPIO_LOW_TO_HIGH_TRANSITION);
    	GPIO_clearInterrupt( GPIO_PORT_P3, GPIO_PIN6);

   	nRF905_mode = nRF905_MODE_TX;
	
	GPIO_enableInterrupt( GPIO_PORT_P3, GPIO_PIN6);
	GPIO_setOutputHighOnPin(    	GPIO_PORT_P5, GPIO_PIN1);  //  RF_TX_CE -> HIGH
	_delay_cycles(80);
	GPIO_setOutputLowOnPin(    	GPIO_PORT_P5, GPIO_PIN1);  //  RF_TX_CE -> LOW
	


}

void nRF905_send_2nd(void)
{
	gsd_hb_packet_t  *hb_pkt;

vPrintString("\t==> Send 2nd .."); vPrintEOL();
	nRF905_pwr_on();

	DelayMS(rf_dly);

	nRF905_conf.ch_no = 108;
	nRF905_conf.auto_retran = 0;
	nRF905_conf.hfreq_pll = 0;
	nRF905_conf.pa_pwr = 3;
	nRF905_conf.rx_red_pwr = 0;
	nRF905_conf.rx_afw = 4;
	nRF905_conf.tx_afw = 4;
	nRF905_conf.rx_pw = 32;
	nRF905_conf.tx_pw = 32;
	nRF905_conf.rx_address_low = 0xE7E7;
	nRF905_conf.rx_address_high = 0xE7E7;
	nRF905_conf.crc_mode = 1;
	nRF905_conf.crc_en = 1;
	nRF905_conf.xof = 3;  // 16MHz, not default 8MHz

	nRF905_conf.rx_address_high = (gsd_setup.RAMn_TAGID&0xFF);
	nRF905_conf.rx_address_high <<= 8;
	nRF905_conf.rx_address_high |= ((gsd_setup.RAMn_TAGID >> 8)&0xFF);
	nRF905_Configure();

	DelayMS(100);

	memset(xbuf, 0x00, 33);


	xbuf[0] = nRF905_W_TX_PAYLOAD_CMD;
	hb_pkt = (gsd_hb_packet_t  *)&xbuf[1];

	spi_rx_ptr = xbuf;

	nRF905_TxStart();

	hb_pkt->HB_TAGID = gsd_setup.RAMn_TAGID;
	hb_pkt->HB_MODE = gsd_setup.RAMn_MODE;
	hb_pkt->HB_ALRM_FLG = gsd_tx_packet_type;
	hb_pkt->HB_ECHO = 1;

	load_wf(&hb_pkt->HB_ROOM_DSCR, RAM_EXTENDED +21); // Load WF's 2nd 21 bytes
	send_wf_flag = 0x02;
	
 	transactionDone();
	WriteBytes(33);  // Load 32 bytes to the TX_PAYLOAD
	transactionDone();

    	GPIO_selectInterruptEdge( GPIO_PORT_P3, GPIO_PIN6, GPIO_LOW_TO_HIGH_TRANSITION);
    	GPIO_clearInterrupt( GPIO_PORT_P3, GPIO_PIN6);

   	nRF905_mode = nRF905_MODE_TX;
	
	GPIO_enableInterrupt( GPIO_PORT_P3, GPIO_PIN6);
	GPIO_setOutputHighOnPin(    	GPIO_PORT_P5, GPIO_PIN1);  //  RF_TX_CE -> HIGH
	_delay_cycles(80);
	GPIO_setOutputLowOnPin(    	GPIO_PORT_P5, GPIO_PIN1);  //  RF_TX_CE -> LOW

	

}

void nRF905_send_3rd(void)
{
	gsd_hb_packet_t  *hb_pkt;

vPrintString("\t==> Send 3rd .."); vPrintEOL();
	nRF905_pwr_on();

	DelayMS(rf_dly);

	nRF905_conf.ch_no = 108;
	nRF905_conf.auto_retran = 0;
	nRF905_conf.hfreq_pll = 0;
	nRF905_conf.pa_pwr = 3;
	nRF905_conf.rx_red_pwr = 0;
	nRF905_conf.rx_afw = 4;
	nRF905_conf.tx_afw = 4;
	nRF905_conf.rx_pw = 32;
	nRF905_conf.tx_pw = 32;
	nRF905_conf.rx_address_low = 0xE7E7;
	nRF905_conf.rx_address_high = 0xE7E7;
	nRF905_conf.crc_mode = 1;
	nRF905_conf.crc_en = 1;
	nRF905_conf.xof = 3;  // 16MHz, not default 8MHz

	nRF905_conf.rx_address_high = (gsd_setup.RAMn_TAGID&0xFF);
	nRF905_conf.rx_address_high <<= 8;
	nRF905_conf.rx_address_high |= ((gsd_setup.RAMn_TAGID >> 8)&0xFF);
	nRF905_Configure();

	DelayMS(100);

	memset(xbuf, 0x00, 33);


	xbuf[0] = nRF905_W_TX_PAYLOAD_CMD;
	hb_pkt = (gsd_hb_packet_t  *)&xbuf[1];

	spi_rx_ptr = xbuf;

	nRF905_TxStart();

	hb_pkt->HB_TAGID = gsd_setup.RAMn_TAGID;
	hb_pkt->HB_MODE = gsd_setup.RAMn_MODE;
	hb_pkt->HB_ALRM_FLG = gsd_tx_packet_type;
	hb_pkt->HB_ECHO = 2;

	load_wf(&hb_pkt->HB_ROOM_DSCR, RAM_EXTENDED +42); // Load WF's 3rd 21 bytes
	send_wf_flag = 0x03;
	
 	transactionDone();
	WriteBytes(33);  // Load 32 bytes to the TX_PAYLOAD
	transactionDone();

    	GPIO_selectInterruptEdge( GPIO_PORT_P3, GPIO_PIN6, GPIO_LOW_TO_HIGH_TRANSITION);
    	GPIO_clearInterrupt( GPIO_PORT_P3, GPIO_PIN6);

   	nRF905_mode = nRF905_MODE_TX;
	
	GPIO_enableInterrupt( GPIO_PORT_P3, GPIO_PIN6);
	GPIO_setOutputHighOnPin(    	GPIO_PORT_P5, GPIO_PIN1);  //  RF_TX_CE -> HIGH
	_delay_cycles(80);
	GPIO_setOutputLowOnPin(    	GPIO_PORT_P5, GPIO_PIN1);  //  RF_TX_CE -> LOW

	

}


#if GSD_FEATURE_ENABLED(DEBUGGING_MENU)


int menuRfPwr(void *menu, void *item, void *args)
{
	int c;
	vPrintString("Enter '1' for power ON, any other - power OFF .."); vPrintEOL();

	while(1) 
	{
		delay_ms(2);

		if ((c = uart_getchar()) == '1') 
		{
			vPrintString("\tnRF905 power ON"); vPrintEOL();
			nRF905_pwr_on();
			break;
		}else
		if ( c != 0)  {
			vPrintString("\tnRF905 power OFF"); vPrintEOL();
			nRF905_pwr_off();
			break;
		}
	}
	return 0;
}


static void nRF905_DumpConfig(uint8_t *show_cfg)
{

	char *sxof, *sout;

	vPrintString("\r\n\tnRF905 Configuration register: [");
	vPrintString(psUInt8HexToString(show_cfg[0], prt_buf)); vPrintString(" ");
	vPrintString(psUInt8HexToString(show_cfg[1], prt_buf)); vPrintString(" ");
	vPrintString(psUInt8HexToString(show_cfg[2], prt_buf)); vPrintString(" ");
	vPrintString(psUInt8HexToString(show_cfg[3], prt_buf)); vPrintString(" ");
	vPrintString(psUInt8HexToString(show_cfg[4], prt_buf)); vPrintString(" ");
	vPrintString(psUInt8HexToString(show_cfg[5], prt_buf)); vPrintString(" ");
	vPrintString(psUInt8HexToString(show_cfg[6], prt_buf)); vPrintString(" ");
	vPrintString(psUInt8HexToString(show_cfg[7], prt_buf)); vPrintString(" ");
	vPrintString(psUInt8HexToString(show_cfg[8], prt_buf)); vPrintString(" ");
	vPrintString(psUInt8HexToString(show_cfg[9], prt_buf)); vPrintString("]"); vPrintEOL();

	nRF905_conf.ch_no = (show_cfg[ 1]&0x1); nRF905_conf.ch_no <<=8;
	nRF905_conf.ch_no |= (show_cfg[ 0]); 

	nRF905_conf.auto_retran = (show_cfg[ 1]&0x20); nRF905_conf.auto_retran>>=6;
	nRF905_conf.rx_red_pwr  = (show_cfg[ 1]&0x10); nRF905_conf.rx_red_pwr>>=5;  
	nRF905_conf.pa_pwr = (show_cfg[ 1]&0x0C);  nRF905_conf.pa_pwr >>= 2;   
	nRF905_conf.hfreq_pll = (show_cfg[ 1]&0x02); nRF905_conf.hfreq_pll >>=1;

	nRF905_conf.tx_afw  = (show_cfg[ 2]&0x70); nRF905_conf.tx_afw >>=4;
	nRF905_conf.rx_afw = (show_cfg[ 2]&0x07);


	nRF905_conf.rx_pw = (show_cfg[ 3]&0x3f); 
	nRF905_conf.tx_pw = (show_cfg[ 4]&0x3f); 
	
	nRF905_conf.rx_address_high = (show_cfg[8] << 8);
	nRF905_conf.rx_address_high |= (show_cfg[7]);
	nRF905_conf.rx_address_low = (show_cfg[6] <<8);
	nRF905_conf.rx_address_low |= (show_cfg[5]);

	
	nRF905_conf.xof = (((show_cfg[ 9] &0x1C) >>2)&0x7);
	nRF905_conf.crc_en =((show_cfg[ 9] &0x02) >>1 ); 
	nRF905_conf.crc_mode =((show_cfg[ 9] &0x01)); 
	

	vPrintString("\tCH_NO=");
	vPrintString(psUInt16ToString(nRF905_conf.ch_no, prt_buf)); vPrintEOL();
	if (nRF905_conf.hfreq_pll == 0) 
		vPrintString("\tmode 433MHz");
	else
		vPrintString("\tmode 868/915MHz");
	vPrintEOL();

	switch (nRF905_conf.pa_pwr){
		default: sout=(char *)"?"; break;
		case 0:  sout=(char *)"-10dBm"; break; 
		case 1:  sout=(char *)"-2dBm"; break; 
		case 2:  sout=(char *)"+6dBm"; break; 
		case 3:  sout=(char *)"+10dBm"; break; 
	}
	vPrintString("\tPA_PWR=");
	vPrintString(sout);
	vPrintEOL();

	vPrintString("\tAUTO_RETRAN=");
	vPrintString((nRF905_conf.auto_retran)? "enabled":"disabled");
	vPrintEOL();

	switch (nRF905_conf.rx_afw){
		default: sout=(char *)"?"; break;
		case 1:  sout=(char *)"1 byte"; break; 
		case 4:  sout=(char *)"4 bytes"; break; 
	}
	vPrintString("\tRX_AFW=");
	vPrintString(sout);
	vPrintEOL();

	switch (nRF905_conf.tx_afw){
		default: sout=(char *)"?"; break;
		case 1:  sout=(char *)"1 byte"; break; 
		case 4:  sout=(char *)"4 bytes"; break; 
	}
	vPrintString("\tTX_AFW=");
	vPrintString(sout);
	vPrintEOL();

	vPrintString("\tRX_PW=");
	vPrintString(psUInt16ToString(nRF905_conf.rx_pw, prt_buf));
	vPrintString(", TX_PW=");
	vPrintString(psUInt16ToString(nRF905_conf.tx_pw, prt_buf));
	vPrintEOL();

	vPrintString("\tRX_ADDRESS=0x");
	vPrintString(psUInt16HexToString(nRF905_conf.rx_address_low, prt_buf));
	vPrintString(psUInt16HexToString(nRF905_conf.rx_address_high, prt_buf));
	vPrintEOL();
	
	switch (nRF905_conf.xof) 
	{
		default:	sxof=(char *)"?"; break;
		case 0:	sxof= (char *)"4MHz"; break; 
		case 1:	sxof= (char *)"8MHz"; break; 
		case 2:	sxof= (char *)"12MHz"; break; 
		case 3:	sxof= (char *)"16MHz"; break; 
		case 4:	sxof= (char *)"20MHz"; break; 
	}

	vPrintString("\tXOF=");
	vPrintString(sxof); vPrintEOL();

	vPrintString("\tCRC_EN=");
	vPrintString((nRF905_conf.crc_en)? "Enable":"Disable");
	vPrintString(", CRC_MODE=");
	vPrintString((nRF905_conf.crc_mode)?"16bits":"8bits");
	vPrintEOL();

}



int menuRfWriteDfltCfg(void *menu, void *item, void *args)
{
	nRF905_conf.ch_no = 108;
	nRF905_conf.auto_retran = 0;
	nRF905_conf.hfreq_pll = 0;
	nRF905_conf.pa_pwr = 3;
	nRF905_conf.rx_red_pwr = 0;
	nRF905_conf.rx_afw = 4;
	nRF905_conf.tx_afw = 4;
	nRF905_conf.rx_pw = 32;
	nRF905_conf.tx_pw = 32;
	nRF905_conf.rx_address_low = 0xE7E7;
	nRF905_conf.rx_address_high = 0xE7E7;
	nRF905_conf.crc_mode = 1;
	nRF905_conf.crc_en = 1;
	nRF905_conf.xof = 3;  // 16MHz, not default 8MHz

	nRF905_conf.rx_address_high = (gsd_setup.RAMn_TAGID&0xFF);
	nRF905_conf.rx_address_high <<= 8;
	nRF905_conf.rx_address_high |= ((gsd_setup.RAMn_TAGID >> 8)&0xFF);
	nRF905_Configure();
	return 0;
	
}
	
int menuRfDumpCfg(void *menu, void *item, void *args)
{
	nRF905_ReadConfig();
	nRF905_DumpConfig(&(nRF905_cfg.cfg[1]));
	return 0;
}

int menuRfSendPacket(void *menu, void *item, void *args)
{
	gsd_hb_packet_t  *hb_pkt;

// sprintf(xbuf, "%d", sizeof(gsd_hb_packet_t));
// printf(": gsd_hb_packet_t size=%s\r\n", xbuf);

	memset(xbuf, 0x00, 33);
	xbuf[0] = nRF905_W_TX_PAYLOAD_CMD;
	hb_pkt = (gsd_hb_packet_t  *)&xbuf[1];

	spi_rx_ptr = xbuf;

	nRF905_TxStart();
	
	hb_pkt->HB_TAGID = gsd_setup.RAMn_TAGID;
	hb_pkt->HB_ALRM_FLG = HBEAT_wAUDIO_PWR_ON_3;

	
 	transactionDone();
	WriteBytes(33);  // Load 32 bytes to the TX_PAYLOAD
	transactionDone();

    	GPIO_selectInterruptEdge( GPIO_PORT_P3, GPIO_PIN6, GPIO_LOW_TO_HIGH_TRANSITION);
    	GPIO_clearInterrupt( GPIO_PORT_P3, GPIO_PIN6);

   	nRF905_mode = nRF905_MODE_TX;

	GPIO_enableInterrupt( GPIO_PORT_P3, GPIO_PIN6);
	GPIO_setOutputHighOnPin(    	GPIO_PORT_P5, GPIO_PIN1);  //  RF_TX_CE -> HIGH
	_delay_cycles(80);
	GPIO_setOutputLowOnPin(    	GPIO_PORT_P5, GPIO_PIN1);  //  RF_TX_CE -> LOW

	return 1;

}

int menuRfRecvPacket (void *menu, void *item, void *args)
{
	uint8_t status, prev = 0, idx;
	
//	nRF905_RxStart();

    	GPIO_clearInterrupt( GPIO_PORT_P3, GPIO_PIN6);
	GPIO_disableInterrupt( GPIO_PORT_P3, GPIO_PIN6);
	GPIO_setOutputHighOnPin(    	GPIO_PORT_P5, GPIO_PIN2);  //  RF_PWRUP  -> HIGH
	GPIO_setOutputLowOnPin(    	GPIO_PORT_P5, GPIO_PIN0);  // RF_TX_EN -> LOW
	GPIO_setOutputHighOnPin(    	GPIO_PORT_P5, GPIO_PIN1);  //  RF_TX_CE -> HIGH

	do {
		 {
			nRF905_RxDone();
			vPrintString("Got DR interrupt .."); vPrintEOL();
			prev = 0;
		}
		transactionDone();   // toggle CS
		status = readByte();  // read status byte..
		if (status) {
			if (status != prev) {	
//				sprintf(prt, "%02x", status);
//				printf("Got status: 0x%s\r\n", prt);
				prev = status;
			}
			if ((status & 0xA0) == 0xA0) {
				vPrintString("=== Got status with DR: ");
				vPrintString(psUInt8HexToString(status, prt_buf));
				vPrintEOL();
				memset(xbuf, 0xa5, 33);
				nRF905_ReadData();
				for (idx =0; idx < 6; idx++) {

					vPrintString("\t");
					vPrintString(psUInt8HexToString(xbuf[(idx*5) +1], prt_buf));
					vPrintString(psUInt8HexToString(xbuf[(idx*5) +2], prt_buf));
					vPrintString(psUInt8HexToString(xbuf[(idx*5) +3], prt_buf));
					vPrintString(psUInt8HexToString(xbuf[(idx*5) +4], prt_buf));
					vPrintString(psUInt8HexToString(xbuf[(idx*5) +5], prt_buf));
					vPrintEOL();
				}
				nRF905_RxDone();
				
//				break;
			}
		}
		delay_ms(1);
	}while(1);

	return 1;
}

#endif //GSD_FEATURE_ENABLED(DEBUGGING_MENU)

#if 0
// [ADK] 11/15/2019   -- use bitbang
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCI_B1_VECTOR
__interrupt
#elif defined(__GNUC__)
__attribute__((interrupt(USCI_B1_VECTOR)))
#endif
void USCI_B1_ISR (void)
{
    switch (__even_in_range(UCB1IV, USCI_SPI_UCTXIFG))
    {
        case USCI_SPI_UCRXIFG:      // UCRXIFG
             spi_rx_ptr[ spi_rx_idx++]= EUSCI_B_SPI_receiveData(EUSCI_B1_BASE);
            break;
        default:
            break;
    }
}
#endif
//******************************************************************************
//
//This is the PORT3_VECTOR interrupt vector service routine
//
//******************************************************************************
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=PORT3_VECTOR
__interrupt
#elif defined(__GNUC__)
__attribute__((interrupt(PORT3_VECTOR)))
#endif
void Port_3 (void)
{

    LPM3_EXIT;
    //P3.6 (DR) IFG cleared
    	__no_operation();                         // For debugger
    GPIO_clearInterrupt( GPIO_PORT_P3, GPIO_PIN6);

    if ( nRF905_mode == nRF905_MODE_TX)	
    		gsd_tx_done_event = GSD_HAVE_EVENT;	
    else	
    if ( nRF905_mode == nRF905_MODE_RX)	
    		gsd_rx_packet_event = GSD_HAVE_EVENT;	
    		
}


