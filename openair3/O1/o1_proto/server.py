from O1_measurements_pb2 import O1MeasurementReport

import socket
import json
import os
import random
import requests
import time


MR_PATH = "/events/unauthenticated.IAB_MEASUREMENTS"

cqiMessage = {
    "event": {
        "commonEventHeader": {
            "domain": "measurements",
            "eventId": "",
            "sourceName": "O-RU-ID",
            "ts": 0,
        },
        "measField": {},
        "failField": {}
    }
}


class CQISimulator():
    def __init__(self, mr_url):
        self.mr_url = mr_url

    def report_cqi(self, o1_msg: O1MeasurementReport):
        # o_ru_id = "ERICSSON-O-RU-1122" + str(random_time)
        print(f"Sending O1Msg: {o1_msg}")
        msg_as_json = cqiMessage
        msg_as_json["event"]["commonEventHeader"]["sourceName"] = socket.getfqdn()
        msg_as_json["event"]["commonEventHeader"]["ts"] = time.time()
        if o1_msg.HasField('ue_msg'):

            msg_as_json["event"]["commonEventHeader"]["eventId"] = 'IAB.MT.MeasureReport'
            msg_as_json["event"]["measField"]["rnti"] = o1_msg.ue_msg.rnti
            msg_as_json["event"]["measField"]["imsi"] = o1_msg.ue_msg.imsi
            msg_as_json["event"]["measField"]["rx_power_avg"] = o1_msg.ue_msg.rx_power_avg
            msg_as_json["event"]["measField"]["rx_power_tot"] = o1_msg.ue_msg.rx_power_tot
            msg_as_json["event"]["measField"]["n0_power_avg"] = o1_msg.ue_msg.n0_power_avg
            msg_as_json["event"]["measField"]["rx_rssi_dBm"] = o1_msg.ue_msg.rx_rssi_dBm
            msg_as_json["event"]["measField"]["ssb_rsrp_dBm"] = o1_msg.ue_msg.ssb_rsrp_dBm
            msg_as_json["event"]["measField"]["mcs"] = o1_msg.ue_msg.mcs
        elif o1_msg.HasField('gnb_msg'):
            msg_as_json["event"]["commonEventHeader"]["eventId"] = 'IAB.DU.MeasureReport'
            msg_as_json["event"]["measField"]["rnti"] = o1_msg.gnb_msg.rnti
            msg_as_json["event"]["measField"]["avg_rsrp"] = o1_msg.gnb_msg.avg_rsrp
            msg_as_json["event"]["measField"]["srs_wide_band_snr"] = o1_msg.gnb_msg.srs_wide_band_snr
            msg_as_json["event"]["measField"]["dlsch_mcs"] = o1_msg.gnb_msg.dlsch_mcs
            msg_as_json["event"]["measField"]["ulsch_mcs"] = o1_msg.gnb_msg.ulsch_mcs
            msg_as_json["event"]["measField"]["cqi"] = o1_msg.gnb_msg.cqi
            msg_as_json["event"]["measField"]["dlsch_bler"] = o1_msg.gnb_msg.dlsch_bler
            msg_as_json["event"]["measField"]["ulsch_bler"] = o1_msg.gnb_msg.ulsch_bler
        elif o1_msg.HasField('ulf'):
            msg_as_json["event"]["commonEventHeader"]['domain'] = 'failure'
            msg_as_json["event"]["commonEventHeader"]["eventId"] = 'IAB.DU.ULSCH.FailureReport'
            msg_as_json["event"]["measField"]["rnti"] = o1_msg.ulf.rnti
            msg_as_json["event"]["measField"]["failure"] = o1_msg.ulf.failure
        elif o1_msg.HasField('rrcf'):
            msg_as_json["event"]["commonEventHeader"]['domain'] = 'failure'
            msg_as_json["event"]["commonEventHeader"]["eventId"] = 'IAB.DU.RRC.FailureReport'
            msg_as_json["event"]["measField"]["rnti"] = o1_msg.rrcf.rnti
            msg_as_json["event"]["measField"]["failure"] = o1_msg.rrcf.failure
        else:
            print("Unkown message type")
        sendPostRequest(self.mr_url, msg_as_json)


def sendPostRequest(url, msg):
    try:
        requests.post(url, json=msg, headers={'Content-Type': 'application/json'})
    except Exception as e:
        print(type(e))
        print(e.args)
        print(e)


mr_host = os.getenv("MR_HOST", "http://10.75.11.18")
print("Using MR Host from os: " + mr_host)
mr_port = os.getenv("MR_PORT", "3904")
print("Using MR Port from os: " + mr_port)
mr_url = mr_host + ":" + mr_port + MR_PATH
CSim = CQISimulator(mr_url)
ue_meas = O1MeasurementReport()

if os.path.exists("/run/openair_o1"):
    os.remove("/run/openair_o1")
try:
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
except (KeyboardInterrupt, SystemExit):
    print("O1 server received Sigterm, exiting")
