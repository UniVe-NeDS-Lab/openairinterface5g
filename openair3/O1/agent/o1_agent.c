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

#include "o1_agent.h"

extern RAN_CONTEXT_t RC;

o1_agent_t* o1_init_agent(const char* url, uint16_t initial_sleep, double hb_period, double pm_period)
{
  assert(url != NULL);
  // TODO: Check it's valid url (http or https)

  printf("[O1 AGENT]: Initializing ... \n");
  o1_agent_t* ag = calloc(1, sizeof(*ag));
  assert(ag != NULL && "Memory exhausted");
  ag->url = malloc(strlen(url) * sizeof(char));
  ag->initial_sleep = initial_sleep;
  ag->hb_period = hb_period;
  ag->pm_period = pm_period;
  strcpy(ag->url, url);
  return ag;
}

void o1_free_agent(o1_agent_t* ag)
{
  ag->agent_stopped = true;
  //   ag->stop_token = true;
  //   while (ag->agent_stopped == false) {
  //     usleep(1000);
  //   }
  free(ag->url);
  free(ag);
}

void o1_send_hb(int fd, short event, void* arg)
{
  o1_agent_t* ag = (o1_agent_t*)arg;
  printf("O1: Sending HB\n\n");
  o1_send_json(ag->url, gen_hb());
}

void o1_send_pm(int fd, short event, void* arg)
{
  printf("O1: Sending PM\n\n");
  o1_agent_t* ag = (o1_agent_t*)arg;
  for (int i = 0; i < RC.nb_nr_macrlc_inst; i++) {
    struct pm_fields pmf[MAX_MOBILES_PER_GNB + 1];
    memset(&pmf, 0, sizeof(struct pm_fields) * (MAX_MOBILES_PER_GNB + 1));
    int ueIndex = 0;
    UE_iterator(RC.nrmac[i]->UE_info.list, UE)
    {
      pthread_mutex_lock(&RC.nrmac[i]->UE_info.mutex);
      NR_UE_sched_ctrl_t* sched_ctrl = &UE->UE_sched_ctrl;
      NR_mac_stats_t* stats = &UE->mac_stats;
      const int avg_rsrp = stats->num_rsrp_meas > 0 ? stats->cumul_rsrp / stats->num_rsrp_meas : 0;
      pmf[ueIndex].avg_rsrp = avg_rsrp;
      // pmf[i].srs_wide_band_snr = stats->srs_wide_band_snr;
      pmf[ueIndex].rnti = UE->rnti;
      pmf[ueIndex].dlsch_bler = sched_ctrl->dl_bler_stats.bler;
      pmf[ueIndex].dlsch_mcs = sched_ctrl->dl_bler_stats.mcs;
      pmf[ueIndex].ulsch_bler = sched_ctrl->ul_bler_stats.bler;
      pmf[ueIndex].ulsch_mcs = sched_ctrl->ul_bler_stats.mcs;
      pmf[ueIndex].cqi = sched_ctrl->CSI_report.cri_ri_li_pmi_cqi_report.wb_cqi_1tb;
      pthread_mutex_unlock(&RC.nrmac[i]->UE_info.mutex);
      ueIndex += 1;
    }
    for (int ueIndex = 0; ueIndex < MAX_MOBILES_PER_GNB + 1; ueIndex++) {
      if (pmf[ueIndex].rnti) {
        o1_send_json(ag->url, gen_pm(pmf[ueIndex]));
      }
    }
  }
}

struct timeval seconds_to_timeval(float time)
{
  struct timeval tv;
  int seconds = (int)time;
  int millis = (int)((time - seconds) * 1000);
  tv.tv_sec = seconds;
  tv.tv_usec = millis;
  return tv;
}

void o1_start_agent(o1_agent_t* ag)
{
  ag->ev_base = event_base_new();
  struct timeval hb_tv = seconds_to_timeval(ag->hb_period);
  struct timeval pm_tv = seconds_to_timeval(ag->pm_period);
  // Sleep to wait for the initalization of OAI
  sleep(ag->initial_sleep);

  // Send PNF message
  o1_send_json(ag->url, gen_pnf());

  // Set periodic event to send Heartbeats Messages
  struct event* hb_ev;
  hb_ev = event_new(ag->ev_base, -1, EV_PERSIST, o1_send_hb, (void*)ag);
  event_add(hb_ev, &hb_tv);

  // Set periodic event to sent Performance Metrics Messages
  struct event* pm_ev;
  pm_ev = event_new(ag->ev_base, -1, EV_PERSIST, o1_send_pm, (void*)ag);
  event_add(pm_ev, &pm_tv);

  // Fire the event dispatcher
  // event_base_dispatch(ag->ev_base);

  // Handle the ITTI messages to send Failure Messages, while managing the event loop
  while (!ag->agent_stopped) {
    // tick a loop in the event loop
    event_base_loop(ag->ev_base, EVLOOP_NONBLOCK | EVLOOP_ONCE);
    MessageDef* msg;
    itti_poll_msg(TASK_O1, &msg); // blocking
    // itti receive released, so we have incoming data
    // Can be a itti message
    if (msg != NULL) {
      switch (ITTI_MSG_ID(msg)) {
        case O1_RLC_FAIL:
          printf("O1: Received O1_RLC_FAIL msg \n\n");
          O1RlcFailMessage m = O1_FAILMSG(msg);
          o1_send_json(ag->url, gen_fm());
          break;
        default:
          printf("O1: Received unhandled msg \n\n");
      }
      itti_free(TASK_O1, msg);
    }
  }
  return;
}

void o1_rrc_fail(rnti_t rnti, uint64_t ngap_id)
{
  MessageDef* message_p;
  message_p = itti_alloc_new_message(TASK_O1, 0, O1_RLC_FAIL);
  O1_FAILMSG(message_p).rntiP = rnti;
  O1_FAILMSG(message_p).ngap_id = ngap_id;
  // O1_FAILMSG(message_p).imsi = ue_context_p->ue_context.imsi;
  itti_send_msg_to_task(TASK_O1, 0, message_p);
}