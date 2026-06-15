/*
 * CH582F Sensor Beacon using coded phy S8 implemented by ch32fun and make command for cmd console in windows to compile the code.
 * Hardware wiring implented as below:

                 PA4 (AIN0)
                   |
      +------------+------------+
      |            |            |
     100k        100k         100k
      |            |            |
   BAT NODE     NTC NODE    LIGHT NODE
   
   Battery:



VBAT
 |
1M
 |
 +---- BAT NODE
 |
1M
 |
PB0
 |
GND



Temperature:

VDD
 |
10k
 |
 +---- NTC NODE
 |
10k NTC
 |
PB1
 |
GND



Light:

VDD
 |
1M
 |
 +---- LIGHT NODE
 |
Photodiode
 |
PB2
 |
GND

Measurement sequence:

PB0=LOW, PB1=INPUT, PB2=INPUT
Read battery
PB0=INPUT

Then:

PB1=LOW
Read NTC
PB1=INPUT

Then:

PB2=LOW
Read photodiode
PB2=INPUT

Only one divider exists at a time.

Sleep current remains essentially the leakage current of the GPIOs.
 *
 * PA4  = ADC input (AIN0)
 * PB0  = Battery divider switch
 * PB1  = NTC divider switch
 * PB2  = Photodiode divider switch
 *
 * Encodes sensor data into MAC address:
 *
 * Byte0 = Node ID
 * Byte1 = Battery
 * Byte2 = Temperature
 * Byte3 = Light
 * Byte4 = Sequence
 * Byte5 = CRC
 */

#include "ch32fun.h"
#include "iSLER.h"
#include "ch5xx_lowpower.h"
#include "fun_adc_ch5xx.h"

#include <stdio.h>
#include <string.h>

#define LED             PA8

#define BAT_SW          PB0
#define NTC_SW          PB1
#define LIGHT_SW        PB2

#define NODE_ID         0x01

#define PHY_MODE        PHY_S8         // PHY_1M  , PHY_2M ,PHY_S2 or PHY_S8 
#define ACCESS_ADDRESS  0x8E89BED6

#define SLEEPTIME_MS    300

u16 adc_raw = 0, adc_temp = 0, adc_bat = 0;


uint8_t sequence = 0;

ISLER_BUF_ATTR uint8_t adv[256];

/*
 * Advertisement
 *
 * MAC bytes are:
 * adv[2]..adv[7]
 */
const uint8_t adv_data[] =
{
    0x02,0x0d,                         // header for LL: PDU + frame length        

    0xAA,0xBB,0xCC,0xDD,0xEE,0xFF,     // MAC (reversed)          

    0x06,0x09,'S','E','N','S'          // 0x09: "Complete Local Name". Can it be used for sending more sensor data replacing local name?
};

uint8_t adv_channels[] = {37,38,39};





__INTERRUPT
void RTC_IRQHandler(void)
{
    R8_RTC_FLAG_CTRL = RB_RTC_TRIG_CLR;
}

void allPinPullUp(void)
{
    R32_PA_DIR = 0;
    R32_PA_PD_DRV = 0;
    R32_PA_PU = P_All;

#if PB
    R32_PB_DIR = 0;
    R32_PB_PD_DRV = 0;
    R32_PB_PU = P_All;
#endif
}

void blink(int n)
{
    while(n--)
    {
        funDigitalWrite(LED,FUN_LOW);
        Delay_Ms(30);

        funDigitalWrite(LED,FUN_HIGH);
        Delay_Ms(30);
    }
}


void incoming_frame_handler() {
	// The chip stores the incoming frame in LLE_BUF, defined in extralibs/iSLER.h
	uint8_t *frame = (uint8_t*)LLE_BUF;
	int rssi = iSLERRSSI();

	// The first two bytes of the frame are metadata with PDU and length
	printf("RSSI:%d PDU:%d len:%d MAC:", rssi, frame[0], frame[1]);
	
	for(int i = 7; i > 2; i--) {
		printf("%02x:", frame[i]);
	}
	printf("%02x data:", frame[2]);
	for(int i = 8; i < frame[1] +2; i++) {
		printf("%02x ", frame[i]);
	}
	printf("\n");

	
}


void adcInit(void)
{
    adc_set_channel(0); // PA4
    adc_buf_enable(1);

    adc_set_config(
        ADC_FREQ_DIV_10,
        ADC_PGA_GAIN_1_4,
        0
    );
}

/********************************************************/
/* BATTERY */
/********************************************************/

uint16_t readBattery_mV(void)
{
    funPinMode(BAT_SW,GPIO_ModeOut_PP_5mA);
    funDigitalWrite(BAT_SW,FUN_LOW);

    Delay_Ms(3);

    adc_set_channel(0);

    uint16_t raw = adc_get_singleReading();

    funPinMode(BAT_SW,GPIO_ModeIN_Floating);

    int32_t mv = adc_to_mV(raw,ADC_PGA_GAIN_1_4);

    if(mv < 0) mv = 0;

    mv *= 2;      // 1M/1M divider

    return (uint16_t)mv;
}

/********************************************************/
/* NTC */
/********************************************************/

uint16_t readNTC(void)
{
    funPinMode(NTC_SW,GPIO_ModeOut_PP_5mA);
    funDigitalWrite(NTC_SW,FUN_LOW);

    Delay_Ms(5);

    adc_set_channel(0);

    uint16_t raw = adc_get_singleReading();

    funPinMode(NTC_SW,GPIO_ModeIN_Floating);

    return raw;
}

/********************************************************/
/* LIGHT */
/********************************************************/

uint16_t readLight(void)
{
    funPinMode(LIGHT_SW,GPIO_ModeOut_PP_5mA);
    funDigitalWrite(LIGHT_SW,FUN_LOW);

    Delay_Ms(5);

    adc_set_channel(0);

    uint16_t raw = adc_get_singleReading();

    funPinMode(LIGHT_SW,GPIO_ModeIN_Floating);

    return raw;
}

uint8_t crc8(uint8_t *d, int len)
{
    uint8_t c = 0;

    for(int i=0;i<len;i++)
        c ^= d[i];

    return c;
}

void buildAdvertisement(void)
{
    uint16_t batt_mv = readBattery_mV();
    uint16_t ntc_raw = readNTC();
    uint16_t light_raw = readLight();

    uint8_t batt8  = batt_mv / 20;
    uint8_t ntc8   = ntc_raw >> 4;
    uint8_t light8 = light_raw >> 4;

    adv[2] = NODE_ID;
    adv[3] = batt8;
    adv[4] = ntc8;
    adv[5] = light8;
    adv[6] = adc_temp; //sequence++;

    adv[7] = adc_bat;  //crc8(&adv[2],5);

    printf(
        "BAT=%umV (%u)  NTC=%u  LIGHT=%u\r\n",
        batt_mv,
        batt8,
        ntc_raw,
        light_raw
    );
}

void sendAdvertisement(void)
{
    for(int c=0;c<sizeof(adv_channels);c++)
    {
        iSLERTX(
            ACCESS_ADDRESS,
            adv,
            sizeof(adv_data),
            adv_channels[c],
            PHY_MODE
        );
    }
}



	
	
int main(void)
{
    SystemInit();

    DCDCEnable();
    LSIEnable();
    RTC_init();
    SleepInit();

    allPinPullUp();

    funGpioInitAll();

    funPinMode(LED,GPIO_CFGLR_OUT_2Mhz_PP);

    funPinMode(BAT_SW,GPIO_ModeIN_Floating);
    funPinMode(NTC_SW,GPIO_ModeIN_Floating);
    funPinMode(LIGHT_SW,GPIO_ModeIN_Floating);

    adcInit();

    uint8_t txPower = LL_TX_POWER_0_DBM;       //  Options = 0,1,2,3,4,5,6 or 7 or LL_TX_POWER_MINUS_25_DBM with options = 20,15,10,8,5,3 or 1.

    iSLERInit(txPower);

    memcpy(adv,adv_data,sizeof(adv_data));

    blink(3);

    printf("CH582 Sensor Beacon\r\n");

    while(1)
    {
        
		//# get temp reading
		adc_set_channel(ADC_TEMPERATURE_CHANNEL);
		adc_set_config(ADC_FREQ_DIV_10, ADC_PGA_GAIN_2, 1);
		adc_temp = adc_get_singleReading();
		printf("\nBuilt In Temperature Sensor: %li mC\r\n", adc_to_mCelsius(adc_temp));

		//# get battery reading
		adc_set_channel(ADC_VBAT_CHANNEL);
		adc_set_config(ADC_FREQ_DIV_10, ADC_PGA_GAIN_1_2, 0);
		adc_bat = adc_get_singleReading();
		printf("Built In Battery Voltage Sensor: %li mV\r\n", adc_to_mV(adc_bat, ADC_PGA_GAIN_1_2));

		//# get ch0 reading PA4
		adc_set_channel(0);
		adc_buf_enable(1);
		adc_set_config(ADC_FREQ_DIV_10, ADC_PGA_GAIN_1_4, 0);
		
		//# get single channel reading
		adc_raw = adc_get_singleReading();
		printf("mV reading from pin PA4: %li mV\r\n", adc_to_mV(adc_raw, ADC_PGA_GAIN_1_4));

		
		
		buildAdvertisement();

        sendAdvertisement();
		
		// now listen for frames on channel 37 on bed6. When the RF subsystem
		// detects and finalizes one, "rx_ready" in iSLER.h is set true
		iSLERRX(ACCESS_ADDRESS, 37, PHY_MODE);
		while(!rx_ready);

		// we stepped over !rx_ready so we got a frame
		blink(5);
		incoming_frame_handler();		

        blink(1);

        LowPower(
            MS_TO_RTC(SLEEPTIME_MS),
            (RB_PWR_RAM2K |
             RB_PWR_RAMX |
             RB_PWR_EXTEND)
        );

        iSLERInit(txPower);
        DCDCEnable();
    }
}


