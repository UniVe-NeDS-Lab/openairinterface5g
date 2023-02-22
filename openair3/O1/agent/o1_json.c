/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#include "o1_json.h"
#include <time.h>

uint64_t get_now_time()
{
  struct timespec spec;
  if (clock_gettime(1, &spec) == -1) { /* 1 is CLOCK_MONOTONIC */
    abort();
  }

  return spec.tv_sec * 1000 + spec.tv_nsec / 1e6;
}

int o1_seqn = 0;

int init_curl()
{
}

int o1_send_json(char *url, json_object *jo)
{
  CURL *curl;
  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();
  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "Accept: application/json");
  headers = curl_slist_append(headers, "Content-Type: application/json");
  headers = curl_slist_append(headers, "charset: utf-8");
  char *credentials = 0;
  asprintf(&credentials, "%s:%s", "sample1", "sample1");
  curl_easy_setopt(curl, CURLOPT_USERPWD, credentials);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 1L);
  curl_easy_setopt(curl, CURLOPT_FRESH_CONNECT, 1L);
  curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

  char *enc_jo = json_object_to_json_string_ext(jo, JSON_C_TO_STRING_PLAIN);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, enc_jo);

  CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    // LOG_E(O1, "Failed POST: %s\n", curl_easy_strerror(res));
    printf("[O1] Failed POST: %s\n", curl_easy_strerror(res));
    curl_easy_cleanup(curl);
    return -1;
  }
  curl_easy_cleanup(curl);
  curl_global_cleanup();
  curl = NULL;
  return 0;
}

json_object *gen_head(char *domain, char *event_id, char *event_name, char *eventType, char *priority)
{
  json_object *common_head = json_object_new_object();
  json_object_object_add(common_head, "domain", json_object_new_string(domain));
  json_object_object_add(common_head, "eventId", json_object_new_string(event_id));
  json_object_object_add(common_head, "eventName", json_object_new_string(event_name));
  json_object_object_add(common_head, "eventType", json_object_new_string(eventType));
  json_object_object_add(common_head, "sequence", json_object_new_int(++o1_seqn));
  json_object_object_add(common_head, "priority", json_object_new_string(priority));
  json_object_object_add(common_head, "reportingEntityId", json_object_new_string(""));
  json_object_object_add(common_head, "reportingEntityName", json_object_new_string("OAI-gNB"));
  json_object_object_add(common_head, "sourceId", json_object_new_string(""));
  json_object_object_add(common_head, "sourceName", json_object_new_string("OAI-gNB")); //TODO add some id
  json_object_object_add(common_head, "startEpochMicrosec", json_object_new_int64(get_now_time()));
  json_object_object_add(common_head, "lastEpochMicrosec", json_object_new_int64(get_now_time()));
  json_object_object_add(common_head, "nfNamingCode", json_object_new_string("OAI 5G gNB"));
  json_object_object_add(common_head, "nfVendorName", json_object_new_string("OAI"));
  json_object_object_add(common_head, "timeZoneOffset", json_object_new_string(""));
  json_object_object_add(common_head, "version", json_object_new_string("4.0"));
  json_object_object_add(common_head, "vesEventListenerVersion", json_object_new_string("7.2.1"));
  return common_head;
}

json_object *gen_hb()
{
  time_t rawtime;
  time(&rawtime);
  char str_time[20];
  sprintf(str_time, "%d", rawtime); 
  json_object *heartbeat_fields = json_object_new_object();
  json_object_object_add(heartbeat_fields, "heartbeatFieldsVersion", json_object_new_string("3.0"));
  json_object_object_add(heartbeat_fields, "heartbeatInterval", json_object_new_int(20));
  json_object *alarm_additional_information = json_object_new_object();
  json_object_object_add(heartbeat_fields, "additionalFields", alarm_additional_information);
  json_object_object_add(alarm_additional_information, "eventTime", json_object_new_string(str_time));

  json_object *event = json_object_new_object();
  json_object_object_add(event, "heartbeatFields", heartbeat_fields);
  json_object_object_add(event, "commonEventHeader", gen_head("heartbeat", "Controller-6", "Controller", "", "Low"));
  json_object *root = json_object_new_object();
  json_object_object_add(root, "event", event);
  return root;
}

json_object *gen_fm()
{
  json_object *fault_fields = json_object_new_object();
  json_object_object_add(fault_fields, "faultFieldsVersion", json_object_new_string("4.0"));
  json_object_object_add(fault_fields, "alarmCondition", json_object_new_string(""));
  json_object_object_add(fault_fields, "alarmInterfaceA", json_object_new_string(""));
  json_object_object_add(fault_fields, "eventSourceType", json_object_new_string(""));
  json_object_object_add(fault_fields, "specificProblem", json_object_new_string(""));
  json_object_object_add(fault_fields, "eventSeverity", json_object_new_string(""));
  json_object_object_add(fault_fields, "vfStatus", json_object_new_string("Active"));
  json_object_object_add(fault_fields, "version", json_object_new_string(""));
  json_object *alarm_additional_information = json_object_new_object();
  json_object_object_add(fault_fields, "alarmAdditionalInformation", alarm_additional_information);
  json_object_object_add(alarm_additional_information, "eventTime", json_object_new_string(""));
  json_object_object_add(alarm_additional_information, "equipType", json_object_new_string(""));
  json_object_object_add(alarm_additional_information, "vendor", json_object_new_string(""));
  json_object_object_add(alarm_additional_information, "model", json_object_new_string(""));
  json_object *event = json_object_new_object();
  json_object_object_add(event, "faultFields", fault_fields);
  json_object_object_add(event, "commonEventHeader", gen_head("fault", "O_RAN_COMPONENT_Alarms-30", "O_RAN_COMPONENT_Alarms", "", "Low"));
  json_object *root = json_object_new_object();
  json_object_object_add(root, "event", event);
  return root;
}

json_object *gen_pm(struct pm_fields pm_f)
{
  json_object *meas_fields = json_object_new_object();
  json_object_object_add(meas_fields, "rnti", json_object_new_int(pm_f.rnti));
  json_object_object_add(meas_fields, "avg_rsrp", json_object_new_int(pm_f.avg_rsrp));
  json_object_object_add(meas_fields, "srs_wide_band_snr", json_object_new_int(pm_f.srs_wide_band_snr));
  json_object_object_add(meas_fields, "dlsch_mcs", json_object_new_int(pm_f.dlsch_mcs));
  json_object_object_add(meas_fields, "ulsch_mcs", json_object_new_int(pm_f.ulsch_mcs));
  json_object_object_add(meas_fields, "cqi", json_object_new_int(pm_f.cqi));
  json_object_object_add(meas_fields, "dlsch_bler", json_object_new_double(pm_f.dlsch_bler));
  json_object_object_add(meas_fields, "ulsch_bler", json_object_new_double(pm_f.ulsch_bler));
  json_object *additional_information = json_object_new_object();
  json_object_object_add(meas_fields, "additionalFields", additional_information);
  json_object *event = json_object_new_object();
  json_object_object_add(event, "measurementFields", meas_fields);
  json_object_object_add(event, "commonEventHeader", gen_head("measurement", "pm-1", "stndDefined_performanceMeasurementStreaming", "performanceMeasurementStreaming", "Low"));
  json_object *root = json_object_new_object();
  json_object_object_add(root, "event", event);
  return root;
}

json_object *gen_pnf()
{
  json_object *pnf_fields = json_object_new_object();
  json_object_object_add(pnf_fields, "pnfRegistrationFieldsVersion", json_object_new_string("2.0"));
  json_object_object_add(pnf_fields, "lastServiceDate", json_object_new_string("2019-08-16"));
  json_object_object_add(pnf_fields, "macAddress", json_object_new_string("1a:2e:4c:31:ee:f4"));
  json_object_object_add(pnf_fields, "manufactureDate", json_object_new_string("2019-08-16"));
  json_object_object_add(pnf_fields, "modelNumber", json_object_new_string("OAI gNB"));
  json_object_object_add(pnf_fields, "oamV4IpAddress", json_object_new_string("0.0.0.0"));
  json_object_object_add(pnf_fields, "oamV6IpAddress", json_object_new_string("::1"));
  json_object_object_add(pnf_fields, "serialNumber", json_object_new_string("OAI gNB v0"));
  json_object_object_add(pnf_fields, "softwareVersion", json_object_new_string(""));
  json_object_object_add(pnf_fields, "unitFamily", json_object_new_string("OAI"));
  json_object_object_add(pnf_fields, "unitType", json_object_new_string("gNB"));
  json_object_object_add(pnf_fields, "vendorName", json_object_new_string("OAI"));

  json_object *additional_information = json_object_new_object();
  json_object_object_add(additional_information, "oamPort", json_object_new_string("830"));
  json_object_object_add(additional_information, "protocol", json_object_new_string("TLS"));
  json_object_object_add(additional_information, "username", json_object_new_string("netconf"));
  json_object_object_add(additional_information, "keyId", json_object_new_string("melacon_server_key"));
  json_object_object_add(additional_information, "reconnectOnChangedSchema", json_object_new_string("false"));
  json_object_object_add(additional_information, "sleep-factor", json_object_new_string("1.5"));
  json_object_object_add(additional_information, "tcpOnly", json_object_new_string("false"));
  json_object_object_add(additional_information, "connectionTimeout", json_object_new_string("20000"));
  json_object_object_add(additional_information, "maxConnectionAttempts", json_object_new_string("100"));
  json_object_object_add(additional_information, "betweenAttemptsTimeout", json_object_new_string("2000"));
  json_object_object_add(additional_information, "keepaliveDelay", json_object_new_string("120"));
  json_object_object_add(pnf_fields, "additionalFields", additional_information);
  json_object *event = json_object_new_object();
  json_object_object_add(event, "pnfRegistrationFields", pnf_fields);
  json_object_object_add(event, "commonEventHeader", gen_head("pnfRegistration", "EventType5G", "EventType5G", "", "Normal"));
  json_object *root = json_object_new_object();
  json_object_object_add(root, "event", event);
  return root;
}