
Receiver Status
---------------
STATUS,rssi,modem_status\r\n

rssi = Current RSSI, in dBm.
modem_status = Contents of RFM98W ReqModemStat register.
    bits 7-5 = Coding rate of last header receiver
       bit 4 = Modem clear
       bit 3 = Header info valid
       bit 2 = RX on-going
       bit 1 = Signal synchronised
       bit 0 = Signal detected


Data Packet
-----------
PKT,packet_length\r\n
immediately followed by <packet_length> bytes of binary data


Data Packet Info
----------------
PKTINFO,packet_rssi,packet_snr\r\n

packet_rssi = RSSI of last received packet (dBm)
packet_snr = SNR of last received packet (dBm*4 - see page 105 of RFM98W datasheet)
