/*
  LoRa Radio Ground-station software
  LoRa Ground station using a HopeRF RFM98W Module 
  
  Copyright (C) 2014 Mark Jessop <vk5qi@rfhead.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    For a full copy of the GNU General Public License, 
    see <http://www.gnu.org/licenses/>.
*/

#include <SPI.h>
#include "RFM98W.h"


#define SS_PIN  53
#define DIO0  A0
#define DIO5  A2
#define RST   A1

char message[256];

RFM98W rfm(SS_PIN, DIO0, DIO5);

void setup(){
  Serial.begin(57600);

  rfm.setLoRaMode();
  rfm.startReceiving();

}

void loop(){
    Serial.print("Current RSSI: ");
    Serial.println(rfm.getRSSI());

    if(rfm.checkRX()){
      int packet_size = rfm.receiveMessage(message);
      int16_t packet_rssi = rfm.getRSSIPacket();
      Serial.print("Got Packet RSSI: ");
      Serial.println(packet_rssi);
      Serial.print("Packet Data:");
      Serial.println(message);
    }
    delay(1000);

}