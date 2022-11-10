
#ifndef O1_h
#define O1_h

#define O1_SOCKET_PATH "/run/openair_o1"
#define O1_MAXBUFFER_LEN 1024
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "o1_proto/O1_measurements.pb-c.h"
#include "PHY/defs_nr_UE.h"
#include "openair2/LAYER2/NR_MAC_gNB/mac_proto.h"
#include "openair2/RRC/NR_UE/rrc_vars.h"
#include "openair2/LAYER2/NR_MAC_UE/mac_defs.h"

void *nr_ue_O1_reporting(void *param);
void *nr_gNB_O1_reporting(void *param);
void o1_ulsch_failure_reporting(rnti_t rnti, int fail);
void o1_rrc_failure_reporting(rnti_t rnti, int fail);
int connect_socket();

typedef struct {
  PHY_VARS_NR_UE *ue;
  NR_UE_RRC_INST_t *rrc_inst;
} O1_PTHREAD_ARGS;

#endif /* _EXECUTABLES_STATS_H_ */