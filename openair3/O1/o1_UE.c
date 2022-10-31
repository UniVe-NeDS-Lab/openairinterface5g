
#include "o1.h"
#include "openair3/UICC/usim_interface.h"

void *nr_ue_O1_reporting(void *param)
{
  uicc_t *uicc = init_uicc("uicc");
  int sockfd = connect_socket();
  char buffer[1024];
  unsigned len;
  O1MeasurementsReport o1message = O1_MEASUREMENTS_REPORT__INIT;
  PHY_VARS_NR_UE *ue = (PHY_VARS_NR_UE *)param;
  while (!oai_exit) {
    sleep(1);
    if (ue->is_synchronized) {
      o1message.n0_power_avg = ue->measurements.n0_power_avg;
      o1message.has_n0_power_avg = 1;
      o1message.n0_power_tot = ue->measurements.n0_power_tot;
      o1message.has_n0_power_tot = 1;
      o1message.mcs = ue->dlsch[0][0][0]->harq_processes[0]->mcs;
      o1message.has_mcs = 1;
      o1message.rx_power_avg = ue->measurements.rx_power_avg[0];
      o1message.has_rx_power_avg = 1;
      o1message.rx_power_tot = ue->measurements.rx_power_tot[0];
      o1message.has_rx_power_tot = 1;
      o1message.ssb_rsrp_dbm = ue->measurements.ssb_rsrp_dBm[0];
      o1message.has_ssb_rsrp_dbm = 1;
      o1message.rx_rssi_dbm = ue->measurements.rx_rssi_dBm[0];
      o1message.has_rx_rssi_dbm = 1;
      o1message.imsi = uicc->imsiStr;
      // ue->sinr_CQI_dB;
      // ue->sinr_dB;
      // ue->sinr_eff

      len = o1_measurements_report__get_packed_size(&o1message);
      if (len >= 1024) {
        LOG_E(NR_MAC, "Message too long\n");
        continue;
      }
      o1_measurements_report__pack(&o1message, buffer);
      if (write(sockfd, buffer, len) < 0) {
        LOG_E(NR_MAC, "Write failed\n");
      }
    }
  }
  close(sockfd);
}
