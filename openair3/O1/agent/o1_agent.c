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

o1_agent_t* o1_init_agent(const char* url, uint16_t report_interval)
{
  assert(url != NULL);
  // TODO: Check it's valid url (http or https)

  printf("[O1 AGENT]: Initializing ... \n");
  o1_agent_t* ag = calloc(1, sizeof(*ag));
  assert(ag != NULL && "Memory exhausted");
  ag->url = malloc(strlen(url) * sizeof(char));
  strcpy(ag->url, url);
  ag->report_interval = report_interval;
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

void o1_start_agent(o1_agent_t* ag)
{
  //init_curl();
  while (!ag->agent_stopped) {
    printf("O1 Reporting running, sleep %d\n", ag->report_interval);
    sleep(ag->report_interval);
    o1_send_json(ag->url, gen_hb());
    for (int i = 0; i < RC.nb_nr_macrlc_inst; i++) {
      pthread_mutex_lock(&RC.nrmac[i]->UE_info.mutex);
      struct pm_fields pmf[MAX_MOBILES_PER_GNB + 1];
      UE_iterator(RC.nrmac[i]->UE_info.list, UE)
      {
        NR_UE_sched_ctrl_t* sched_ctrl = &UE->UE_sched_ctrl;
        NR_mac_stats_t* stats = &UE->mac_stats;
        const int avg_rsrp = stats->num_rsrp_meas > 0 ? stats->cumul_rsrp / stats->num_rsrp_meas : 0;
        pmf[i].avg_rsrp = avg_rsrp;
        // pmf[i].srs_wide_band_snr = stats->srs_wide_band_snr;
        pmf[i].rnti = UE->rnti;
        pmf[i].dlsch_bler = sched_ctrl->dl_bler_stats.bler;
        pmf[i].dlsch_mcs = sched_ctrl->dl_bler_stats.mcs;
        pmf[i].ulsch_bler = sched_ctrl->ul_bler_stats.bler;
        pmf[i].ulsch_mcs = sched_ctrl->ul_bler_stats.mcs;
        pmf[i].cqi = sched_ctrl->CSI_report.cri_ri_li_pmi_cqi_report.wb_cqi_1tb;
      }
      pthread_mutex_unlock(&RC.nrmac[i]->UE_info.mutex);
      for (int i = 0; i < MAX_MOBILES_PER_GNB + 1; i++) {
        if (pmf[i].rnti) {
          o1_send_json(ag->url, gen_pm(pmf[i]));
        }
      }
    }
    MessageDef* msg_p;
	itti_poll_msg(TASK_O1, msg_p);
	
  }
  return;
}