

#include "defs.h"
#include "rrc_proto.h"

int nr_l3_init_ue(void){
    LOG_I(RRC, "[MAIN] NR UE MAC initialization...\n");

    openair_rrc_top_init_ue_nr(); 

    return 1;

}
