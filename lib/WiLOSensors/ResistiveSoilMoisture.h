/**
 *
 * Copyright (c) 2022 by WiLO Networks Inc., Chenye Yang
 *
 */

#ifndef RESISTIVESOILMOISTURE_H
#define RESISTIVESOILMOISTURE_H

#include <Arduino.h>


class Sensors_Resistive_Soil_Moisture
{
    // purchase link: https://www.amazon.com/Icstation-Resistive-Soil-Moisture-Sensor/dp/B076DDWDJK/ref=sr_1_2?keywords=soil+ph+sensor+for+arduino&qid=1645459524&sr=8-2
    // datasheet: https://images-na.ssl-images-amazon.com/images/I/81SaGn76xgL.pdf 

private:
    uint8_t moisture_data_pin;

public:
    Sensors_Resistive_Soil_Moisture(uint8_t data_pin = 13);

    uint16_t Get_Moisture_Data_Raw();

};


#endif