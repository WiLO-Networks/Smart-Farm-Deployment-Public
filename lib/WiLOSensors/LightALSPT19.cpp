/**
 *
 * Copyright (c) 2022 by WiLO Networks Inc., Chenye Yang
 *
 */

#include "LightALSPT19.h"


/*!
* \brief constructor. set the pin map. 
*
* \param [IN]: (data_pin)     analog_data_pin
*/
Sensors_Light_PT19::Sensors_Light_PT19(uint8_t data_pin)
{
    analog_data_pin = data_pin;
}


/*!
* \brief return the sensor value
*
*/
uint16_t Sensors_Light_PT19::Get_Light_Data_Raw()
{
    return analogRead(analog_data_pin);
}

