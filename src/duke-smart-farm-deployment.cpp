/*

Module:
    duke-smart-farm-deployment.cpp

Function:
    This file is used for Duke smart farm deployment. Nodes work in Class A mode.

    This file should be used with configuration file "platformio-duke-smart-farm-deployment.ini" (replacing "platformio.ini" with it). 

Copyright notice and License:
    See LICENSE file accompanying this project.

Author:
    Chenye Yang, WiLO Networks Inc., 2022

*/

#if GATEWAY

#include <Arduino.h>
#include <ArduinoJson.h>

#include "WiLOLoRa.h"

using namespace std;

/*
we formerly would check this configuration; but now there is a flag,
in the LMIC, LMIC.noRXIQinversion;
if we set that during init, we get the same effect.  If
DISABLE_INVERT_IQ_ON_RX is defined, it means that LMIC.noRXIQinversion is
treated as always set.

#if !defined(DISABLE_INVERT_IQ_ON_RX)
#error This example requires DISABLE_INVERT_IQ_ON_RX to be set. Update \ 
       lmic_project_config.h in arduino-lmic/project_config to set it.
#endif

How often to send a packet. Note that this sketch bypasses the normal
LMIC duty cycle limiting, so when you change anything in this sketch
(payload length, frequency, spreading factor), be sure to check if
this interval should not also be increased.
See this spreadsheet for an easy airtime and duty cycle calculator:
https://docs.google.com/spreadsheets/d/1voGAtQAjC1qBmaVuP1ApNKs1ekgUjavHuVQIXyYSvNc
*/

#define TX_INTERVAL 2000        // milliseconds
#define RX_RSSI_INTERVAL 100    // milliseconds

// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in arduino-lmoc/project_config/lmic_project_config.h,
// otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

// this gets called by the library but we choose not to display any info;
// and no action is required.
void onEvent (ev_t ev) {
}

/*************************************************************\
|     Work around for inconsistency in providing
|     Serial.dtr() method
\*************************************************************/

// Use SFINAE to deal with lack of Serial.dtr() on some platforms
template<class T>
auto getDtr_help(T* obj)
 -> decltype(  obj->dtr()  )
{
    return     obj->dtr();
}
// use this if there's no dtr() method
auto getDtr_help(...) -> bool
{
    return false;
}

// this wrapper lets us avoid use of explicit pointers
template<class T>
bool getDtr(T &obj)
{
    return getDtr_help(&obj);
}

/*************************************************************\
|     Print stub for use by LMIC
\*************************************************************/

extern "C" {
void lmic_printf(const char *fmt, ...);
};

void lmic_printf(const char *fmt, ...) {
    if (! getDtr(Serial))
        return;

    char buf[256];
    va_list ap;

    va_start(ap, fmt);
    (void) vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
    va_end(ap);

    // in case we overflowed:
    buf[sizeof(buf) - 1] = '\0';
    if (getDtr(Serial)) Serial.print(buf);
}


/* *****************************************
 * Gateway initialization
 * ****************************************/

WiLOGateway Gateway;

#if OLED_DISPLAY
#include "SSD1306.h"

SSD1306  Display(0x3c, SDA_OLED, SCL_OLED, RST_OLED);

void OLED_Chip_Info(String chip_freq)
{
    Display.init();
    Display.wakeup();

    delay(100);

    Display.setFont(ArialMT_Plain_16);
    Display.setTextAlignment(TEXT_ALIGN_LEFT);
    Display.clear();
    Display.drawString(0, 0, "GW");
    Display.drawString(54, 0, chip_freq);
    Display.drawString(0, 16, Gateway.chip_mac);
    Display.drawString(0, 32, "Ver. 1.1"); // light sleep stategy 1

    Display.display();
}
#endif

/*************************************************************\
|     Application logic
\*************************************************************/


static void Rx_Func (osjob_t* job);


/* uplink process */

void Process_Data (char * source_mac, char * dest_mac, char * rcvd_data);



static void Rx_Func (osjob_t* job)
{
    // Blink once to confirm reception and then keep the led on
    digitalWrite(LED_BUILTIN, HIGH); // on
    delay(10);
    digitalWrite(LED_BUILTIN, LOW); // off



    Serial.print("Got ");
    Serial.print(LMIC.dataLen);
    Serial.println(" bytes");
    Serial.write(LMIC.frame, LMIC.dataLen); 
    Serial.println();

    memcpy(Gateway.rxpacket, LMIC.frame, LMIC.dataLen );
    Gateway.rxpacket[LMIC.dataLen]='\0';

    // handle the received packet. e.g. rxpacket = 
    // "D C4AC50BD9E7C C4AC50BD9E7C XX"
    char prefix;
    strncpy(&prefix, Gateway.rxpacket, 1);

    char source_mac[13];
    strncpy(source_mac, Gateway.rxpacket + 2 /* offset for prefix */, 12);
    source_mac[12] = '\0';

    switch (prefix)
    {

    case 'D': {
        char destination_mac[13];
        strncpy(destination_mac, Gateway.rxpacket + 15 /* offset for prefix and source mac */, 12);
        destination_mac[12] = '\0';

        // print the data received
        char rcvd_data[227];
        strncpy(rcvd_data, Gateway.rxpacket + 28 /* offset for prefix and macs */, sizeof(rcvd_data)-1);
        rcvd_data[sizeof(rcvd_data)-1] = '\0';
        Serial.printf("Received data: %s\n", rcvd_data);

        Process_Data(source_mac, destination_mac, rcvd_data);
    }
        break;
    
    default: {
        Serial.println("Not a valid message");
        // discard it 
        Gateway.Rx(Rx_Func); // Back to Rx
    }
        break;
    }

}



/*!
* \brief process the received DATA packet and response properly. Back to Rx
* 
* \param [IN]: (source_mac)     source mac address of the DATA
* \param [IN]: (dest_mac)       destination mac address of the DATA
* \param [IN]: (rcvd_data)      received data content
*/
void Process_Data (char * source_mac, char * dest_mac, char * rcvd_data)
{
    if ( strcmp(dest_mac, Gateway.chip_mac) == 0 ) { // DATA sent to me
        
#if SERIAL_RELEASE
        DynamicJsonDocument data_json(256);
        deserializeJson(data_json, rcvd_data); // decode received json doc
        data_json["mac"] = source_mac; // add source mac address to it
        data_json["rssi"] = LMIC.rssi; // add RSSI to it
        string data_str;
        serializeJson(data_json, data_str); // encode the json doc to a json string for serial print
        Serial.printf("!DATA %s\n", data_str.c_str());
#endif

        Gateway.Rx(Rx_Func); // Back to Rx

    } else { // DATA not to me. 
        Serial.printf("Sent to %s, not me\n", dest_mac);
        // discard it
        Gateway.Rx(Rx_Func); // Back to Rx
    }
}





// application entry point
void setup()
{
    // delay(3000) makes recovery from botched images much easier, as it
    // gives the host time to break in to start a download. Without it,
    // you get to the crash before the host can break in.
    delay(3000);

    // even after the delay, we wait for the host to open the port. operator
    // bool(Serial) just checks dtr(), and it tosses in a 10ms delay.
    //   while(! getDtr(Serial))
            /* wait for the PC */;

    Serial.begin(115200);
    Serial.println("Starting");

    pinMode(LED_BUILTIN, OUTPUT);

    // initialize runtime env
    // don't die mysteriously; die noisily.
    const lmic_pinmap *pPinMap = Arduino_LMIC::GetPinmap_ThisBoard();

    if (pPinMap == nullptr) {
        pinMode(LED_BUILTIN, OUTPUT);
        for (;;) {
            // flash lights, sleep.
            for (int i = 0; i < 5; ++i) {
                digitalWrite(LED_BUILTIN, 1);
                delay(100);
                digitalWrite(LED_BUILTIN, 0);
                delay(900);
            }
            Serial.println(F("board not known to library; add pinmap or update getconfig_thisboard.cpp"));
        }
    }

    os_init_ex(pPinMap);


    // Set up these settings once, and use them for both TX and RX
#ifdef ARDUINO_ARCH_STM32
    LMIC_setClockError(10*65536/100);
#endif

#if defined(CFG_eu868)
    // Use a frequency in the g3 which allows 10% duty cycling.
    LMIC.freq = 869525000;
    // Use a medium spread factor. This can be increased up to SF12 for
    // better range, but then, the interval should be (significantly)
    // raised to comply with duty cycle limits as well.
    LMIC.datarate = DR_SF9;
    // Maximum TX power
    LMIC.txpow = 27;
#elif defined(CFG_us915)
    // make it easier for test, by pull the parameters up to the top of the
    // block. Ideally, we'd use the serial port to drive this; or have
    // a voting protocol where one side is elected the controller and
    // guides the responder through all the channels, powers, ramps
    // the transmit power from min to max, and measures the RSSI and SNR.
    // Even more amazing would be a scheme where the controller could
    // handle multiple nodes; in that case we'd have a way to do
    // production test and qualification. However, using an RWC5020A
    // is a much better use of development time.

    // set fDownlink true to use a downlink channel; false
    // to use an uplink channel. Generally speaking, uplink
    // is more interesting, because you can prove that gateways
    // *should* be able to hear you.
    const static bool fDownlink = false;

    // the downlink channel to be used.
    const static uint8_t kDownlinkChannel = 3;

    // the uplink channel to be used.
    const static uint8_t kUplinkChannel = 8 + 3;

    // this is automatically set to the proper bandwidth in kHz,
    // based on the selected channel.
    uint32_t uBandwidth;

    if (! fDownlink) {
        if (kUplinkChannel < 64) {
                LMIC.freq = US915_125kHz_UPFBASE +
                            kUplinkChannel * US915_125kHz_UPFSTEP;
                uBandwidth = 125;
            }
        else {
                LMIC.freq = US915_500kHz_UPFBASE +
                            (kUplinkChannel - 64) * US915_500kHz_UPFSTEP;
                uBandwidth = 500;
            }
    }
    else {
        // downlink channel
        LMIC.freq = US915_500kHz_DNFBASE +
                    kDownlinkChannel * US915_500kHz_DNFSTEP;
        uBandwidth = 500;
    }

    // Use a suitable spreading factor
    if (uBandwidth < 500)
        LMIC.datarate = US915_DR_SF7;         // DR4
    else
        LMIC.datarate = US915_DR_SF12CR;      // DR8

    // default tx power for US: 21 dBm
    LMIC.txpow = 21;
#elif defined(CFG_au915)
    // make it easier for test, by pull the parameters up to the top of the
    // block. Ideally, we'd use the serial port to drive this; or have
    // a voting protocol where one side is elected the controller and
    // guides the responder through all the channels, powers, ramps
    // the transmit power from min to max, and measures the RSSI and SNR.
    // Even more amazing would be a scheme where the controller could
    // handle multiple nodes; in that case we'd have a way to do
    // production test and qualification. However, using an RWC5020A
    // is a much better use of development time.

    // set fDownlink true to use a downlink channel; false
    // to use an uplink channel. Generally speaking, uplink
    // is more interesting, because you can prove that gateways
    // *should* be able to hear you.
    const static bool fDownlink = false;

    // the downlink channel to be used.
    const static uint8_t kDownlinkChannel = 3;

    // the uplink channel to be used.
    const static uint8_t kUplinkChannel = 8 + 3;

    // this is automatically set to the proper bandwidth in kHz,
    // based on the selected channel.
    uint32_t uBandwidth;

    if (! fDownlink)
    {
        if (kUplinkChannel < 64)
        {
            LMIC.freq = AU915_125kHz_UPFBASE +
                        kUplinkChannel * AU915_125kHz_UPFSTEP;
            uBandwidth = 125;
        }
        else
        {
            LMIC.freq = AU915_500kHz_UPFBASE +
                        (kUplinkChannel - 64) * AU915_500kHz_UPFSTEP;
            uBandwidth = 500;
        }
    }
    else
    {
        // downlink channel
        LMIC.freq = AU915_500kHz_DNFBASE +
                    kDownlinkChannel * AU915_500kHz_DNFSTEP;
        uBandwidth = 500;
    }

    // Use a suitable spreading factor
    if (uBandwidth < 500)
        LMIC.datarate = AU915_DR_SF7;         // DR4
    else
        LMIC.datarate = AU915_DR_SF12CR;      // DR8

    // default tx power for AU: 30 dBm
    LMIC.txpow = 30;
#elif defined(CFG_as923)
    // make it easier for test, by pull the parameters up to the top of the
    // block. Ideally, we'd use the serial port to drive this; or have
    // a voting protocol where one side is elected the controller and
    // guides the responder through all the channels, powers, ramps
    // the transmit power from min to max, and measures the RSSI and SNR.
    // Even more amazing would be a scheme where the controller could
    // handle multiple nodes; in that case we'd have a way to do
    // production test and qualification. However, using an RWC5020A
    // is a much better use of development time.
    const static uint8_t kChannel = 0;
    uint32_t uBandwidth;

    LMIC.freq = AS923_F1 + kChannel * 200000;
    uBandwidth = 125;

    // Use a suitable spreading factor
    if (uBandwidth == 125)
        LMIC.datarate = AS923_DR_SF7;         // DR7
    else
        LMIC.datarate = AS923_DR_SF7B;        // DR8

    // default tx power for AS: 21 dBm
    LMIC.txpow = 16;

    if (LMIC_COUNTRY_CODE == LMIC_COUNTRY_CODE_JP)
    {
        LMIC.lbt_ticks = us2osticks(AS923JP_LBT_US);
        LMIC.lbt_dbmax = AS923JP_LBT_DB_MAX;
    }
#elif defined(CFG_kr920)
    // make it easier for test, by pull the parameters up to the top of the
    // block. Ideally, we'd use the serial port to drive this; or have
    // a voting protocol where one side is elected the controller and
    // guides the responder through all the channels, powers, ramps
    // the transmit power from min to max, and measures the RSSI and SNR.
    // Even more amazing would be a scheme where the controller could
    // handle multiple nodes; in that case we'd have a way to do
    // production test and qualification. However, using an RWC5020A
    // is a much better use of development time.
    const static uint8_t kChannel = 0;
    uint32_t uBandwidth;

    LMIC.freq = KR920_F1 + kChannel * 200000;
    uBandwidth = 125;

    LMIC.datarate = KR920_DR_SF7;         // DR7
    // default tx power for KR: 14 dBm
    LMIC.txpow = KR920_TX_EIRP_MAX_DBM;
    if (LMIC.freq < KR920_F14DBM)
        LMIC.txpow = KR920_TX_EIRP_MAX_DBM_LOW;

    LMIC.lbt_ticks = us2osticks(KR920_LBT_US);
    LMIC.lbt_dbmax = KR920_LBT_DB_MAX;
#elif defined(CFG_in866)
    // make it easier for test, by pull the parameters up to the top of the
    // block. Ideally, we'd use the serial port to drive this; or have
    // a voting protocol where one side is elected the controller and
    // guides the responder through all the channels, powers, ramps
    // the transmit power from min to max, and measures the RSSI and SNR.
    // Even more amazing would be a scheme where the controller could
    // handle multiple nodes; in that case we'd have a way to do
    // production test and qualification. However, using an RWC5020A
    // is a much better use of development time.
    const static uint8_t kChannel = 0;
    uint32_t uBandwidth;

    LMIC.freq = IN866_F1 + kChannel * 200000;
    uBandwidth = 125;

    LMIC.datarate = IN866_DR_SF7;         // DR7
    // default tx power for IN: 30 dBm
    LMIC.txpow = IN866_TX_EIRP_MAX_DBM;
#else
# error Unsupported LMIC regional configuration.
#endif


    // disable RX IQ inversion
    LMIC.noRXIQinversion = true;

    // This sets CR 4/5, BW125 (except for EU/AS923 DR_SF7B, which uses BW250)
    LMIC.rps = updr2rps(LMIC.datarate);

    Serial.println("***** Gateway *****");
    Serial.println(Gateway.chip_mac);
    Serial.print("Frequency: "); Serial.print(LMIC.freq / 1000000);
        Serial.print("."); Serial.print((LMIC.freq / 100000) % 10);
        Serial.print("MHz");
    Serial.print("  LMIC.datarate: "); Serial.print(LMIC.datarate);
    Serial.print("  LMIC.txpow: "); Serial.println(LMIC.txpow);
    Serial.println("Started");
    Serial.flush();

#if OLED_DISPLAY
    String freqStr = String(LMIC.freq / 1000000) + "." + String((LMIC.freq / 100000) % 10) + "MHz";
    OLED_Chip_Info(freqStr);
#endif

    // start to Rx
    Gateway.Rx(Rx_Func);

}

void loop()
{
    // execute scheduled jobs and events
    os_runloop_once();
}

#endif


#if NODE

#include <Arduino.h>
#include <ArduinoJson.h>

#include "WiLOLoRa.h"
#include "WiLOSensors.h"

using namespace std;

/*
we formerly would check this configuration; but now there is a flag,
in the LMIC, LMIC.noRXIQinversion;
if we set that during init, we get the same effect.  If
DISABLE_INVERT_IQ_ON_RX is defined, it means that LMIC.noRXIQinversion is
treated as always set.

#if !defined(DISABLE_INVERT_IQ_ON_RX)
#error This example requires DISABLE_INVERT_IQ_ON_RX to be set. Update \ 
       lmic_project_config.h in arduino-lmic/project_config to set it.
#endif

How often to send a packet. Note that this sketch bypasses the normal
LMIC duty cycle limiting, so when you change anything in this sketch
(payload length, frequency, spreading factor), be sure to check if
this interval should not also be increased.
See this spreadsheet for an easy airtime and duty cycle calculator:
https://docs.google.com/spreadsheets/d/1voGAtQAjC1qBmaVuP1ApNKs1ekgUjavHuVQIXyYSvNc
*/

#define TX_INTERVAL 2000        // milliseconds
#define RX_RSSI_INTERVAL 100    // milliseconds

// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in arduino-lmoc/project_config/lmic_project_config.h,
// otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

// this gets called by the library but we choose not to display any info;
// and no action is required.
void onEvent (ev_t ev) {
}

/*************************************************************\
|     Work around for inconsistency in providing
|     Serial.dtr() method
\*************************************************************/

// Use SFINAE to deal with lack of Serial.dtr() on some platforms
template<class T>
auto getDtr_help(T* obj)
 -> decltype(  obj->dtr()  )
{
    return     obj->dtr();
}
// use this if there's no dtr() method
auto getDtr_help(...) -> bool
{
    return false;
}

// this wrapper lets us avoid use of explicit pointers
template<class T>
bool getDtr(T &obj)
{
    return getDtr_help(&obj);
}

/*************************************************************\
|     Print stub for use by LMIC
\*************************************************************/

extern "C" {
void lmic_printf(const char *fmt, ...);
};

void lmic_printf(const char *fmt, ...) {
    if (! getDtr(Serial))
        return;

    char buf[256];
    va_list ap;

    va_start(ap, fmt);
    (void) vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
    va_end(ap);

    // in case we overflowed:
    buf[sizeof(buf) - 1] = '\0';
    if (getDtr(Serial)) Serial.print(buf);
}


/* *****************************************
 * Node initialization
 * ****************************************/

WiLONode Node;

#if OLED_DISPLAY
#include "SSD1306.h"

SSD1306  Display(0x3c, SDA_OLED, SCL_OLED, RST_OLED);

void OLED_Chip_Info(String chip_freq)
{
    Display.init();
    Display.wakeup();

    delay(100);

    Display.setFont(ArialMT_Plain_16);
    Display.setTextAlignment(TEXT_ALIGN_LEFT);
    Display.clear();
    Display.drawString(0, 0, "Node");
    Display.drawString(54, 0, chip_freq);
    Display.drawString(0, 16, Node.chip_mac);
    Display.drawString(0, 32, "Ver. 1.1"); // light sleep stategy 1

    Display.display();
}
#endif


/* *****************************************
 * Sensors (Powered by Vext pin)
 * ****************************************/

#if SENSORS

#define DS18B20_DATA_PIN 17
Sensors_Temperature_DS18B20 sensor_Temperature(DS18B20_DATA_PIN);

#define MOISTURE_DATA_PIN 13
Sensors_Resistive_Soil_Moisture sensor_Moisture(MOISTURE_DATA_PIN);

#define PH_DATA_PIN 12
Sensors_Analog_pH_Gravity sensor_PH(PH_DATA_PIN);

// battery measurement pin = 37
Battery_Life sensor_Battery;

#endif


/* uplink done handler */

static void DataDone_Func (osjob_t* job);

/* uplink Tx */

void Send_Data (char * dest_mac, string dataToSend);



static void DataDone_Func (osjob_t* job) {
}


/*!
* \brief use LoRa PHY to send out a data packet to ONE specific address. 
*        Packet: D chip_mac dest_mac data
*
* \param [IN]: (dest_mac)       destination mac address
* \param [IN]: (dataToSend)   data content to send
*/
void Send_Data (char * dest_mac, string dataToSend)
{
    sprintf(Node.txpacket, "%s ", "D");
    sprintf(Node.txpacket+strlen(Node.txpacket), "%s ", Node.chip_mac); // source address
    sprintf(Node.txpacket+strlen(Node.txpacket), "%s ", dest_mac); // destination address
    // sprintf(txpacket+strlen(txpacket), "%s ", "RSSI");
    sprintf(Node.txpacket+strlen(Node.txpacket), "%s", dataToSend.c_str()); // data to send

    Serial.printf("\r\nsending packet \"%s\" , length %d\r\n",Node.txpacket, strlen(Node.txpacket));

    Node.Tx(Node.txpacket, DataDone_Func);
}


// application entry point
void setup() {
    // delay(3000) makes recovery from botched images much easier, as it
    // gives the host time to break in to start a download. Without it,
    // you get to the crash before the host can break in.
    delay(3000);

    // even after the delay, we wait for the host to open the port. operator
    // bool(Serial) just checks dtr(), and it tosses in a 10ms delay.
    //   while(! getDtr(Serial))
            /* wait for the PC */;

    Serial.begin(115200);
    Serial.println("Starting");

    pinMode(LED_BUILTIN, OUTPUT);

    // initialize runtime env
    // don't die mysteriously; die noisily.
    const lmic_pinmap *pPinMap = Arduino_LMIC::GetPinmap_ThisBoard();

    if (pPinMap == nullptr) {
        pinMode(LED_BUILTIN, OUTPUT);
        for (;;) {
            // flash lights, sleep.
            for (int i = 0; i < 5; ++i) {
                digitalWrite(LED_BUILTIN, 1);
                delay(100);
                digitalWrite(LED_BUILTIN, 0);
                delay(900);
            }
            Serial.println(F("board not known to library; add pinmap or update getconfig_thisboard.cpp"));
        }
    }

    os_init_ex(pPinMap);


    // Set up these settings once, and use them for both TX and RX
#ifdef ARDUINO_ARCH_STM32
    LMIC_setClockError(10*65536/100);
#endif

#if defined(CFG_eu868)
    // Use a frequency in the g3 which allows 10% duty cycling.
    LMIC.freq = 869525000;
    // Use a medium spread factor. This can be increased up to SF12 for
    // better range, but then, the interval should be (significantly)
    // raised to comply with duty cycle limits as well.
    LMIC.datarate = DR_SF9;
    // Maximum TX power
    LMIC.txpow = 27;
#elif defined(CFG_us915)
    // make it easier for test, by pull the parameters up to the top of the
    // block. Ideally, we'd use the serial port to drive this; or have
    // a voting protocol where one side is elected the controller and
    // guides the responder through all the channels, powers, ramps
    // the transmit power from min to max, and measures the RSSI and SNR.
    // Even more amazing would be a scheme where the controller could
    // handle multiple nodes; in that case we'd have a way to do
    // production test and qualification. However, using an RWC5020A
    // is a much better use of development time.

    // set fDownlink true to use a downlink channel; false
    // to use an uplink channel. Generally speaking, uplink
    // is more interesting, because you can prove that gateways
    // *should* be able to hear you.
    const static bool fDownlink = false;

    // the downlink channel to be used.
    const static uint8_t kDownlinkChannel = 3;

    // the uplink channel to be used.
    const static uint8_t kUplinkChannel = 8 + 3;

    // this is automatically set to the proper bandwidth in kHz,
    // based on the selected channel.
    uint32_t uBandwidth;

    if (! fDownlink) {
        if (kUplinkChannel < 64) {
                LMIC.freq = US915_125kHz_UPFBASE +
                            kUplinkChannel * US915_125kHz_UPFSTEP;
                uBandwidth = 125;
            }
        else {
                LMIC.freq = US915_500kHz_UPFBASE +
                            (kUplinkChannel - 64) * US915_500kHz_UPFSTEP;
                uBandwidth = 500;
            }
    }
    else {
        // downlink channel
        LMIC.freq = US915_500kHz_DNFBASE +
                    kDownlinkChannel * US915_500kHz_DNFSTEP;
        uBandwidth = 500;
    }

    // Use a suitable spreading factor
    if (uBandwidth < 500)
        LMIC.datarate = US915_DR_SF7;         // DR4
    else
        LMIC.datarate = US915_DR_SF12CR;      // DR8

    // default tx power for US: 21 dBm
    LMIC.txpow = 21;
#elif defined(CFG_au915)
    // make it easier for test, by pull the parameters up to the top of the
    // block. Ideally, we'd use the serial port to drive this; or have
    // a voting protocol where one side is elected the controller and
    // guides the responder through all the channels, powers, ramps
    // the transmit power from min to max, and measures the RSSI and SNR.
    // Even more amazing would be a scheme where the controller could
    // handle multiple nodes; in that case we'd have a way to do
    // production test and qualification. However, using an RWC5020A
    // is a much better use of development time.

    // set fDownlink true to use a downlink channel; false
    // to use an uplink channel. Generally speaking, uplink
    // is more interesting, because you can prove that gateways
    // *should* be able to hear you.
    const static bool fDownlink = false;

    // the downlink channel to be used.
    const static uint8_t kDownlinkChannel = 3;

    // the uplink channel to be used.
    const static uint8_t kUplinkChannel = 8 + 3;

    // this is automatically set to the proper bandwidth in kHz,
    // based on the selected channel.
    uint32_t uBandwidth;

    if (! fDownlink)
    {
        if (kUplinkChannel < 64)
        {
            LMIC.freq = AU915_125kHz_UPFBASE +
                        kUplinkChannel * AU915_125kHz_UPFSTEP;
            uBandwidth = 125;
        }
        else
        {
            LMIC.freq = AU915_500kHz_UPFBASE +
                        (kUplinkChannel - 64) * AU915_500kHz_UPFSTEP;
            uBandwidth = 500;
        }
    }
    else
    {
        // downlink channel
        LMIC.freq = AU915_500kHz_DNFBASE +
                    kDownlinkChannel * AU915_500kHz_DNFSTEP;
        uBandwidth = 500;
    }

    // Use a suitable spreading factor
    if (uBandwidth < 500)
        LMIC.datarate = AU915_DR_SF7;         // DR4
    else
        LMIC.datarate = AU915_DR_SF12CR;      // DR8

    // default tx power for AU: 30 dBm
    LMIC.txpow = 30;
#elif defined(CFG_as923)
    // make it easier for test, by pull the parameters up to the top of the
    // block. Ideally, we'd use the serial port to drive this; or have
    // a voting protocol where one side is elected the controller and
    // guides the responder through all the channels, powers, ramps
    // the transmit power from min to max, and measures the RSSI and SNR.
    // Even more amazing would be a scheme where the controller could
    // handle multiple nodes; in that case we'd have a way to do
    // production test and qualification. However, using an RWC5020A
    // is a much better use of development time.
    const static uint8_t kChannel = 0;
    uint32_t uBandwidth;

    LMIC.freq = AS923_F1 + kChannel * 200000;
    uBandwidth = 125;

    // Use a suitable spreading factor
    if (uBandwidth == 125)
        LMIC.datarate = AS923_DR_SF7;         // DR7
    else
        LMIC.datarate = AS923_DR_SF7B;        // DR8

    // default tx power for AS: 21 dBm
    LMIC.txpow = 16;

    if (LMIC_COUNTRY_CODE == LMIC_COUNTRY_CODE_JP)
    {
        LMIC.lbt_ticks = us2osticks(AS923JP_LBT_US);
        LMIC.lbt_dbmax = AS923JP_LBT_DB_MAX;
    }
#elif defined(CFG_kr920)
    // make it easier for test, by pull the parameters up to the top of the
    // block. Ideally, we'd use the serial port to drive this; or have
    // a voting protocol where one side is elected the controller and
    // guides the responder through all the channels, powers, ramps
    // the transmit power from min to max, and measures the RSSI and SNR.
    // Even more amazing would be a scheme where the controller could
    // handle multiple nodes; in that case we'd have a way to do
    // production test and qualification. However, using an RWC5020A
    // is a much better use of development time.
    const static uint8_t kChannel = 0;
    uint32_t uBandwidth;

    LMIC.freq = KR920_F1 + kChannel * 200000;
    uBandwidth = 125;

    LMIC.datarate = KR920_DR_SF7;         // DR7
    // default tx power for KR: 14 dBm
    LMIC.txpow = KR920_TX_EIRP_MAX_DBM;
    if (LMIC.freq < KR920_F14DBM)
        LMIC.txpow = KR920_TX_EIRP_MAX_DBM_LOW;

    LMIC.lbt_ticks = us2osticks(KR920_LBT_US);
    LMIC.lbt_dbmax = KR920_LBT_DB_MAX;
#elif defined(CFG_in866)
    // make it easier for test, by pull the parameters up to the top of the
    // block. Ideally, we'd use the serial port to drive this; or have
    // a voting protocol where one side is elected the controller and
    // guides the responder through all the channels, powers, ramps
    // the transmit power from min to max, and measures the RSSI and SNR.
    // Even more amazing would be a scheme where the controller could
    // handle multiple nodes; in that case we'd have a way to do
    // production test and qualification. However, using an RWC5020A
    // is a much better use of development time.
    const static uint8_t kChannel = 0;
    uint32_t uBandwidth;

    LMIC.freq = IN866_F1 + kChannel * 200000;
    uBandwidth = 125;

    LMIC.datarate = IN866_DR_SF7;         // DR7
    // default tx power for IN: 30 dBm
    LMIC.txpow = IN866_TX_EIRP_MAX_DBM;
#else
# error Unsupported LMIC regional configuration.
#endif


    // disable RX IQ inversion
    LMIC.noRXIQinversion = true;

    // This sets CR 4/5, BW125 (except for EU/AS923 DR_SF7B, which uses BW250)
    LMIC.rps = updr2rps(LMIC.datarate);

    Serial.println("***** Node *****");
    Serial.println(Node.chip_mac);
    Serial.print("Frequency: "); Serial.print(LMIC.freq / 1000000);
        Serial.print("."); Serial.print((LMIC.freq / 100000) % 10);
        Serial.print("MHz");
    Serial.print("  LMIC.datarate: "); Serial.print(LMIC.datarate);
    Serial.print("  LMIC.txpow: "); Serial.println(LMIC.txpow);
    Serial.println("Started");
    Serial.flush();

#if OLED_DISPLAY
    String freqStr = String(LMIC.freq / 1000000) + "." + String((LMIC.freq / 100000) % 10) + "MHz";
    OLED_Chip_Info(freqStr);
#endif


    // uint32_t remained_wakeup_time = os_jobIsTimed(& sleepjob) ? osticks2ms(sleepjob.deadline - os_getTime()) : 30000 ; // ms
    uint32_t sleep_time = 2700000 + random(5000); // ms
    // setup the wakeup source for esp32 in light sleep mode
    esp_sleep_enable_timer_wakeup(sleep_time * 1000);


    // prepare data
    DynamicJsonDocument prep_data_json(227); // 256 - "D source_mac dest_mac \0"
#if SENSORS_TEMP
    prep_data_json["temp"] = sensor_Temperature.Get_Temperature_Data_C();
#endif
#if SENSORS_MOIS
    prep_data_json["mois"] = sensor_Moisture.Get_Moisture_Data_Raw();
#endif
#if SENSORS_PH
    string ph_str(10, '\0');
    int written_ph = snprintf(&ph_str[0], ph_str.size(), "%.2f", sensor_PH.Get_pH_Data_Converted());
    ph_str.resize(written_ph);
    prep_data_json["ph"] = ph_str;
#endif
#if SENSORS_BATTERY
    string battery_str(10, '\0');
    int written_bat = snprintf(&battery_str[0], battery_str.size(), "%.2f", sensor_Battery.Get_Battery_Life_Percentage());
    battery_str.resize(written_bat);
    sensor_Battery.After_Get_Data();
    prep_data_json["bat"] = battery_str;
#endif
#if LOCATION
    prep_data_json["lat"] = "40.8067311";
    prep_data_json["lng"] = "-73.9566026";
#endif
    // prep_data_json["par"] = "E02114F7C630";

    // Serialize JSON document
    string prep_data_str;
    serializeJson(prep_data_json, prep_data_str);

    char dest_mac[13];
    strncpy(dest_mac, "E02114F7C630", 12);
    dest_mac[12] = '\0';

    Send_Data( dest_mac, prep_data_str );

    // execute scheduled jobs and events
    os_runloop_once();

    esp_deep_sleep_start();

}

void loop() {

}

#endif