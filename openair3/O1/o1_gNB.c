
#include "o1.h"

int o1_sockfd = 0;

void *nr_gNB_O1_reporting(void *param)
{
  char buffer[O1_MAXBUFFER_LEN];
  unsigned len;
  o1_sockfd = connect_socket();

  RAN_CONTEXT_t *RC = (RAN_CONTEXT_t *)param;
  while (!oai_exit) {
    sleep(1);
    for (int i = 0; i < RC->nb_nr_macrlc_inst; i++) {
      pthread_mutex_lock(&RC->nrmac[i]->UE_info.mutex);
      UE_iterator(RC->nrmac[i]->UE_info.list, UE)
      {
        O1GNBMeasurementReport gnbmessage = O1_GNBMEASUREMENT_REPORT__INIT;
        NR_UE_sched_ctrl_t *sched_ctrl = &UE->UE_sched_ctrl;
        NR_mac_stats_t *stats = &UE->mac_stats;
        const int avg_rsrp = stats->num_rsrp_meas > 0 ? stats->cumul_rsrp / stats->num_rsrp_meas : 0;
        gnbmessage.avg_rsrp = avg_rsrp;
        gnbmessage.srs_wide_band_snr = stats->srs_wide_band_snr;
        gnbmessage.rnti = UE->rnti;
        gnbmessage.dlsch_bler = sched_ctrl->dl_bler_stats.bler;
        gnbmessage.dlsch_mcs = sched_ctrl->dl_bler_stats.mcs;
        gnbmessage.ulsch_bler = sched_ctrl->ul_bler_stats.bler;
        gnbmessage.ulsch_mcs = sched_ctrl->ul_bler_stats.mcs;
        gnbmessage.cqi = sched_ctrl->CSI_report.cri_ri_li_pmi_cqi_report.wb_cqi_1tb;
        O1MeasurementReport o1message = O1_MEASUREMENT_REPORT__INIT;
        o1message.gnb_msg = &gnbmessage;
        len = o1_measurement_report__get_packed_size(&o1message);
        if (len >= O1_MAXBUFFER_LEN) {
          LOG_E(NR_MAC, "Message too long\n");
          continue;
        }
        o1_measurement_report__pack(&o1message, buffer);
        if (write(o1_sockfd, buffer, len) < 0) {
          LOG_E(NR_MAC, "Write failed\n");
        }
      }
      pthread_mutex_unlock(&RC->nrmac[i]->UE_info.mutex);
    }
  }
  close(o1_sockfd);
  return;
}

void o1_ulsch_failure_reporting(rnti_t rnti, int fail)
{
  O1GNBMeasurementReport gnbmessage = O1_GNBMEASUREMENT_REPORT__INIT;
  O1ULSCHFailure lfmessage = O1_ULSCHFAILURE__INIT;
  lfmessage.rnti = rnti;
  lfmessage.failure = fail;
  O1MeasurementReport o1message = O1_MEASUREMENT_REPORT__INIT;
  o1message.ulf = &lfmessage;
  char buffer[O1_MAXBUFFER_LEN];

  unsigned len = o1_measurement_report__get_packed_size(&o1message);
  if (len >= O1_MAXBUFFER_LEN) {
    LOG_E(NR_MAC, "Message too long\n");
    return;
  }
  o1_measurement_report__pack(&o1message, buffer);
  if (write(o1_sockfd, buffer, len) < 0) {
    LOG_E(NR_MAC, "Write failed\n");
    return;
  }
}

o1_rrc_failure_reporting(rnti_t rnti, int fail)
{
  O1GNBMeasurementReport gnbmessage = O1_GNBMEASUREMENT_REPORT__INIT;
  O1RRCFailure rrcfmessage = O1_RRCFAILURE__INIT;
  rrcfmessage.rnti = rnti;
  rrcfmessage.failure = fail;
  O1MeasurementReport o1message = O1_MEASUREMENT_REPORT__INIT;
  o1message.rrcf = &rrcfmessage;
  unsigned len = o1_measurement_report__get_packed_size(&o1message);
  char buffer[O1_MAXBUFFER_LEN];

  if (len >= O1_MAXBUFFER_LEN) {
    LOG_E(NR_MAC, "Message too long\n");
    return;
  }
  o1_measurement_report__pack(&o1message, buffer);
  if (write(o1_sockfd, buffer, len) < 0) {
    LOG_E(NR_MAC, "Write failed\n");
    return;
  }
}