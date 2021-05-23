#!/usr/bin/env python
import datetime
import serial
from influxdb import InfluxDBClient

# influx configuration - edit these
ifuser = "admin"
ifpass = "MissRhoades420"
ifdb   = "OnlySolarDB_host"
ifhost = "localhost"
ifport = 8086
measurement_name = "solar_panel"

# take a timestamp for this measurement
time = datetime.datetime.utcnow()
port = serial.Serial("/dev/ttyACM0", baudrate=9600, timeout=3.0)

# collect some stats from the nucleo
received = port.readline()
while received == b"":
	received = port.readline()

# Dividing the stats into different variables
all_readings = received[:-2].decode('UTF-8').split("|")

# format the data as a single measurement for influx
body = [
    {
        "measurement": measurement_name,
        "time": time,
        "fields": {
            "degr_hor": float(all_readings[0]),
	    "degr_ver" : float(all_readings[1]),
	    "volt_sp" : float(all_readings[2]),
	    "cur_sp" : float(all_readings[3]),
	    "watt_sp" : float(all_readings[4]),
	    "volt_bat" : float(all_readings[5]),
	    "cur_bat" : float(all_readings[6]),
	    "watt_bat" : float(all_readings[7]),
            }
    }
]

# connect to influx
ifclient = InfluxDBClient(ifhost,ifport,ifuser,ifpass,ifdb)

# write the measurement
ifclient.write_points(body)
