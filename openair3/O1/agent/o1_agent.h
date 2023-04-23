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

#ifndef O1_AGENT_H
#define O1_AGENT_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include "PHY/defs_nr_UE.h"
#include "openair2/LAYER2/NR_MAC_gNB/mac_proto.h"
#include "intertask_interface.h"
#include "event2/event.h"

typedef struct o1_agent_s {
  char* url;
  char hostname[1025];
  float hb_period;
  float pm_period;
  int initial_sleep;

  atomic_bool agent_stopped;
  struct event_base* ev_base;
} o1_agent_t;

// API
o1_agent_t* o1_init_agent(const char* url, uint16_t initial_sleep, double hb_period, double pm_period);
void o1_free_agent(o1_agent_t* ag);
void o1_start_agent(o1_agent_t* ag);

// Callbacks
void o1_send_pm(int fd, short event, void* arg);
void o1_send_hb(int fd, short event, void* arg);
void o1_handle_itti(int fd, short event, void* arg);

// misc
struct timeval seconds_to_timeval(float time);
void o1_rrc_fail(rnti_t rnti, uint64_t ngap_id);
void o1_ulsch_fail(rnti_t rnti, uint64_t ngap_id);

#endif