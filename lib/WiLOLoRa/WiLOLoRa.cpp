/**
 *
 * Copyright (c) 2021 by WiLO Networks Inc., Chenye Yang
 *
 */

#include "WiLOLoRa.h"

/* *****************************************
 * WiLOLoRa
 * ****************************************/

WiLOLoRa::WiLOLoRa ()
{
    chip_mac = new char[13]; // memory space for chip_mac
    Get_Chip_ID(chip_mac);   // assign current esp32 mac address to chip_mac
}


/* Chip MAC Address */

/*!
* \brief assign the esp32 mac address to variable chip_mac
*
* \param [IN]: (chip_mac)        variable to store the chip mac address
*/
void WiLOLoRa::Get_Chip_ID (char * chip_mac)
{
    const uint64_t chip_mac_tmp = ESP.getEfuseMac(); // original chip id for mesh routing
    // char chip_mac[13]; // chip id in mac address format
    Serial.printf("ESP32ChipID=%04X",(uint16_t)(chip_mac_tmp>>32));//print High 2 bytes
    Serial.printf("%08X\n",(uint32_t)chip_mac_tmp);//print Low 4bytes.
    sprintf(chip_mac, "%04X", (uint16_t)(chip_mac_tmp>>32)); // remember to pre-declare a mem space for it
    sprintf(chip_mac+strlen(chip_mac), "%08X", (uint32_t)chip_mac_tmp);
}


/* LoRa PHY */

/*!
* \brief Set the radio to sleep mode, and the chip to light sleep mode. 
*        Before entering the low power, the wakeup source for esp32 should be set. 
*
*/
void WiLOLoRa::Low_Power()
{
    // the radio is probably in other mode; stop it.
    os_radio(RADIO_RST);
    // wait a bit so the radio can come out of other mode
    delay(1);
    Serial.println("[Low Power]");
    // set the esp32 to light sleep mode, pause the CPU and keep the main memory
    esp_light_sleep_start();
}

/*!
* \brief Transmit the given string and call the given function afterwards
*
*/
void WiLOLoRa::Tx(const char *str, osjobcb_t func)
{
    // the radio is probably in RX mode; stop it.
    os_radio(RADIO_RST);
    // wait a bit so the radio can come out of RX mode
    delay(1);

    // prepare data
    LMIC.dataLen = 0;
    while (*str)
        LMIC.frame[LMIC.dataLen++] = *str++;

    // set completion function.
    LMIC.osjob.func = func;

    // start the transmission
    os_radio(RADIO_TX);
    Serial.println("[Tx]");


    // TODO: calculate or use oscilloscope to decide the exact time needed for Txing a message, 
    //       and change this number. 
}

/*!
* \brief Enable rx mode and call func when a packet is received
*
*/
void WiLOLoRa::Rx(osjobcb_t func)
{
    // the radio is probably in other mode; stop it.
    os_radio(RADIO_RST);
    // wait a bit so the radio can come out of other mode
    delay(1);

    LMIC.osjob.func = func;
    LMIC.rxtime = os_getTime(); // RX _now_
    // Enable "continuous" RX (e.g. without a timeout, still stops after receiving a packet)
    os_radio(RADIO_RXON);

    Serial.println("[Rx]");

#if RX_TIMEOUT
    // Timeout RX (i.e. update led status) after 3 periods without RX
    os_setTimedCallback(&timeoutjob, os_getTime() + ms2osticks(3*TX_INTERVAL), RxTimeout_Func);
    Serial.print("RxTimeout job scheduled after "); Serial.print(3*TX_INTERVAL); Serial.println(" ms");
#endif

    // TODO: set a reasonable rxtimeout time
}


/* Tuple List Functions */

/*!
* \brief serial print a tuple list.
*        can be used to print the child_list or parent list
*
* \param [IN]: listTuple     the tuple list
*/
void WiLOLoRa::Serial_Print_ListTuple (list_tuple_t listTuple)
{
    Serial.println("MAC_address \t| RSSI \t| Timeout_count");
    list_tuple_t::iterator it;
    for (it = listTuple.begin(); it != listTuple.end(); ++it) {
        Serial.print(get<0>(*it).c_str()); Serial.print(" \t| "); Serial.printf("%d", get<1>(*it)); Serial.print(" \t| "); Serial.printf("%d\n", get<2>(*it));
    }
}

/*!
* \brief update the element in listTuple of a specific mac address (if exist), or add it (if not exist). 
*        can be used to modify the values of a node with the specified mac address in parent_list or child_list. 
*        mac address is the unique identifier. 
*
* \param [IN]: (listTuple)        which tuple list to update
* \param [IN]: (macAdd)           node mac address
* \param [IN]: (RSSI)             node mac rssi
* \param [IN]: (count)            node timeout_count or hop_count
* 
* \todo searching algorithm to make it more efficient
*/
void WiLOLoRa::Update_ListTuple_of_Mac (list_tuple_t & listTuple, char * macAdd, int16_t RSSI, uint8_t count)
{
    bool notFound = true;
    list_tuple_t::iterator it;
    for (it = listTuple.begin(); it != listTuple.end(); /* no need to '++it' because of erase() */) {
        if ( get<0>(*it) == macAdd ) {
            // tuple<char *, int16_t, uint8_t> tmpTuple = *it;
            it = listTuple.erase(it); // erase returns an iterator to the element past the (last) erased element
            listTuple.insert(it, tuple<string, int16_t, uint8_t>(macAdd, RSSI, count) ); // insert before the element whose iterator you pass it
            notFound = false;
            break;
        } else {
            ++it;
        }
    }
    if (notFound) { // didn't find the mac address in the current tuple list, or the current tuple list is empty, add this tuple element to the list
        listTuple.push_back( tuple<string, int16_t, uint8_t>(macAdd, RSSI, count) );
    }
}

/*!
* \brief whether the provided mac address is in the tuple list. 
*
* \param [IN]: (listTuple)        which tuple list to check
* \param [IN]: (macAdd)           node mac address
* 
* \todo searching algorithm to make it more efficient
*/
bool WiLOLoRa::Mac_in_ListTuple (list_tuple_t & listTuple, char * macAdd)
{
    bool notFound = true;
    list_tuple_t::iterator it;
    for (it = listTuple.begin(); it != listTuple.end(); ++it) {
        if ( get<0>(*it) == macAdd ) {
            notFound = false;
            break;
        }
    }
    return ( !notFound );
}


/*!
* \brief Delete one node with the specific macAdd from the listTuple
*
* \param [IN]: (listTuple)        the tuple list to delete node from
* \param [IN]: (macAdd)           node mac address
* 
*/
void WiLOLoRa::Delete_Mac_From_ListTuple (list_tuple_t & listTuple, char * macAdd)
{
    list_tuple_t::iterator it;
    for (it = listTuple.begin(); it != listTuple.end(); ++it) {
        if ( get<0>(*it) == macAdd ) { // if this macAdd is in my child_list
            listTuple.erase(it); // delete it
            Serial.printf("%s erased from %s", macAdd, listTuple == child_list ? "child_list" : "listTuple");
            break;
        }
    }
}


/* Reset */

/*!
* \brief Reset the LoRa mesh network, i.e. clear the child_list
* 
*/
void WiLOLoRa::Reset_Mesh()
{
    child_list.clear();
}


/* *****************************************
 * WiLOGateway
 * ****************************************/

WiLOGateway::WiLOGateway ()
{
    // If the parent constructor is non-parameterized, 
    // then it will be called automatically with the derived class
    // WiLOLoRa::WiLOLoRa () will be called automatically
}

/* Tuple List Functions */

/*!
* \brief Delete one node with the specific macAdd from the listTuple. 
*        If the listTuple to change is the child_list, and next child node to query data is this macAdd, skip this query
*        Rule: if I receive my child's message, but it's not sent to me, then delete this child. 
*
* \param [IN]: (listTuple)        the tuple list to delete node from
* \param [IN]: (macAdd)           node mac address
* \param [IN]: (s_childlist_iterator)        the query queue iterator of child_list
* 
*/
void WiLOGateway::Delete_Mac_From_ListTuple (list_tuple_t & listTuple, char * macAdd, list_tuple_t::iterator s_childlist_iterator)
{
    list_tuple_t::iterator it;
    for (it = listTuple.begin(); it != listTuple.end(); ++it) {
        if ( get<0>(*it) == macAdd ) { // if this macAdd is in my listTuple

            // if the listTuple to change is the child_list, and next child node to query data is this macAdd, skip this query
            if ( (listTuple == child_list) && (get<0>(*s_childlist_iterator) == macAdd) ) {
                ++s_childlist_iterator;
                // To solve the corruption caused by querying a node not in its child_list: 
                // i.e. just erased this node, and then tried to query it
                // C4005BBD9E7C erased from child_list[Rx]
                // Querying data from C4005BBD9E7C
                // CORRUPT HEAP: Bad head at xxxxxxxxx
            }

            listTuple.erase(it); // delete this macAdd
            Serial.printf("%s erased from %s", macAdd, listTuple == child_list ? "child_list" : "parent_list");
            break;
        }
    }
}


/* *****************************************
 * WiLONode
 * ****************************************/

WiLONode::WiLONode ()
{
    // If the parent constructor is non-parameterized, 
    // then it will be called automatically with the derived class
    // WiLOLoRa::WiLOLoRa () will be called automatically

    // assume an isolated node
    parent_alive = false;
    hop_to_gateway = 255;
}


/* Parent Node Functions */

/*!
* \brief serial print the parent node information: 
*        MAC address, RSSI, Alive
*/
void WiLONode::Serial_Print_Parent_Node ()
{
    Serial.println("Parent node info: ");
    Serial.print("  MAC address: \t"); Serial.println(get<0>(parent_node).c_str());
    Serial.print("  RSSI: \t"); Serial.println(get<1>(parent_node));
    Serial.print("  Alive: \t"); Serial.println(parent_alive ? "True" : "False");
    Serial.printf("My hop count to gateway: %d\n", hop_to_gateway);
}

/*!
* \brief update the parent node from the parent_list. 
*        choose the node with the strongest RSSI and with the smallest hop-count distance to the GW in the present moment. 
*        update the node's hop_to_gateway count. 
*
*/
void WiLONode::Update_Parent_Node ()
{
    if ( ( !get<0>(parent_node).empty() ) && parent_alive ) { // already has an alive parent node
        Serial.print("Current ");
        Serial_Print_Parent_Node();
        Serial.print("Updating parent node...");
    } else { // doesn't have an alive parent node
        Serial.println("Haven't join a network");
        Serial.print("Updating parent node...");
    }

    parent_list.sort(Compare_Node_Tuple);
    if ( parent_list.empty() ) { // if the parent_list is empty, manually set the parent_node to empty, otherwise it's not correctly set to empty
        parent_node = tuple<string, int16_t>();
        parent_alive = false;
        hop_to_gateway = 255;
    }
    else {
        parent_node = tuple<string, int16_t>( get<0>(* parent_list.begin()), get<1>(* parent_list.begin()) ); 
        hop_to_gateway = get<2>(* parent_list.begin()) + 1;
        // TODO: currently this sort method works fine, discuss if there are other sorting rules
    }

    Serial.println("Finished");
}

/*!
* \brief use after Update_Parent_Node(). return TRUE when the parent node got updated, or not alive. 
*
* \param [IN]: (old_parent)    old parent node <mac, rssi> (store this before Update_Parent_Node)
* \param [IN]: (new_parent)    new parent node <mac, rssi>
*/
bool WiLONode::Parent_Node_Changed_or_NotAlive (tuple<string, int16_t> old_parent, tuple<string, int16_t> new_parent)
{
    bool same_parent = false;
    if ( get<0>(old_parent).empty() && get<0>(new_parent).empty() ) { // two 'empty' parent nodes.
        parent_alive = false;
        same_parent = true;
    }
    else if ( (!get<0>(old_parent).empty()) && (!get<0>(new_parent).empty()) ) { // two 'full' parent nodes.
        same_parent = ( get<0>(old_parent) == get<0>(new_parent) );
    }
    else { // one empty, one full
        same_parent = false;
    }

    if ( same_parent && parent_alive ) {
        Serial.println("Same & Alive parent node after updaing. No need to send new JOIN request.");
        return false; // back to Rx mode
    } else {
        Serial.println("Parent node changed after updating, or not alive, sending JOIN request.");
        parent_alive = false; // already has an alive parent but changes to new parent, need to set alive = false
        return true; // Send out join request with probability 0.8
    }
}


/* Tuple List Functions */

/*!
* \brief compare two node tuples, return (first better than second). 
*        'better' means compare rssi when valid and not different too much, or compare hop count otherwise. 
*
* \param [IN]: (first)     first node tuple <mac, rssi, hop_count>
* \param [IN]: (second)    second node tuple <mac, rssi, hop_count>
* 
* \todo a better criterion for choosing between two nodes
*/
bool WiLONode::Compare_Node_Tuple (tuple<string, int16_t, uint8_t> first, tuple<string, int16_t, uint8_t> second)
{
    int16_t rssi_1 = get<1>(first);
    int16_t rssi_2 = get<1>(second);
    uint8_t hop_1 = get<2>(first);
    uint8_t hop_2 = get<2>(second);

    if (rssi_1 && rssi_2) { // if all the rssi are valid i.e. not 0
        if ( abs( rssi_1 - rssi_2 ) > 20 ) {
            return ( rssi_1 > rssi_2 ); // rssi differ too much, use rssi as criterion
        } else {
            return ( hop_1 < hop_2 ); // rssi similiar,  use hop count as criterion
        }
    } else {
        return (hop_1 < hop_2 ); // some rssi not valid, compare hop count
    }
}

/*!
* \brief compare two node tuples, return (whether they have same mac address). 
*
* \param [IN]: (first)     first node tuple <mac, rssi, hop_count>
* \param [IN]: (second)    second node tuple <mac, rssi, hop_count>
*/
bool WiLONode::Same_Node_Tuple (tuple<string, int16_t, uint8_t> first, tuple<string, int16_t, uint8_t> second)
{
    if ( get<0>(first).empty() && get<0>(second).empty() ) { // two 'empty' tuples.
        return true;
    }
    else if ( (!get<0>(first).empty()) && (!get<0>(second).empty()) ) { // two 'full' tuples.
        return ( get<0>(first) == get<0>(second) );
    }
    else { // one empty, one full
        return false;
    }
}


/* Reset */

/*!
* \brief Reset the LoRa mesh network, i.e. clear the child_list, parent_list, parent_node, parent_alive, hop_to_gateway
* 
*/
void WiLONode::Reset_Mesh()
{
    child_list.clear();
    parent_list.clear();
    // assume an isolated node
    parent_node = tuple<string, int16_t>();
    parent_alive = false;
    hop_to_gateway = 255;
}
