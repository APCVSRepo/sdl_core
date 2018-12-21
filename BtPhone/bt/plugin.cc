/*
 * Copyright (c) 2018, Ford Motor Company
 * All rights reserved.
 * 
 */


#include "include/plugin.h"
#include "bluetooth_packet.h"
#include "bluetooth_channel.h"
#include "packet_bluetooth.h"

#include <qqml.h>

Plugin::Plugin() {

}


void Plugin::registerTypes(const char *uri)
{
    
    qmlRegisterType<bluetooth_channel>(uri, 1, 0, "bluetooth_channel");
    qmlRegisterType<bluetooth_packet>(uri, 1, 0, "bluetooth_packet");
    qmlRegisterType<packet_bluetooth>(uri, 1, 0, "packet_bluetooth");
    
}