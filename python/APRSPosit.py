#
# APRS Position Uploader
# Change the aprsUser and aprsPass variables before using!!!
#

from socket import *

serverHost = "rotate.aprs2.net"
serverPort = 14580
aprsUser = "NOCALL"
aprsPass = 0
callsign = aprsUser

def push_balloon_to_aprs(object_name, lat, lon, alt, comment):
	# Pad or limit the sonde ID to 9 characters.
	if len(object_name) > 9:
		object_name = object_name[:9]
	elif len(object_name) < 9:
		object_name = object_name + " "*(9-len(object_name))
	
	# Convert float latitude to APRS format (DDMM.MM)
	lat_degree = abs(int(lat))
	lat_minute = abs(lat - int(lat)) * 60.0
	lat_min_str = ("%02.2f" % lat_minute).zfill(5)
	lat_dir = "S"
	if lat>0.0:
		lat_dir = "N"
	lat_str = "%02d%s" % (lat_degree,lat_min_str) + lat_dir
	
	# Convert float longitude to APRS format (DDDMM.MM)
	lon_degree = abs(int(lon))
	lon_minute = abs(lon - int(lon)) * 60.0
	lon_min_str = ("%02.2f" % lon_minute).zfill(5)
	lon_dir = "E"
	if lon<0.0:
		lon_dir = "W"
	lon_str = "%03d%s" % (lon_degree,lon_min_str) + lon_dir
	
	# Convert Alt (in metres) to feet
	alt = int(alt/0.3048)
	
	# Produce the APRS object string.
	out_str = ";%s*111111z%s/%sO000/000/A=%06d %s" % (object_name,lat_str,lon_str,alt, comment)
	print out_str
	
	# Connect to an APRS-IS server, login, then push our object position in.
	
	# create socket & connect to server
	sSock = socket(AF_INET, SOCK_STREAM)
	sSock.connect((serverHost, serverPort))
	# logon
	sSock.send('user %s pass %s vers VK5QI-Python 0.01\n' % (aprsUser, aprsPass) )
	# send packet
	sSock.send('%s>APRS:%s\n' % (callsign, out_str) )
	
	# close socket
	sSock.shutdown(0)
	sSock.close()
