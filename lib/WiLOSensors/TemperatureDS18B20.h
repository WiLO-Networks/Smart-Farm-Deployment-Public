/**
 *
 * Copyright (c) 2021 by WiLO Networks Inc., Chenye Yang
 *
 */

#ifndef TEMPERATUREDS18B20_H
#define TEMPERATUREDS18B20_H

#include <Arduino.h>

#include <OneWire.h> 
#include <DallasTemperature.h>


class Sensors_Temperature_DS18B20
{
    // purchase link: https://www.amazon.com/Gikfun-DS18B20-Temperature-Waterproof-EK1083x3/dp/B012C597T0/ref=sr_1_1_sspa?keywords=DS18B20+Temperature+sensor&qid=1636762504&s=industrial&sr=1-1-spons&psc=1&spLa=ZW5jcnlwdGVkUXVhbGlmaWVyPUEyRUNFN0tVVTZXRkhXJmVuY3J5cHRlZElkPUEwODUyODk3MTdNSzFZQ1lCWUxRVyZlbmNyeXB0ZWRBZElkPUEwMjA0MTQ3MUtSMjY4UlVMQkZBRyZ3aWRnZXROYW1lPXNwX2F0ZiZhY3Rpb249Y2xpY2tSZWRpcmVjdCZkb05vdExvZ0NsaWNrPXRydWU=
    // datasheet: https://datasheets.maximintegrated.com/en/ds/DS18B20.pdf

private:
    OneWire oneWire; 
    DallasTemperature sensors_DS18B20;

    uint8_t DS18B20_data_pin;

public:
    Sensors_Temperature_DS18B20(uint8_t data_pin = 17);

    float Get_Temperature_Data_C ();
};

#endif