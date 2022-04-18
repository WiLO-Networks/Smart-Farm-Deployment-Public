/**
 *
 * Copyright (c) 2021 by WiLO Networks Inc., Chenye Yang
 *
 */

#include "CapacitiveSoilMoistureV1_2.h"


/*!
* \brief constructor. set the default value for air and water humidity. 
*
* \param [IN]: (data_pin)     moisture_data_pin
*/
Sensors_Capacitive_Soil_Moisture::Sensors_Capacitive_Soil_Moisture(uint8_t data_pin)
{
    moisture_data_pin = data_pin;
    // pre-calibrated value
    air_humidity = 520;
    water_humidity = 15;
}

/*!
* \brief read the sensor value when exposed in air
*
*/
void Sensors_Capacitive_Soil_Moisture::Calibrate_Air()
{
    air_humidity = analogRead(moisture_data_pin);
    Serial.println(air_humidity);
}

/*!
* \brief read the sensor value when exposed in water
*
*/
void Sensors_Capacitive_Soil_Moisture::Calibrate_Water()
{
    water_humidity = analogRead(moisture_data_pin);
    Serial.println(water_humidity);
}

/*!
* \brief return the sensor value
*
*/
uint16_t Sensors_Capacitive_Soil_Moisture::Get_Moisture_Data_Raw()
{
    return analogRead(moisture_data_pin);
}

/*!
* \brief return 3 for very wet, return 2 for wet, return 1 for dry, return 0 for invalid
*
*/
uint8_t Sensors_Capacitive_Soil_Moisture::Get_Moisture_Data_Calibrated()
{
    uint16_t intervals = (air_humidity - water_humidity) / 3;

    uint16_t soil_humidity = analogRead(moisture_data_pin);

    if (soil_humidity > water_humidity && soil_humidity < (water_humidity + intervals)) {
        return 3; // very wet
    } else if (soil_humidity > (water_humidity + intervals) && soil_humidity < (air_humidity - intervals)) {
        return 2; // wet
    } else if(soil_humidity < air_humidity && soil_humidity > (air_humidity - intervals)) {
        return 1; // dry
    } else {
        return 0; // invalid
    }
}
