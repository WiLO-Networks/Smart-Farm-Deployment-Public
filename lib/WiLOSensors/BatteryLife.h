/**
 *
 * Copyright (c) 2022 by WiLO Networks Inc., Chenye Yang
 *
 */

#ifndef BATTERYLIFE_H
#define BATTERYLIFE_H

#include <Arduino.h>


class Battery_Life
{
    // Heltec WiFi LoRa V2.1 uses Pin 37 for battery detection: https://heltec-automation-docs.readthedocs.io/en/latest/esp32/wifi_lora_32/hardware_update_log.html#v2-1
    // Another post about battery measurement: http://community.heltec.cn/t/heltec-wifi-lora-v2-battery-management/147/32
    
private:
    uint8_t battery_pin = 37;

public:
    Battery_Life();

    uint16_t Get_Battery_Life_Raw();

    float Get_Battery_Life_Percentage();

    void After_Get_Data();

};


#endif