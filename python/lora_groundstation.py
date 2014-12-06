#!/usr/bin/env python

import serial,sys,struct, logging
from optparse import OptionParser


parser = OptionParser()
parser.set_defaults(baud=57600)
parser.add_option("-p", "--port", dest="port", help="Serial Port", metavar="PORT")
parser.add_option("-s", "--baud-rate", dest="baud", help="Baud rate (default 57600)")
(options,args) = parser.parse_args()


# Open Serial port
ser = serial.Serial(options.port, int(options.baud), timeout=1)

modem_status = 0
rssi = 0

def print_modem_status(old_status_data, new_status_data):
    # Signal Detected
    signal_detected = new_status_data&1
    if (signal_detected != old_status_data&1 and signal_detected>0):
        print "Signal Detected"

    # Signal Sync
    signal_sync = new_status_data&2
    if (signal_sync != old_status_data&2 and signal_sync>0):
        print "Signal Sync"

    # Header Info Valid
    header_valid = new_status_data&8
    if (header_valid != old_status_data&8 and header_valid>0):
        print "Valid Header Detected"


while(True):
    line = ser.readline()

    #print line

    if line.startswith("STATUS"):
        modem_status_temp = int(line.split(',')[2])
        rssi_temp = int(line.split(',')[1])

        if modem_status_temp != modem_status:
            print_modem_status(modem_status, modem_status_temp)
            modem_status = modem_status_temp

        if rssi_temp != rssi:
            print "Current RSSI: %d" % (rssi_temp)
            rssi = rssi_temp
    elif line.startswith("PKTINFO"):
        packet_rssi = int(line.split(',')[1])
        packet_snr = int(line.split(',')[2])/4
        print "Packet had %d RSSI, %d SNR" % (packet_rssi,packet_snr)
    elif line.startswith("PKT"):
        packet_length = int(line.split(',')[1])
        packet = ser.read(packet_length)
        print "Got Packet: %s" % (packet)


