
#ifndef O1_h
#define O1_h

#define O1_SOCKET_PATH "/run/openair_o1"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "o1_proto/ue_O1_measurements.pb-c.h"
#include "PHY/defs_nr_UE.h"
#include "openair2/LAYER2/NR_MAC_gNB/mac_proto.h"

void *nr_ue_O1_reporting(void *param);
void *nr_gNB_O1_reporting(void *param);
int connect_socket();

#endif /* _EXECUTABLES_STATS_H_ */