/**
 *
 * Copyright (c) 2022 by WiLO Networks Inc., Chenye Yang
 *
 */

#include "ResistiveSoilMoisture.h"


/*!
* \brief constructor. set the data pin
*
* \param [IN]: (data_pin)     moisture_data_pin
*/
Sensors_Resistive_Soil_Moisture::Sensors_Resistive_Soil_Moisture(uint8_t data_pin)
{
    moisture_data_pin = data_pin;
}


/*!
* \brief return the sensor value
*
*/
uint16_t Sensors_Resistive_Soil_Moisture::Get_Moisture_Data_Raw()
{
    return analogRead(moisture_data_pin);
}
