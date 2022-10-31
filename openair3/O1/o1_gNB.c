
#include "o1.h"

void *nr_gNB_O1_reporting(void *param)
{
  int sockfd = connect_socket();

  char buffer[1024];
  unsigned len;
  O1MeasurementsReport o1message = O1_MEASUREMENTS_REPORT__INIT;
  RAN_CONTEXT_t *RC = (RAN_CONTEXT_t *)param;
  while (!oai_exit) {
    sleep(1);
    for (int i = 0; i < RC->nb_nr_macrlc_inst; i++) {
      pthread_mutex_lock(&RC->nrmac[i]->UE_info.mutex);
      UE_iterator(RC->nrmac[i]->UE_info.list, UE)
      {
        NR_UE_sched_ctrl_t *sched_ctrl = &UE->UE_sched_ctrl;
        NR_mac_stats_t *stats = &UE->mac_stats;
        const int avg_rsrp = stats->num_rsrp_meas > 0 ? stats->cumul_rsrp / stats->num_rsrp_meas : 0;

        LOG_E(NR_MAC,
              "UE RNTI %04x PH %d dB PCMAX %d dBm, average RSRP %d (%d meas), UL-SNR %d dB\n",
              UE->rnti,
              sched_ctrl->ph,
              sched_ctrl->pcmax,
              avg_rsrp,
              stats->num_rsrp_meas,
              stats->srs_wide_band_snr);
      }
      pthread_mutex_unlock(&RC->nrmac[i]->UE_info.mutex);
    }
  }
  close(sockfd);
}