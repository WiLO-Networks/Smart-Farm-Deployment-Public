/**
 *
 * Copyright (c) 2022 by WiLO Networks Inc., Chenye Yang
 *
 */

#include "AnalogPHGravity.h"


/*!
* \brief constructor. set the analog data pin
*
* \param [IN]: (data_pin)     ph_data_pin
*/
Sensors_Analog_pH_Gravity::Sensors_Analog_pH_Gravity(uint8_t data_pin)
{
    ph_data_pin = data_pin;
}


/*!
* \brief return the raw analog read of sensor value
*
*/
uint16_t Sensors_Analog_pH_Gravity::Get_pH_Data_Raw()
{
    return analogRead(ph_data_pin);
}


/*!
* \brief return the converted pH value
*
*/
float_t Sensors_Analog_pH_Gravity::Get_pH_Data_Converted()
{
    float_t voltage = float(analogRead(ph_data_pin)) / 4095.0 * 3.3;
    float_t ph = ( -5.6548 * voltage ) + 15.509;
    return ph;
}