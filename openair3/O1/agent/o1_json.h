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

#ifndef O1_json_h
#define O1_json_h

#include <json-c/json.h>
#include <curl/curl.h>
#include <string.h>
#include "o1_agent.h"

struct pm_fields {
  int rnti;
  uint64_t ngap_id;
  int avg_rsrp;
  int srs_wide_band_snr;
  int dlsch_mcs;
  int ulsch_mcs;
  int cqi;
  float dlsch_bler;
  float ulsch_bler;
};

json_object *gen_head(char *domain, char *event_id, char *event_name, char *eventType, char *priority);
json_object *gen_hb();
json_object *my_gen_hb(o1_agent_t *ag);
json_object *gen_fm();
json_object *gen_pm();
json_object *my_gen_pm(o1_agent_t *ag, struct pm_fields pm_f);
json_object *my_gen_rlc_fail(o1_agent_t *ag, O1RlcFailMessage m);
json_object *my_gen_ulsch_fail(o1_agent_t *ag, O1ulschFailMessage m);
json_object *gen_pnf();
int o1_send_json(char *url, json_object *jo);

#endif