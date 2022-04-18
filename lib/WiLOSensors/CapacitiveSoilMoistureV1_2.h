/**
 *
 * Copyright (c) 2021 by WiLO Networks Inc., Chenye Yang
 *
 */

#ifndef CAPACITIVESOILMOISTUREV1_2_H
#define CAPACITIVESOILMOISTUREV1_2_H

#include <Arduino.h>


class Sensors_Capacitive_Soil_Moisture
{
    // purchase link: https://www.amazon.com/DAOKI-Capacitive-Corrosion-Resistant-Electronic/dp/B085XSQXC5/ref=sr_1_5?keywords=Capacitive+Soil+Moisture+Sensor+V1.2&qid=1636761078&qsid=137-6150841-3615939&sr=8-5&sres=B085XSQXC5%2CB07HQDWB39%2CB07RYQLP9R%2CB07ZZ418N5%2CB08JPQPMCT%2CB07SYBSHGX%2CB094J8XD83%2CB07H3P1NRM%2CB08LDDTT44%2CB014MJ8J2U%2CB07RYCNFZ5%2CB08TG6VJSS%2CB07BR52P26%2CB087DYDNGZ%2CB098TBK9JK%2CB01N7NA3HP%2CB08RMPVY6Z%2CB07W83ZVFB%2CB01DK29K28%2CB01DKISKLO

private:
    uint16_t air_humidity;
    uint16_t water_humidity;
    uint8_t moisture_data_pin;

public:
    Sensors_Capacitive_Soil_Moisture(uint8_t data_pin = 13);

    void Calibrate_Air();
    void Calibrate_Water();

    uint16_t Get_Moisture_Data_Raw();
    uint8_t Get_Moisture_Data_Calibrated();

};


#endif