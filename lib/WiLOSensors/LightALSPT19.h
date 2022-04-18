/**
 *
 * Copyright (c) 2022 by WiLO Networks Inc., Chenye Yang
 *
 */

#ifndef LIGHTALSPT19_H
#define LIGHTALSPT19_H

#include <Arduino.h>


class Sensors_Light_PT19
{
    // purchase link: https://www.adafruit.com/product/2748

private:
    uint8_t analog_data_pin;

public:
    Sensors_Light_PT19(uint8_t data_pin = 12);

    uint16_t Get_Light_Data_Raw();

};


#endif