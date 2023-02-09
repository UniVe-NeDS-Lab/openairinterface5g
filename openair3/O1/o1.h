
#ifndef O1_h
#define O1_h

#include "PHY/defs_nr_UE.h"
#include "openair2/LAYER2/NR_MAC_gNB/mac_proto.h"
#include <json-c/json.h>
#include <curl/curl.h>
#include <time.h>
#include "agent/o1_agent.h"
#include "agent/o1_agent_api.h"



// void *nr_ue_O1_reporting(void *param);
void *nr_gNB_O1_reporting(o1_agent_args_t *args);
void o1_ulsch_failure_reporting(rnti_t rnti, int fail);
void o1_rrc_failure_reporting(rnti_t rnti, int fail);
int connect_socket();

#endif /* _O1_H_ */