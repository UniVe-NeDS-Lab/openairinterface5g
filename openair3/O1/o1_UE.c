
#include "openair3/UICC/usim_interface.h"
#include "openair2/LAYER2/NR_MAC_UE/mac_defs.h"
#include "openair2/LAYER2/NR_MAC_UE/mac_proto.h"

// void *nr_ue_O1_reporting(void *param)
// {
//   NR_UE_MAC_INST_t *mac = get_mac_inst(0);
//   uicc_t *uicc = init_uicc("uicc");
//   int sockfd = connect_socket();
//   char buffer[1024];
//   unsigned len;
//   O1UEMeasurementReport uemessage = O1_UEMEASUREMENT_REPORT__INIT;
//   PHY_VARS_NR_UE *ue = (PHY_VARS_NR_UE *)param;
//   while (!oai_exit) {
//     sleep(1);
//     if (ue->is_synchronized) {
//       uemessage.n0_power_avg = ue->measurements.n0_power_avg;
//       // uemessage.has_n0_power_avg = 1;
//       uemessage.n0_power_tot = ue->measurements.n0_power_tot;
//       // uemessage.has_n0_power_tot = 1;
//       uemessage.mcs = ue->dlsch[0][0][0]->harq_processes[0]->mcs;
//       // uemessage.has_mcs = 1;
//       uemessage.rx_power_avg = ue->measurements.rx_power_avg[0];
//       // uemessage.has_rx_power_avg = 1;
//       uemessage.rx_power_tot = ue->measurements.rx_power_tot[0];
//       // uemessage.has_rx_power_tot = 1;
//       uemessage.ssb_rsrp_dbm = ue->measurements.ssb_rsrp_dBm[0];
//       // uemessage.has_ssb_rsrp_dbm = 1;
//       uemessage.rx_rssi_dbm = ue->measurements.rx_rssi_dBm[0];
//       // uemessage.has_rx_rssi_dbm = 1;
//       uemessage.imsi = uicc->imsiStr;
//       uemessage.rnti = mac->crnti;
//       // ue->sinr_CQI_dB;
//       // ue->sinr_dB;
//       // ue->sinr_eff

//       O1MeasurementReport o1message = O1_MEASUREMENT_REPORT__INIT;
//       o1message.ue_msg = &uemessage;
//       len = o1_measurement_report__get_packed_size(&o1message);
//       if (len >= 1024) {
//         LOG_E(NR_MAC, "Message too long\n");
//         continue;
//       }
//       o1_measurement_report__pack(&o1message, buffer);
//       if (write(sockfd, buffer, len) < 0) {
//         LOG_E(NR_MAC, "Write failed\n");
//       }
//     }
//   }
//   close(sockfd);
//   return;
// }
