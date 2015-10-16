#!/usr/bin/env python
#
# LoRa Groundstation Parser + Uploader
# Copyright (C) 2015 Mark Jessop <vk5qi@rfhead.net>
# 
# This code is intended to be paired with an Arduino + LoRa shield running the lora_groundstation code.
# It will decode the binary packet data from the arduino+LoRa and send to:
#     - Habitat - For plotting on http://tracker.habhub.org/
#     - APRS-IS
#     - UDP Socket - For transfer of data to other applications. 
#     - [TODO] MultiCast UDP - Same, but for distribution througout a chase-car network.
#
# NOTE: CHANGE THINGS LIKE PAYLOAD NAME, APRS CALLSIGNS, ETC BEFORE USING THIS CODE!
# 
# With thanks to daveake and rossengeorgiev
#
import serial,sys,struct,logging,time,datetime
from optparse import OptionParser
from APRSPosit import *
from HabitatPosit import *
from socket import *


parser = OptionParser()
parser.set_defaults(baud=57600, rate=30)
parser.add_option("-p", "--port", dest="port", help="Serial Port", metavar="PORT")
parser.add_option("-s", "--baud-rate", dest="baud", help="Baud rate (default 57600)")
parser.add_option("--aprs", action="store_true", dest="aprson", help="Enable APRS-IS Uploading", default=False)
parser.add_option("--habitat", action="store_true", dest="habitat", help="Enable Habitat Uploading", default=False)
parser.add_option("--habitat-listener", dest="listener_callsign", help="Habitat Listener Callsign", default="LORA_RX")
parser.add_option("-r", "--update-rate", dest="rate", type="int", help="Update Rate (default 30 seconds)")
parser.add_option("--udp", action="store_true", dest="udpon", help="Enable UDP Uploading", default=False)
parser.add_option("--udp-host", dest="udp_host", help="UDP Hostname", default="127.0.0.1")
parser.add_option("--udp-port", dest="udp_port", help="UDP Port", default=8942)


(options,args) = parser.parse_args()

lastuploadtime = time.time()

last_rssi = 0
last_snr = 0
last_freqerror = 0

binarypacketformat = "<BHHffHBBBB"
binarypacketlength = 19

binarypacket_base = "<<BHHffH"

rssi = 0

outfilename = datetime.utcnow().strftime("%Y%m%d-%H%M.csv")
outfile = open(outfilename,"wb")


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

def decode_binary_packet(data):
    try:
        unpacked = struct.unpack(binarypacketformat, data)
    except:
        print "Wrong string length. Packet contents:"
        print ":".join("{:02x}".format(ord(c)) for c in data)
        return

    payload_id = unpacked[0]
    counter = unpacked[1]
    time_biseconds = unpacked[2]
    latitude = unpacked[3]
    longitude = unpacked[4]
    altitude = unpacked[5]
    speed = unpacked[6]
    batt_voltage = unpacked[7]
    sats = unpacked[8]
    temp = unpacked[9]


    time_string = time.strftime("%H:%M:%S", time.gmtime(time_biseconds*2))

    batt_voltage_float = 0.5 + 1.5*batt_voltage/255.0

    #print "Decoded Packet: %s  %f,%f %d %.2f %d" % (time_string, latitude, longitude, altitude, speed*1.852, sats)

    print "\n\nDecoded Packet: %s" % (":".join("{:02x}".format(ord(c)) for c in data))
    print "      ID: %d" % payload_id
    print " Seq No.: %d" % counter
    print "    Time: %s" % time_string
    print "     Lat: %.5f" % latitude
    print "     Lon: %.5f" % longitude
    print "     Alt: %d m" % altitude
    print "   Speed: %.1f kph" % (speed*1.852)
    print "    Sats: %d" % sats
    print "    Batt: %.3f" % batt_voltage_float
    print "    Temp: %d" % temp
    print "    RSSI: %d dBm" % last_rssi
    print "     SNR: %.1f dB" % last_snr
    print " FreqErr: %d Hz" % last_freqerror
    print " "

    # Write a line to the output file
    out_line = "%d,%s,%.5f,%.5f,%d,%.1f,%d,%.3f,%d,%d,%.1f,%d\n" % (counter,time_string,latitude,longitude,altitude,speed*1.852,sats,batt_voltage_float,temp, last_rssi, last_snr, last_freqerror)
    outfile.write(out_line)

    global lastuploadtime
    timeelapsed = time.time() - lastuploadtime
    print "Time since last upload: %d" % timeelapsed

    if timeelapsed>options.rate and sats>=4:
        lastuploadtime = time.time()
        if(options.aprson):
            commentfield = "Horus LoRa Sats: %d Speed %.1f" % (sats, speed*1.852)
            try:
                push_balloon_to_aprs("VK5QI-11", latitude, longitude, altitude, commentfield)
                print "=================== UPLOADED TO APRS ========================"
            except:
                print "APRS Upload Failed!"

        habitat_sentence = produce_horuslora(counter,time_string,latitude,longitude,altitude,speed*1.852,sats)
        print habitat_sentence
        if(options.habitat):
            upload_sentence(habitat_sentence,options.listener_callsign)
            print "=================== UPLOADED TO HABITAT ========================"

        if(options.udpon):
            try:
                sock = socket(AF_INET,SOCK_DGRAM)
                sock.sendto(habitat_sentence,(options.udp_host,options.udp_port))
                sock.close()
            except Exception as uhoh:
                print uhoh

# Decode the data in the RegIRQFlags Register, which contains the packet CRC.
def checkPacketCRC(packet_flags):
	if packet_flags&0x20 != 0:
		return False # Failed CRC.
	else:
		return True

def main():
	global last_snr,last_rssi,last_freqerror,last_flags
	# Open Serial port
	ser = serial.Serial(options.port, int(options.baud), timeout=1)

	modem_status = 0

	while True:
	    line = ser.readline()

	    #print line

	    if line.startswith("STATUS"):
	        modem_status_temp = int(line.split(',')[2])
	        rssi_temp = int(line.split(',')[1])

	        if modem_status_temp != modem_status:
	            print_modem_status(modem_status, modem_status_temp)
	            modem_status = modem_status_temp

	        print "Current RSSI: %d" % (rssi_temp)

	    elif line.startswith("PKTINFO"):
	        packet_rssi = int(line.split(',')[1])
	        packet_snr = int(line.split(',')[2])/4.0
	        packet_freqerror = float(line.split(',')[3])
	        packet_freqerror = -1*int((packet_freqerror * 2**24.0 / 32e6)*(125e6/500e6))
	        packet_flags = int(line.split(',')[4])
	        #print "Packet has %d RSSI, %d SNR" % (packet_rssi,packet_snr)
	        last_snr = packet_snr
	        last_rssi = packet_rssi
	        last_freqerror = packet_freqerror
	        last_flags = packet_flags

	    elif line.startswith("PKT"):
	        packet_length = int(line.split(',')[1])
	        packet = ser.read(packet_length)
	        #print "Got Packet: %s Length: %d" % (packet, packet_length)
	        #print ":".join("{:02x}".format(ord(c)) for c in packet)
	        print "Got Packet!"
	        if checkPacketCRC(last_flags):
	        	decode_binary_packet(packet)
	        else:
	        	print "Packet Failed CRC! SNR: %.1f dB, RSSI: %d dB" % (last_snr,last_rssi)

while True:
	try:
		main()
	except Exception as e:
		print "Error: %s" % e # This is mainly to catch any serial comms issues i.e. if the usb-serial adaptor is knocked loose.
		time.sleep(2) # Wait a bit before trying again.