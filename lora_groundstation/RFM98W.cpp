/*
  RFM98W.cpp - RFM98W Comms Library
  
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

#include "Arduino.h"
#include <SPI.h>
#include "RFM98W.h"


RFM98W::RFM98W(uint8_t SS, uint8_t DIO0, uint8_t DIO5){
    this->SS_PIN = SS;
    this->DIO0_PIN = DIO0;
    this->DIO5_PIN = DIO5;

    this->currentMode = 0x81;

    pinMode(this->SS_PIN, OUTPUT);
    digitalWrite(this->SS_PIN, HIGH);
    pinMode(this->DIO0_PIN, INPUT);
    pinMode(this->DIO5_PIN, INPUT);

    SPI.begin();
}

void RFM98W::writeRegister(byte addr, byte value){
    digitalWrite(this->SS_PIN, LOW);
    SPI.transfer(addr | 0x80);
    SPI.transfer(value);
    digitalWrite(this->SS_PIN, HIGH);
}

byte RFM98W::readRegister(byte addr){
    digitalWrite(this->SS_PIN, LOW);
    SPI.transfer(addr & 0x7F);
    byte regval = SPI.transfer(0);
    digitalWrite(this->SS_PIN, HIGH);
    return regval;
}

int16_t RFM98W::getRSSI(){
    uint8_t rssi = this->readRegister(REG_RSSI_CURRENT);
    return (int16_t)rssi - 137;
}

uint8_t RFM98W::checkRX(){
    return digitalRead(this->DIO0_PIN);
}

int16_t RFM98W::getRSSIPacket(){
    uint8_t rssi = this->readRegister(REG_RSSI_PACKET);
    return (int16_t)rssi - 137;
}

void RFM98W::setMode(byte newMode)
{
  if(newMode == currentMode)
    return;  
  
  switch (newMode) 
  {
    case RF96_MODE_RX_CONTINUOUS:
      this->writeRegister(REG_PA_CONFIG, PA_OFF_BOOST);  // TURN PA OFF FOR RECIEVE??
      this->writeRegister(REG_LNA, LNA_MAX_GAIN);  // LNA_MAX_GAIN);  // MAX GAIN FOR RECIEVE
      this->writeRegister(REG_OPMODE, newMode);
      this->currentMode = newMode; 
      Serial.println("Changing to Receive Continuous Mode\n");
      break;
      
      break;
    case RF96_MODE_SLEEP:
      Serial.println("Changing to Sleep Mode"); 
      this->writeRegister(REG_OPMODE, newMode);
      this->currentMode = newMode; 
      break;
    case RF96_MODE_STANDBY:
      Serial.println("Changing to Standby Mode");
      this->writeRegister(REG_OPMODE, newMode);
      this->currentMode = newMode; 
      break;
    default: return;
  } 
  
  if(newMode != RF96_MODE_SLEEP){
    while(digitalRead(this->DIO5_PIN) == 0)
    {
      Serial.print("z");
    } 
  }
   
  Serial.println(" Mode Change Done");
  return;
}

void RFM98W::setLoRaMode()
{
  Serial.println("Setting LoRa Mode");
  this->setMode(RF96_MODE_SLEEP);
  this->writeRegister(REG_OPMODE,0x80);
   
  // frequency  
  this->setMode(RF96_MODE_SLEEP);
  /*
  writeRegister(0x06, 0x6C);
  writeRegister(0x07, 0x9C);
  writeRegister(0x08, 0xCC);
  */
  this->writeRegister(0x06, 0x6C);
  this->writeRegister(0x07, 0x9C);
  this->writeRegister(0x08, 0x8E);
   
  Serial.println("LoRa Mode Set");
  
  Serial.print("Mode = "); Serial.println(this->readRegister(REG_OPMODE));
  
  return;
}

void RFM98W::startReceiving()
{
  this->writeRegister(REG_MODEM_CONFIG, EXPLICIT_MODE | ERROR_CODING_4_8 | BANDWIDTH_20K8);
  this->writeRegister(REG_MODEM_CONFIG2, SPREADING_11 | CRC_ON);
  this->writeRegister(0x26, 0x0C);    // 0000 1 1 00
  Serial.println("Set slow mode");
  
  this->writeRegister(0x26, 0x0C);    // 0000 1 1 00
  this->writeRegister(REG_PAYLOAD_LENGTH, 80);
  this->writeRegister(REG_RX_NB_BYTES, 80);

  this->writeRegister(REG_HOP_PERIOD,0xFF);
  this->writeRegister(REG_FIFO_ADDR_PTR, this->readRegister(REG_FIFO_RX_BASE_AD));   
  
  // Setup Receive Continous Mode
  this->setMode(RF96_MODE_RX_CONTINUOUS); 
}

int RFM98W::receiveMessage(char *message)
{
  int i, Bytes, currentAddr;

  int x = this->readRegister(REG_IRQ_FLAGS);
  // printf("Message status = %02Xh\n", x);
  
  // clear the rxDone flag
  // writeRegister(REG_IRQ_FLAGS, 0x40); 
  this->writeRegister(REG_IRQ_FLAGS, 0xFF); 
   
  // check for payload crc issues (0x20 is the bit we are looking for
  if((x & 0x20) == 0x20)
  {
    // printf("CRC Failure %02Xh!!\n", x);
    // reset the crc flags
    this->writeRegister(REG_IRQ_FLAGS, 0x20); 
  }
  else
  {
    currentAddr = this->readRegister(REG_FIFO_RX_CURRENT_ADDR);
    Bytes = this->readRegister(REG_RX_NB_BYTES);
    // printf ("%d bytes in packet\n", Bytes);

    // printf("RSSI = %d\n", readRegister(REG_RSSI) - 137);
  
    this->writeRegister(REG_FIFO_ADDR_PTR, currentAddr);   
    // now loop over the fifo getting the data
    for(i = 0; i < Bytes; i++)
    {
      message[i] = (unsigned char)this->readRegister(REG_FIFO);
    }
    message[Bytes] = '\0';
  
    // writeRegister(REG_FIFO_ADDR_PTR, 0);  // currentAddr);   
  } 
  
  return Bytes;
}