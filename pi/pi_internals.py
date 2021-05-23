#!/usr/bin/env python
import datetime
import psutil
from influxdb import InfluxDBClient

# influx configuration - edit these
ifuser = "admin"
ifpass = "MissRhoades420"
ifdb   = "OnlySolarDB_host"
ifhost = "localhost"
ifport = 8086
measurement_name = "system"

# take a timestamp for this measurement
time = datetime.datetime.utcnow()

# collect some stats from psutil
disk = psutil.disk_usage('/')
mem = psutil.virtual_memory()
load = psutil.getloadavg()
conn = psutil.net_if_stats()

# format the data as a single measurement for influx
body = [
    {
        "measurement": measurement_name,
        "time": time,
        "fields": {
            "load_1": load[0],
            "load_5": load[1],
            "load_15": load[2],
            "disk_percent": disk.percent,
            "disk_free": disk.free,
            "disk_used": disk.used,
            "mem_percent": mem.percent,
            "mem_free": mem.free,
            "mem_used": mem.used,
	    "degr_hor": 49.875000,
	    "degr_ver": 112.963600,
	    "volt_sp": 6.510000,
	    "cur_sp" : 3.200000,
	    "watt_sp" : 10.000000,
	    "volt_bat" : 4.250000,
	    "cur_bat" : 3.750000,
	    "watt_bat" : 10.000000,
        }
    }
]

# connect to influx
ifclient = InfluxDBClient(ifhost,ifport,ifuser,ifpass,ifdb)

# write the measurement
ifclient.write_points(body)
