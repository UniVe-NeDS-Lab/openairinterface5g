from ue_O1_measurements_pb2 import O1MeasurementsReport

import socket
import json
import os
import random
import requests
import time

HOST = "127.0.0.1"  # Standard loopback interface address (localhost)
PORT = 5000  # Port to listen on (non-privileged ports are > 1023)


MR_PATH = "/events/unauthenticated.IAB_MEASUREMENTS"
FAULT_ID = "28"

cqiMessage = {
    "event": {
        "commonEventHeader": {
            "domain": "measurements",
            "eventId": "nt:network-topology/nt:topology/nt:node/nt:node-id",
            "eventName": "IAB_Measurement_Report",
            "eventType": "MeasurementsReport",
            "sequence": 0,
            "priority": "Normal",
            "reportingEntityId": "SDNR",
            "reportingEntityName": "@controllerName@",
            "sourceId": "",
            "sourceName": "O-RU-ID",
            "startEpochMicrosec": "@timestamp@",
            "lastEpochMicrosec": "@timestamp@",
            "nfNamingCode": "",
            "nfVendorName": "ietf-hardware (RFC8348) /hardware/component[not(parent)][1]/mfg-name",
            "timeZoneOffset": "+00:00",
            "version": "4.1",
            "vesEventListenerVersion": "7.2.1"
        },
        "measField": {
            'cell': '',
            'rssi': 0,
            'snr': 0,
            'rsrq': 0
        }
    }
}


class CQISimulator():
    def __init__(self, mr_url):
        self.mr_url = mr_url

    def report_cqi(self, o1_msg: O1MeasurementsReport):
        # o_ru_id = "ERICSSON-O-RU-1122" + str(random_time)
        print(f"Sending O1Msg: {o1_msg}")
        msg_as_json = cqiMessage
        msg_as_json["event"]["commonEventHeader"]["sourceName"] = o1_msg.imsi
        msg_as_json["event"]["measField"]["cell"] = 'test'
        msg_as_json["event"]["measField"]["rx_power_avg"] = o1_msg.rx_power_avg
        msg_as_json["event"]["measField"]["rx_power_tot"] = o1_msg.rx_power_tot
        msg_as_json["event"]["measField"]["n0_power_avg"] = o1_msg.n0_power_avg
        msg_as_json["event"]["measField"]["rx_rssi_dBm"] = o1_msg.rx_rssi_dBm
        msg_as_json["event"]["measField"]["ssb_rsrp_dBm"] = o1_msg.ssb_rsrp_dBm
        msg_as_json["event"]["measField"]["mcs"] = o1_msg.mcs
        #sendPostRequest(self.mr_url, msg_as_json)


def sendPostRequest(url, msg):
    try:
        requests.post(url, json=msg, headers={'Content-Type': 'application/json'})
    except Exception as e:
        print(type(e))
        print(e.args)
        print(e)


mr_host = os.getenv("MR-HOST", "http://10.75.11.18")
print("Using MR Host from os: " + mr_host)
mr_port = os.getenv("MR-PORT", "3904")
print("Using MR Port from os: " + mr_port)
mr_url = mr_host + ":" + mr_port + MR_PATH
CSim = CQISimulator(mr_url)
ue_meas = O1MeasurementsReport()

if os.path.exists("/run/openair_o1"):
    os.remove("/run/openair_o1")

with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
    s.bind("/run/openair_o1")
    s.listen()
    while True:
        conn, addr = s.accept()
        with conn:
            print(f"Connected by {addr}")
            while True:
                data = conn.recv(1024)
                if not data:
                    break
                ue_meas.ParseFromString(data)
                CSim.report_cqi(ue_meas)
