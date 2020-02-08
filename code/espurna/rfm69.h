/*

RFM69 MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

struct packet_t;

#if RFM69_SUPPORT

#include "libs/RFM69Wrap.h"

struct packet_t {
    unsigned long messageID;
    unsigned char packetID;
    unsigned char senderID;
    unsigned char targetID;
    char * key;
    char * value;
    int16_t rssi;
};

void rfm69Setup();

#endif // RFM69_SUPPORT == 1
