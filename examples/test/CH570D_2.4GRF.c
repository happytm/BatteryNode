    /* CH570D hardware wiring for 2.4 ghz RF and analog sensors using Mounriver Studio 2 with https://github.com/Witawat/CH570D-SimpleHAL
  
                  PA3
                   |
      +------------+------------+
      |            |            |
     100k        100k         100k
      |            |            |
   BAT NODE     NTC NODE    LIGHT NODE
 
 
 
 VBAT
   |
  1M
   |
   +------ PA3
   |
  1M
   |
  PB0 (switch)
   |
  GND


   VDD
  |
 10k
  |
  +------ PA3
  |
 10k NTC
  |
 PB1 (switch)
  |
 GND


  VDD
  |
 1M
  |
  +------ PA3
  |
 Photodiode
  |
 PB2 (switch)
  |
 GND


Power behavior (important for coin cell)
Sensor	Active current	Sleep leakage
Battery divider	~1每3 ?A	~0 ?A
NTC	~50每200 ?A (only during sample)	0 ?A
Photodiode	~1每5 ?A	0 ?A

Average system consumption stays extremely low because:

sensors are powered only ~3每10 ms per wake cycle

*/

/******************************************************************************
 *
 * CH570D Ultra Low Power Wireless Sensor
 *
 * Measures:
 *   - Battery voltage
 *   - NTC temperature
 *   - Light level
 *
 * Sends packet via built-in 2.4GHz radio.
 *
 * Wakes every 60 seconds using RTC.
 *
 * After transmission:
 *   -> enters Shutdown mode
 *   -> RTC wakes chip
 *   -> chip resets
 *   -> main() runs again
 *
 ******************************************************************************/

#include "simple_hal.h"
#include "CH57x_common.h"

/*---------------------------------------------------------------------------
    USER CONFIGURATION
---------------------------------------------------------------------------*/

#define NODE_ID        1      /* 1=Kitchen, 2=Bedroom, etc */

/* ADC pins */

#define PIN_BATTERY    PA2
#define PIN_NTC        PA3
#define PIN_LIGHT      PA4

/* RF */

#define RF_FREQ_KHZ    2402000

/* RTC */

#define WAKE_SECONDS   60UL

/*---------------------------------------------------------------------------
    SENSOR PACKET
---------------------------------------------------------------------------*/

typedef struct __attribute__((packed))
{
    uint8_t  node_id;

    uint16_t battery_raw;
    uint16_t ntc_raw;
    uint16_t light_raw;

} sensor_packet_t;

/*---------------------------------------------------------------------------
    RTC CALLBACK
---------------------------------------------------------------------------*/

static void rtc_callback(void *arg)
{
    (void)arg;
}

/*---------------------------------------------------------------------------
    MAIN
---------------------------------------------------------------------------*/

int main(void)
{
    /***********************************************************************
        CLOCK SETUP
    ***********************************************************************/

    HSECFG_Capacitance(HSECap_18p);

    SetSysClock(CLK_SOURCE_HSE_PLL_24MHz);

    /***********************************************************************
        ADC SETUP

        Replace these with your actual ADC initialization calls if needed.
    ***********************************************************************/

    const uint8_t adc_channels[] =
    {
        PIN_BATTERY,
        PIN_NTC,
        PIN_LIGHT
    };

    ADC_SimpleInitChannels(adc_channels, 3);

    /***********************************************************************
        RF SETUP
    ***********************************************************************/

    hal_rf_handle_t rf;

    rf = hal_rf_init(
            RF_FREQ_KHZ,
            HAL_RF_PHY_2G4,
            0);                /* 0 dBm */

    /***********************************************************************
        READ SENSORS
    ***********************************************************************/

    sensor_packet_t pkt;

    pkt.node_id = NODE_ID;

    pkt.battery_raw = ADC_Read(PIN_BATTERY);

    pkt.ntc_raw = ADC_Read(PIN_NTC);

    pkt.light_raw = ADC_Read(PIN_LIGHT);

    /***********************************************************************
        TRANSMIT PACKET
    ***********************************************************************/

    hal_rf_send(
        rf,
        (uint8_t *)&pkt,
        sizeof(pkt));

    /***********************************************************************
        ALLOW TX TO COMPLETE
    ***********************************************************************/

    mDelaymS(10);

    /***********************************************************************
        RTC CONFIGURATION
    ***********************************************************************/

    hal_rtc_handle_t rtc;

    rtc = hal_rtc_init(Count_32);

    hal_rtc_attach_cb(
        rtc,
        rtc_callback,
        NULL);

    /*
     * Enable RTC as wake source
     */

    hal_pwr_wakeup_cfg(
        RB_SLP_RTC_WAKE,
        HAL_PWR_WAKE_DELAY_512);

    /*
     * RTC runs at 32768 Hz
     *
     * 60 seconds =
     * 60 * 32768
     */

    hal_rtc_set_trigger(
        rtc,
        WAKE_SECONDS * 32768UL);

    /***********************************************************************
        ENTER SHUTDOWN
    ***********************************************************************/

    /*
     * Deepest low power mode.
     *
     * RTC remains active.
     * Wakeup causes software reset.
     */

    hal_pwr_shutdown(0);

    while(1)
    {
    }
}
