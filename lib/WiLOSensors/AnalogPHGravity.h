/**
 *
 * Copyright (c) 2022 by WiLO Networks Inc., Chenye Yang
 *
 */

#ifndef ANALOGPHGRAVITY_H
#define ANALOGPHGRAVITY_H

#include <Arduino.h>


class Sensors_Analog_pH_Gravity
{
    // datasheet: https://atlas-scientific.com/kits/gravity-analog-ph-kit/

private:
    uint8_t ph_data_pin;

public:
    Sensors_Analog_pH_Gravity(uint8_t data_pin = 12);

    uint16_t Get_pH_Data_Raw();
    float_t Get_pH_Data_Converted();

};


#endif