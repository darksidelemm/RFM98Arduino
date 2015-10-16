/*
  RFM98W.cpp - RFM98W Comms Library
  
  Copyright (C) 2014 Mark Jessop <vk5qi@rfhead.net>
  Original code by David Ackerman.

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
    this->lastMessageFlags = 0x00;

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

uint8_t RFM98W::checkInterrupt(){
    return digitalRead(this->DIO0_PIN);
}

int16_t RFM98W::getRSSIPacket(){
    uint8_t rssi = this->readRegister(REG_RSSI_PACKET);
    return (int16_t)rssi - 137;
}

int32_t RFM98W::getFrequencyError(){
  int32_t Temp;
  
  Temp = (int32_t)readRegister(REG_FREQ_ERROR) & 7;
  Temp <<= 8L;
  Temp += (int32_t)readRegister(REG_FREQ_ERROR+1);
  Temp <<= 8L;
  Temp += (int32_t)readRegister(REG_FREQ_ERROR+2);
  
  if (readRegister(REG_FREQ_ERROR) & 8)
  {
    Temp = Temp - 524288;
  }

  return Temp;
}

void RFM98W::setMode(byte newMode)
{
  if(newMode == currentMode)
    return;  
  
  switch (newMode) 
  {
    case RF96_MODE_TX:
      Serial.println("Changing to Transmit Mode");
      this->writeRegister(REG_LNA, LNA_OFF_GAIN);  // TURN LNA OFF FOR TRANSMITT
      this->writeRegister(REG_PA_CONFIG, PA_MAX_UK);
      this->writeRegister(REG_OPMODE, newMode);
      this->currentMode = newMode; 
      break;
    case RF96_MODE_RX_CONTINUOUS:
      this->writeRegister(REG_PA_CONFIG, PA_OFF_BOOST);  // TURN PA OFF FOR RECIEVE??
      this->writeRegister(REG_LNA, LNA_MAX_GAIN);  // LNA_MAX_GAIN);  // MAX GAIN FOR RECIEVE
      this->writeRegister(REG_OPMODE, newMode);
      this->currentMode = newMode; 
      Serial.println("Changing to Receive Continuous Mode\n");
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
/*
void RFM98W::setFrequency(){
 // this->writeRegister(0x06, 0x6C);
 // this->writeRegister(0x07, 0x9C);
 // this->writeRegister(0x08, 0xCC);

  // 431.650MHz
  this->writeRegister(0x06, 0x6B);
  this->writeRegister(0x07, 0xE9);
  this->writeRegister(0x08, 0x99);
}
*/
void RFM98W::setFrequency(double Frequency)
{
  unsigned long FrequencyValue;

  Frequency = Frequency * 7110656 / 434000000;
  FrequencyValue = (unsigned long)(Frequency);

  writeRegister(0x06, (FrequencyValue >> 16) & 0xFF);   // Set frequency
  writeRegister(0x07, (FrequencyValue >> 8) & 0xFF);
  writeRegister(0x08, FrequencyValue & 0xFF);

}

void RFM98W::setLoRaMode(double Frequency)
{
  Serial.println("Setting LoRa Mode");
  this->setMode(RF96_MODE_SLEEP);
  this->writeRegister(REG_OPMODE,0x80);
   
  // frequency  
  //this->setMode(RF96_MODE_SLEEP);
  /*
  writeRegister(0x06, 0x6C);
  writeRegister(0x07, 0x9C);
  writeRegister(0x08, 0xCC);
  */
  this->setFrequency(Frequency);
   
  Serial.println("LoRa Mode Set");
  
  Serial.print("Mode = "); Serial.println(this->readRegister(REG_OPMODE));
  
  return;
}

void RFM98W::startReceiving()
{
  this->writeRegister(REG_MODEM_CONFIG, EXPLICIT_MODE | ERROR_CODING_4_8 | BANDWIDTH_125K);
  this->writeRegister(REG_MODEM_CONFIG2, SPREADING_10 | CRC_ON);
  this->writeRegister(0x26, 0x0C);    // 0000 1 1 00
  Serial.println("Set slow mode");
  
  this->writeRegister(REG_DETECT_OPT,0x03);
  this->writeRegister(REG_DETECTION_THRESHOLD,0x0A);

  this->writeRegister(REG_PAYLOAD_LENGTH, PAYLOAD_LENGTH);
  this->writeRegister(REG_RX_NB_BYTES, PAYLOAD_LENGTH);


  this->writeRegister(REG_HOP_PERIOD,0xFF);
  this->writeRegister(REG_FIFO_ADDR_PTR, this->readRegister(REG_FIFO_RX_BASE_AD));   
  
  // Setup Receive Continous Mode
  this->setMode(RF96_MODE_RX_CONTINUOUS); 
}

void RFM98W::setupTX(){
  this->writeRegister(REG_MODEM_CONFIG, EXPLICIT_MODE | ERROR_CODING_4_8 | BANDWIDTH_125K);

  this->writeRegister(REG_MODEM_CONFIG2, SPREADING_10 | CRC_ON);
  
  this->writeRegister(0x26, 0x0C);    // 0000 1 1 00
  this->writeRegister(REG_PAYLOAD_LENGTH,PAYLOAD_LENGTH);
  this->writeRegister(REG_RX_NB_BYTES,PAYLOAD_LENGTH);
  
  // Change the DIO mapping to 01 so we can listen for TxDone on the interrupt
  this->writeRegister(REG_DIO_MAPPING_1,0x40);
  this->writeRegister(REG_DIO_MAPPING_2,0x00);
  
  // Go to standby mode
  this->setMode(RF96_MODE_STANDBY);
}

int RFM98W::receiveMessage(char *message)
{
  int i, Bytes, currentAddr;

  int x = this->readRegister(REG_IRQ_FLAGS);
  this->lastMessageFlags = x;
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

uint8_t RFM98W::getLastMessageFlags(){
    return this->lastMessageFlags;
}

void RFM98W::sendData(char *buffer, int len)
{
  int Length;
  if(len==0){
    Length = strlen(buffer);
  }
  
  //Serial.print("Sending "); Serial.print(Length);Serial.println(" bytes");
  //Serial.println(buffer);
  
  this->setMode(RF96_MODE_STANDBY);

  this->writeRegister(REG_FIFO_TX_BASE_AD, 0x00);  // Update the address ptr to the current tx base address
  this->writeRegister(REG_FIFO_ADDR_PTR, 0x00); 
  
  digitalWrite(this->SS_PIN, LOW);
  // tell SPI which address you want to write to
  SPI.transfer(REG_FIFO | 0x80);
  
  // loop over the payload and put it on the buffer 
  for (int i = 0; i < PAYLOAD_LENGTH; i++)
  {
    if (i < Length)
    {
      SPI.transfer(buffer[i] & 0x7F);
      Serial.write(buffer[i]);
    }
    else
    {
      SPI.transfer(0);
    }
  }
  digitalWrite(this->SS_PIN, HIGH);

  
  // go into transmit mode
  this->setMode(RF96_MODE_TX);
}
