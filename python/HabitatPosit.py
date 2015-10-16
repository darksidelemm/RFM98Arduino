#
# Habitat upload tool.
#
# Shamelessly copied from https://github.com/rossengeorgiev/hab-tools/blob/master/send_tlm_to_habitat.py
#

import httplib, json, crcmod
from base64 import b64encode
from hashlib import sha256
from datetime import datetime

def upload_sentence(sentence, callsign):
	sentence_b64 = b64encode(sentence)

	date = datetime.utcnow().isoformat("T") + "Z"

	data = {
	    "type": "payload_telemetry",
	    "data": {
	        "_raw": sentence_b64
	        },
	    "receivers": {
	        callsign: {
	            "time_created": date,
	            "time_uploaded": date,
	            },
	        },
	}
	try:
		c = httplib.HTTPConnection("habitat.habhub.org")
		c.request(
		    "PUT",
		    "/habitat/_design/payload_telemetry/_update/add_listener/%s" % sha256(sentence_b64).hexdigest(),
		    json.dumps(data),  # BODY
		    {"Content-Type": "application/json"}  # HEADERS
		    )

		response = c.getresponse()
	except Exception as e:
		print "Failed to upload to Habitat: %s" % (str(e))

def crc16_ccitt(data):
	"""
	Calculate the CRC16 CCITT checksum of *data*.
	
	(CRC16 CCITT: start 0xFFFF, poly 0x1021)
	"""
	crc16 = crcmod.predefined.mkCrcFun('crc-ccitt-false')
	return hex(crc16(data))[2:].upper().zfill(4)

def produce_horuslora(counter,time_str,lat,lon,altitude,speed,sats):
	output = "$$HORUSLORA,%d,%s,%.5f,%.5f,%d,%.1f,%d" % (counter,time_str,lat,lon,altitude,speed,sats)

	checksum = crc16_ccitt(output[2:])
	output = output + "*" + checksum + "\n"
	return output