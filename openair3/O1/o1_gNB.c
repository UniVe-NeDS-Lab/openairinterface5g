
// #include "o1.h"
// #include "o1_json.h"

// extern RAN_CONTEXT_t RC;

// void *nr_gNB_O1_reporting(o1_agent_args_t *args)
// {
//   char *url = malloc(strlen(args->url)*sizeof(char));
//   uint16_t interval = args->report_interval;
//   strcpy(url, args->url);
//   init_curl();
//   while (!oai_exit) {
//     printf("O1 Reporting running, sleep %d\n", interval);
//     sleep(interval);
//     o1_send_json(url, gen_hb());
//     for (int i = 0; i < RC.nb_nr_macrlc_inst; i++) {
//       pthread_mutex_lock(&RC.nrmac[i]->UE_info.mutex);
//       struct pm_fields pmf[MAX_MOBILES_PER_GNB + 1];
//       UE_iterator(RC.nrmac[i]->UE_info.list, UE)
//       {
//         NR_UE_sched_ctrl_t *sched_ctrl = &UE->UE_sched_ctrl;
//         NR_mac_stats_t *stats = &UE->mac_stats;
//         const int avg_rsrp = stats->num_rsrp_meas > 0 ? stats->cumul_rsrp / stats->num_rsrp_meas : 0;
//         pmf[i].avg_rsrp = avg_rsrp;
//         // pmf[i].srs_wide_band_snr = stats->srs_wide_band_snr;
//         pmf[i].rnti = UE->rnti;
//         pmf[i].dlsch_bler = sched_ctrl->dl_bler_stats.bler;
//         pmf[i].dlsch_mcs = sched_ctrl->dl_bler_stats.mcs;
//         pmf[i].ulsch_bler = sched_ctrl->ul_bler_stats.bler;
//         pmf[i].ulsch_mcs = sched_ctrl->ul_bler_stats.mcs;
//         pmf[i].cqi = sched_ctrl->CSI_report.cri_ri_li_pmi_cqi_report.wb_cqi_1tb;
//       }
//       pthread_mutex_unlock(&RC.nrmac[i]->UE_info.mutex);
//       for (int i = 0; i < MAX_MOBILES_PER_GNB + 1; i++) {
//         if (pmf[i].rnti) {
//           o1_send_json(url, gen_pm(pmf[i]));
//         }
//       }
//     }
//   }
//   return;
// }

// // void o1_ulsch_failure_reporting(rnti_t rnti, int fail)
// // {
// //   O1GNBMeasurementReport gnbmessage = O1_GNBMEASUREMENT_REPORT__INIT;
// //   O1ULSCHFailure lfmessage = O1_ULSCHFAILURE__INIT;
// //   lfmessage.rnti = rnti;
// //   lfmessage.failure = fail;
// //   O1MeasurementReport o1message = O1_MEASUREMENT_REPORT__INIT;
// //   o1message.ulf = &lfmessage;
// //   char buffer[O1_MAXBUFFER_LEN];

// //   unsigned len = o1_measurement_report__get_packed_size(&o1message);
// //   if (len >= O1_MAXBUFFER_LEN) {
// //     LOG_E(NR_MAC, "Message too long\n");
// //     return;
// //   }
// //   o1_measurement_report__pack(&o1message, buffer);
// //   if (write(o1_sockfd, buffer, len) < 0) {
// //     LOG_E(NR_MAC, "Write failed\n");
// //     return;
// //   }
// // }

// // o1_rrc_failure_reporting(rnti_t rnti, int fail)
// // {
// //   O1GNBMeasurementReport gnbmessage = O1_GNBMEASUREMENT_REPORT__INIT;
// //   O1RRCFailure rrcfmessage = O1_RRCFAILURE__INIT;
// //   rrcfmessage.rnti = rnti;
// //   rrcfmessage.failure = fail;
// //   O1MeasurementReport o1message = O1_MEASUREMENT_REPORT__INIT;
// //   o1message.rrcf = &rrcfmessage;
// //   unsigned len = o1_measurement_report__get_packed_size(&o1message);
// //   char buffer[O1_MAXBUFFER_LEN];
// //   if (len >= O1_MAXBUFFER_LEN) {
// //     LOG_E(NR_MAC, "Message too long\n");
// //     return;
// //   }
// //   o1_measurement_report__pack(&o1message, buffer);
// //   if (write(o1_sockfd, buffer, len) < 0) {
// //     LOG_E(NR_MAC, "Write failed\n");
// //     return;
// //   }
// // }
