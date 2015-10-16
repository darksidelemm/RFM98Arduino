/*
  LoRa Radio Ground-station software
  LoRa Ground station using a HopeRF RFM98W Module
  
  Copyright (C) 2015 Mark Jessop <vk5qi@rfhead.net>

  The output from this code is intended to be parsed by lora_groundstation.py

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
#include <avr/wdt.h>
#include <SPI.h>
#include "RFM98W.h"

#define PAYLOAD_LENGTH 55


// UNCOMMENT ONE OF THESE BLOCKS, DEPENDING ON THE BOARD YOU USE.

// Seeeduino Mega (3.3V)
/*
#define SS_PIN  53
#define DIO0  A0
#define DIO5  A2
#define RST   A1
*/

// Seeduino (3.3V)

#define SS_PIN  10
#define DIO0  A0
#define DIO5  A2
#define RST   A1


double Frequency = 431.650e6;
double FrequencyOffset = 0.0; // TODO: Automatic Frequency Correction

char message[256];

RFM98W rfm(SS_PIN, DIO0, DIO5);

void setup(){
  Serial.begin(57600);
  wdt_enable(WDTO_8S); // Enable watchdog timer, in case the SPI link dies.
  wdt_reset();
  rfm.setLoRaMode(Frequency + FrequencyOffset);
  rfm.startReceiving();
}

void loop(){
    wdt_reset();
    rx_status();

    if(rfm.checkInterrupt()){
      get_packet();
    }
    wdt_reset();
    delay(1000);
}

void get_packet(){
    int packet_size = rfm.receiveMessage(message);
    packet_info(); // Send packet info message *before* we send the packet itself.
    Serial.print("PKT,");
    Serial.println(packet_size);
    Serial.write((uint8_t*)message, packet_size);
}

void rx_status(){
    uint8_t modem_status = rfm.readRegister(REG_MODEM_STATUS);
    int16_t rssi = rfm.getRSSI();
    uint8_t freq_error = rfm.readRegister(REG_FREQ_ERROR);

    uint8_t signal_detected = (modem_status&MODEM_STATUS_SIGNAL_DETECTED)>0;
    uint8_t signal_sync = (modem_status&MODEM_STATUS_SIGNAL_SYNC)>0;
    uint8_t rx_in_progress = (modem_status&MODEM_STATUS_RX_IN_PROGRESS)>0;
    uint8_t got_header = (modem_status&MODEM_STATUS_GOT_HEADER)>0;

    Serial.print("STATUS,");
    Serial.print(rssi);
    Serial.print(",");
    Serial.print(modem_status);
    Serial.println("");
}

void packet_info(){
    int16_t packet_rssi = rfm.getRSSIPacket();
    int8_t packet_snr = rfm.readRegister(REG_PACKET_SNR);
    int32_t freq_error = rfm.getFrequencyError();
    uint8_t lastMessageFlags = rfm.getLastMessageFlags();

    Serial.print("PKTINFO,");
    Serial.print(packet_rssi);
    Serial.print(",");
    Serial.print(packet_snr);
    Serial.print(",");
    Serial.print(freq_error);
    Serial.print(",");
    Serial.print(lastMessageFlags);
    Serial.println("");
}
