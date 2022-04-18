/**
 *
 * Copyright (c) 2022 by WiLO Networks Inc., Chenye Yang
 *
 */

#include "BatteryLife.h"


/*!
* \brief constructor. do nothing. 
*
*/
Battery_Life::Battery_Life()
{
    
}


/*!
* \brief return the raw analog read of battery voltage
*
*/
uint16_t Battery_Life::Get_Battery_Life_Raw()
{
    pinMode(21, OUTPUT);
    digitalWrite(21, LOW); // GPIO 37 will read the divided voltage V37 = 100 / 320 * VBAT
    return analogRead(battery_pin);
}

/*!
* \brief return the normalized read of battery life
*
*/
float Battery_Life::Get_Battery_Life_Percentage()
{
    pinMode(21, OUTPUT);
    digitalWrite(21, LOW); // GPIO 37 will read the divided voltage V37 = 100 / 320 * VBAT
    // 4095 <-> 3.3V (12 bit ADC)
    // V37 = analog_read / 4095 * 3.3
    // VBAT = 3.2 * V37
    // percentage = VBAT * 100 / 3.7
    return float(analogRead(battery_pin)) * 704 / 10101;
}


/*!
* \brief reset pin 21 to avoid power leak
*
*/
void Battery_Life::After_Get_Data()
{
    digitalWrite(21, HIGH);
}