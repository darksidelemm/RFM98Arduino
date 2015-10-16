RFM98Arduino
============

This is an attempt to produce a Arduino library to control a HopeRF RFM98W module.



The 'lora_groundstation' Arduino sketch connects to the LORA module and outputs the following data strings, which the 'lora_groundation.py' python script will interpret, display, and (optionally) upload to habitat.

Receiver Status
---------------
STATUS,rssi,modem_status\r\n

* rssi = Current RSSI, in dBm.
* modem_status = Contents of RFM98W ReqModemStat register.
** bits 7-5 = Coding rate of last header receiver
** bit 4 = Modem clear
** bit 3 = Header info valid
** bit 2 = RX on-going
** bit 1 = Signal synchronised
** bit 0 = Signal detected


Data Packet
-----------
PKT,packet_length\r\n

* Immediately followed by packet_length bytes of binary data


Data Packet Info
----------------
PKTINFO,packet_rssi,packet_snr,pkt_freqerror,irq_flags\r\n

* packet_rssi = RSSI of last received packet (dBm)
* packet_snr = SNR of last received packet (dBm*4 - see page 105 of RFM98W datasheet)
* pkt_freqerror = Contents of RFM98W's RegFeiMSB/Mid/LSB registers.
* irq_flags = Contents of RFM98W's RegIrqFlags Register.

