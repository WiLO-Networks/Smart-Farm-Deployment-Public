/**
 *
 * Copyright (c) 2021 by WiLO Networks Inc., Chenye Yang
 *
 */

#include "TemperatureDS18B20.h"


/*!
* \brief constructor, create the OneWire object and DallasTemperature object
*
* \param [IN]: (data_pin)     DS18B20_data_pin
*/
Sensors_Temperature_DS18B20::Sensors_Temperature_DS18B20(uint8_t data_pin)
{
    DS18B20_data_pin = data_pin;

    pinMode(21, OUTPUT);
    digitalWrite(21, LOW);

    // Setup a oneWire instance to communicate with any OneWire devices  
    // (not just Maxim/Dallas temperature ICs) 
    // Data wire is plugged into pin 17 on the Arduino by default
    oneWire = OneWire(DS18B20_data_pin);
    // Pass our oneWire reference to Dallas Temperature. 
    sensors_DS18B20 = DallasTemperature(&oneWire);
    sensors_DS18B20.begin();

    digitalWrite(21, HIGH);
}

/*!
* \brief read temperature data from sensor DS18B20
*
*/
float Sensors_Temperature_DS18B20::Get_Temperature_Data_C () 
{
    pinMode(21, OUTPUT);
    digitalWrite(21, LOW);

    // call sensors.requestTemperatures() to issue a global temperature 
    // request to all devices on the bus 
    Serial.print("Requesting temperatures...");

    sensors_DS18B20.requestTemperatures(); // Send the command to get temperature readings 

    Serial.println("DONE"); 

    float temperature = sensors_DS18B20.getTempCByIndex(0); // Why "byIndex"?  
    // You can have more than one DS18B20 on the same bus.  
    // 0 refers to the first IC on the wire 
    Serial.print("Temperature is: "); 
    Serial.println(temperature); 

    digitalWrite(21, HIGH);

    return temperature;
}