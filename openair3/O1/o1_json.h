

#ifndef O1_json_h
#define O1_json_h

#include "o1.h"

json_object *gen_head();
json_object *gen_hb();
json_object *gen_fm();
json_object *gen_pm();

struct pm_fields {
  int rnti;
  int avg_rsrp;
  int srs_wide_band_snr;
  int dlsch_mcs;
  int ulsch_mcs;
  int cqi;
  float dlsch_bler;
  float ulsch_bler;
};

#endif