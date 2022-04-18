/**
 *
 * Copyright (c) 2021 by WiLO Networks Inc., Chenye Yang
 *
 */

#ifndef WILOLORA_H
#define WILOLORA_H

#include <Arduino.h>
// https://github.com/mcci-catena/arduino-lmic
#include <lmic.h>
#include <hal/hal.h>
#include <arduino_lmic_hal_boards.h>


#include <list>
#include <tuple>
#include <string>


using namespace std;


#define BUFFER_SIZE 256 // LoRa offers a maximum packet size of 256 bytes.

#ifndef RX_TIMEOUT
#define RX_TIMEOUT 0
#endif




typedef list< tuple<string, int16_t, uint8_t> > list_tuple_t; // list of tuple
// For child_list (children connected to it): 
// <mac address, rssi, timeout_count>
// <C4AC50BD9E7C, 0, 0>
// 
// For parent_list (all the possiable parents): 
// <mac address, rssi, hop-count distance to the parent>
// <14AB50BD9E7C, 0, 1>


class WiLOLoRa
{
public:

    WiLOLoRa();

    /* Chip MAC Address */

    char * chip_mac; // current esp32 mac address
    void Get_Chip_ID (char * chip_mac);

    /* LoRa PHY */

    char txpacket[BUFFER_SIZE];
    char rxpacket[BUFFER_SIZE];
    void Low_Power();
    void Tx(const char *str, osjobcb_t func);
    void Rx(osjobcb_t func);

    /* LoRa Mesh */

    list_tuple_t child_list; // children nodes connected to it

    /* Tuple List Functions */

    void Serial_Print_ListTuple (list_tuple_t listTuple);
    void Update_ListTuple_of_Mac (list_tuple_t & listTuple, char * macAdd, int16_t RSSI, uint8_t count);
    bool Mac_in_ListTuple (list_tuple_t & listTuple, char * macAdd);
    void Delete_Mac_From_ListTuple (list_tuple_t & listTuple, char * macAdd);

    /* Reset */

    void Reset_Mesh();

};

class WiLOGateway: public WiLOLoRa
{
public:

    WiLOGateway();

    /* Tuple List Functions */

    void Delete_Mac_From_ListTuple (list_tuple_t & listTuple, char * macAdd, list_tuple_t::iterator s_childlist_iterator);

};


class WiLONode: public WiLOLoRa
{
public:

    WiLONode();

    /* LoRa Mesh */

    list_tuple_t parent_list; // all the possible parents
    tuple<string, int16_t> parent_node; // currently chosen parent <mac address, rssi>
    bool parent_alive; // whether the parent node is alive
    uint8_t hop_to_gateway; // the hop count of this node to gateway

    /* Parent Node Functions */

    void Serial_Print_Parent_Node ();
    void Update_Parent_Node ();
    bool Parent_Node_Changed_or_NotAlive (tuple<string, int16_t> old_parent, tuple<string, int16_t> new_parent);

    /* Tuple List Functions */

    static bool Compare_Node_Tuple (tuple<string, int16_t, uint8_t> first, tuple<string, int16_t, uint8_t> second);
    bool Same_Node_Tuple (tuple<string, int16_t, uint8_t> first, tuple<string, int16_t, uint8_t> second);
    
    /* Reset */
    
    void Reset_Mesh();

};



#endif