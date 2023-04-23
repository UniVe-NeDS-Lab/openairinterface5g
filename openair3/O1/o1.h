
#ifndef O1_h
#define O1_h

#include "PHY/defs_nr_UE.h"
#include "openair2/LAYER2/NR_MAC_gNB/mac_proto.h"
#include <json-c/json.h>
#include <curl/curl.h>
#include <time.h>
#include "agent/o1_agent.h"
#include "agent/o1_agent_api.h"

#define CONFIG_STRING_O1AGENT "o1_agent"

#define O1AGENT_CONFIG_ENABLE "enable"
#define O1AGENT_CONFIG_URL "url"
#define O1AGENT_CONFIG_REPORT_INTERVAL "report_interval"

#define O1AGENT_PARAMS_DESC   \
{  \
    {O1AGENT_CONFIG_ENABLE, "Enable the O1 Agent", PARAMFLAG_BOOL, iptr : NULL, defintval : o1agent_config_enable_default, TYPE_INT, 0},            \
    {O1AGENT_CONFIG_URL, "VES termination URL", 0, strptr : NULL, defstrval : (char *)o1agent_config_url_default, TYPE_STRING, 0},              \
    {O1AGENT_CONFIG_REPORT_INTERVAL, "Report interval", 0, u16ptr : NULL, defuintval : e2agent_config_report_interval_default, TYPE_UINT16, 0}, \
}

#define O1AGENT_CONFIG_ENABLE_IDX 0
#define O1AGENT_CONFIG_URL_IDX 1
#define O1AGENT_CONFIG_REPORT_INTERVAL_IDX 2

static const bool o1agent_config_enable_default = 0;
static const char *const o1agent_config_url_default = "http://localhost/";
static const uint16_t e2agent_config_report_interval_default = 5; // seconds



// void *nr_ue_O1_reporting(void *param);
void *nr_gNB_O1_reporting(o1_agent_args_t *args);
void o1_ulsch_failure_reporting(rnti_t rnti, int fail);
void o1_rrc_failure_reporting(rnti_t rnti, int fail);
int connect_socket();

#endif /* _O1_H_ */