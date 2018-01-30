/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*! \file lte-enb.c
 * \brief Top-level threads for eNodeB
 * \author R. Knopp, F. Kaltenberger, Navid Nikaein
 * \date 2012
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr, navid.nikaein@eurecom.fr
 * \note
 * \warning
 */

#define _GNU_SOURCE
#include <pthread.h>

#include "time_utils.h"

#undef MALLOC //there are two conflicting definitions, so we better make sure we don't use it at all

#include "rt_wrapper.h"

#include "assertions.h"


#include "PHY/types.h"

#include "PHY/defs.h"
#undef MALLOC //there are two conflicting definitions, so we better make sure we don't use it at all
//#undef FRAME_LENGTH_COMPLEX_SAMPLES //there are two conflicting definitions, so we better make sure we don't use it at all

#include "../../ARCH/COMMON/common_lib.h"

//#undef FRAME_LENGTH_COMPLEX_SAMPLES //there are two conflicting definitions, so we better make sure we don't use it at all

#include "PHY/LTE_TRANSPORT/if4_tools.h"
#include "PHY/LTE_TRANSPORT/if5_tools.h"

#include "PHY/extern.h"
#include "SCHED/extern.h"
#include "LAYER2/MAC/extern.h"

#include "../../SIMU/USER/init_lte.h"

#include "LAYER2/MAC/defs.h"
#include "LAYER2/MAC/extern.h"
#include "LAYER2/MAC/proto.h"
#include "RRC/LITE/extern.h"
#include "PHY_INTERFACE/extern.h"
#include "PHY_INTERFACE/defs.h"
#ifdef SMBV
#include "PHY/TOOLS/smbv.h"
unsigned short config_frames[4] = {2,9,11,13};
#endif
#include "UTIL/LOG/log_extern.h"
#include "UTIL/OTG/otg_tx.h"
#include "UTIL/OTG/otg_externs.h"
#include "UTIL/MATH/oml.h"
#include "UTIL/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "enb_config.h"
//#include "PHY/TOOLS/time_meas.h"

#ifndef OPENAIR2
#include "UTIL/OTG/otg_extern.h"
#endif

#if defined(ENABLE_ITTI)
# if defined(ENABLE_USE_MME)
#   include "s1ap_eNB.h"
#ifdef PDCP_USE_NETLINK
#   include "SIMULATION/ETH_TRANSPORT/proto.h"
#endif
# endif
#endif

#include "T.h"

//#define DEBUG_THREADS 1

//#define USRP_DEBUG 1
struct timing_info_t {
  //unsigned int frame, hw_slot, last_slot, next_slot;
  RTIME time_min, time_max, time_avg, time_last, time_now;
  //unsigned int mbox0, mbox1, mbox2, mbox_target;
  unsigned int n_samples;
} timing_info;

// Fix per CC openair rf/if device update
// extern openair0_device openair0;


#if defined(ENABLE_ITTI)
extern volatile int             start_eNB;
extern volatile int             start_UE;
#endif
extern volatile int                    oai_exit;

extern openair0_config_t openair0_cfg[MAX_CARDS];

extern int transmission_mode;

extern int oaisim_flag;

//pthread_t                       main_eNB_thread;

time_stats_t softmodem_stats_mt; // main thread
time_stats_t softmodem_stats_hw; //  hw acquisition
time_stats_t softmodem_stats_rxtx_sf; // total tx time
time_stats_t softmodem_stats_rx_sf; // total rx time

/* mutex, cond and variable to serialize phy proc TX calls
 * (this mechanism may be relaxed in the future for better
 * performances)
 */
static struct {
  pthread_mutex_t  mutex_phy_proc_tx;
  pthread_cond_t   cond_phy_proc_tx;
  volatile uint8_t phy_proc_CC_id;
} sync_phy_proc;

extern double cpuf;

void exit_fun(const char* s);

void init_eNB(int,int);
void stop_eNB(int nb_inst);

int wakeup_tx(PHY_VARS_eNB *eNB,RU_proc_t *ru_proc);
void wakeup_prach_eNB(PHY_VARS_eNB *eNB,RU_t *ru,int frame,int subframe);
#ifdef Rel14
void wakeup_prach_eNB_br(PHY_VARS_eNB *eNB,RU_t *ru,int frame,int subframe);
#endif
extern int codingw;

static inline int rxtx(PHY_VARS_eNB *eNB,eNB_rxtx_proc_t *proc, char *thread_name) {

  start_meas(&softmodem_stats_rxtx_sf);

  // ****************************************
  // Common RX procedures subframe n

  T(T_ENB_PHY_DL_TICK, T_INT(eNB->Mod_id), T_INT(proc->frame_tx), T_INT(proc->subframe_tx));

  // if this is IF5 or 3GPP_eNB
  if (eNB->RU_list[0]->function < NGFI_RAU_IF4p5) {
    wakeup_prach_eNB(eNB,NULL,proc->frame_rx,proc->subframe_rx);
#ifdef Rel14
    wakeup_prach_eNB_br(eNB,NULL,proc->frame_rx,proc->subframe_rx);
#endif
  }
  // UE-specific RX processing for subframe n
  phy_procedures_eNB_uespec_RX(eNB, proc, no_relay );
  
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_ULSCH_SCHEDULER , 1 );
    
  pthread_mutex_lock(&eNB->UL_INFO_mutex);
  eNB->UL_INFO.frame     = proc->frame_rx;
  eNB->UL_INFO.subframe  = proc->subframe_rx;
  eNB->UL_INFO.module_id = eNB->Mod_id;
  eNB->UL_INFO.CC_id     = eNB->CC_id;
  eNB->if_inst->UL_indication(&eNB->UL_INFO);
  pthread_mutex_unlock(&eNB->UL_INFO_mutex);
  
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_ULSCH_SCHEDULER , 0 );
  
  if(get_nprocs() > 8)
  {
    wakeup_tx(eNB,eNB->proc.ru_proc);
  }
  else if(get_nprocs() > 4)
  {
    if(oai_exit) return(-1);
    phy_procedures_eNB_TX(eNB, proc, no_relay, NULL, 1);
    
    pthread_mutex_lock(&eNB->proc.ru_proc->mutex_eNBs);
    ++eNB->proc.ru_proc->instance_cnt_eNBs;
    pthread_cond_signal(&eNB->proc.ru_proc->cond_eNBs);
    pthread_mutex_unlock(&eNB->proc.ru_proc->mutex_eNBs);
  }
  else
  {
    if(oai_exit) return(-1);
    phy_procedures_eNB_TX(eNB, proc, no_relay, NULL, 1);
  }

  stop_meas( &softmodem_stats_rxtx_sf );
  
  return(0);
}


static void* tx_thread(void* param) {

  eNB_proc_t *eNB_proc  = (eNB_proc_t*)param;
  eNB_rxtx_proc_t *proc = &eNB_proc->proc_rxtx[1];
  PHY_VARS_eNB *eNB = RC.eNB[0][proc->CC_id];
  
  char thread_name[100];
  sprintf(thread_name,"TXnp4_%d\n",&eNB->proc.proc_rxtx[0] == proc ? 0 : 1);
  thread_top_init(thread_name,1,470000,500000,500000);
  
  wait_sync("tx_thread");
  
  while (!oai_exit) {

    if (wait_on_condition(&proc->mutex_rxtx,&proc->cond_rxtx,&proc->instance_cnt_rxtx,thread_name)<0) break;
    if (oai_exit) break;    
    // *****************************************
    // TX processing for subframe n+4
    // run PHY TX procedures the one after the other for all CCs to avoid race conditions
    // (may be relaxed in the future for performance reasons)
    // *****************************************
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_TX1_ENB,proc->subframe_tx);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_RX1_ENB,proc->subframe_rx);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX1_ENB,proc->frame_tx);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_RX1_ENB,proc->frame_rx);
    
    phy_procedures_eNB_TX(eNB, proc, no_relay, NULL, 1);
	if (release_thread(&proc->mutex_rxtx,&proc->instance_cnt_rxtx,thread_name)<0) break;
	
	
    pthread_mutex_lock(&eNB_proc->ru_proc->mutex_eNBs);
    ++eNB_proc->ru_proc->instance_cnt_eNBs;
    eNB_proc->ru_proc->timestamp_tx = proc->timestamp_tx;
    eNB_proc->ru_proc->subframe_tx  = proc->subframe_tx;
    eNB_proc->ru_proc->frame_tx     = proc->frame_tx;
    pthread_cond_signal(&eNB_proc->ru_proc->cond_eNBs);
    pthread_mutex_unlock(&eNB_proc->ru_proc->mutex_eNBs);
  }

  return 0;
}

/*!
 * \brief The RX UE-specific and TX thread of eNB.
 * \param param is a \ref eNB_proc_t structure which contains the info what to process.
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */

static void* eNB_thread_rxtx( void* param ) {

  static int eNB_thread_rxtx_status;
  eNB_proc_t *eNB_proc  = (eNB_proc_t*)param;
  eNB_rxtx_proc_t *proc = &eNB_proc->proc_rxtx[0];
  PHY_VARS_eNB *eNB = RC.eNB[0][proc->CC_id];
  //RU_proc_t *ru_proc = NULL;

  char thread_name[100];

  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);

  // set default return value
  eNB_thread_rxtx_status = 0;


  sprintf(thread_name,"RXn_TXnp4_%d\n",&eNB->proc.proc_rxtx[0] == proc ? 0 : 1);
  thread_top_init(thread_name,1,470000,500000,500000);
  pthread_setname_np( pthread_self(),"rxtx processing");
  LOG_I(PHY,"thread rxtx created id=%ld\n", syscall(__NR_gettid));

  //CPU_SET(3, &cpuset);
  //pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  
  wait_sync("eNB_thread_rxtx");

  while (!oai_exit) {
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_eNB_PROC_RXTX0+(proc->subframe_rx&1), 0 );

    if (wait_on_condition(&proc->mutex_rxtx,&proc->cond_rxtx,&proc->instance_cnt_rxtx,thread_name)<0) break;

    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_CPUID_ENB_THREAD_RXTX,sched_getcpu());
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_eNB_PROC_RXTX0+(proc->subframe_rx&1), 1 );
    //ru_proc = eNB_proc->ru_proc;
   
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_TX0_ENB,proc->subframe_tx);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_RX0_ENB,proc->subframe_rx);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX0_ENB,proc->frame_tx);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_RX0_ENB,proc->frame_rx);
 
  
    if (oai_exit) break;

    if (eNB->CC_id==0)
      if (rxtx(eNB,proc,thread_name) < 0) break;

    if (release_thread(&proc->mutex_rxtx,&proc->instance_cnt_rxtx,thread_name)<0) break;
  
  } // while !oai_exit

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_eNB_PROC_RXTX0+(proc->subframe_rx&1), 0 );

  printf( "Exiting eNB thread RXn_TXnp4\n");

  eNB_thread_rxtx_status = 0;
  return &eNB_thread_rxtx_status;
}


#if 0 //defined(ENABLE_ITTI) && defined(ENABLE_USE_MME)
// Wait for eNB application initialization to be complete (eNB registration to MME)
static void wait_system_ready (char *message, volatile int *start_flag) {
  
  static char *indicator[] = {".    ", "..   ", "...  ", ".... ", ".....",
			      " ....", "  ...", "   ..", "    .", "     "};
  int i = 0;
  
  while ((!oai_exit) && (*start_flag == 0)) {
    LOG_N(EMU, message, indicator[i]);
    fflush(stdout);
    i = (i + 1) % (sizeof(indicator) / sizeof(indicator[0]));
    usleep(200000);
  }
  
  LOG_D(EMU,"\n");
}
#endif





void eNB_top(PHY_VARS_eNB *eNB, int frame_rx, int subframe_rx, char *string,RU_t *ru)
{
  RU_proc_t *ru_proc         = &ru->proc;
  eNB_proc_t *proc           = &eNB->proc;
  eNB_rxtx_proc_t *proc_rxtx = &proc->proc_rxtx[0];

  proc->frame_rx    = frame_rx;
  proc->subframe_rx = subframe_rx;

  if (!oai_exit) {
    LOG_D(PHY,"eNB_top in %p (proc %p, CC_id %d), frame %d, subframe %d, instance_cnt_prach %d\n",
	  (void*)pthread_self(), proc, eNB->CC_id, proc->frame_rx,proc->subframe_rx,proc->instance_cnt_prach);

    T(T_ENB_MASTER_TICK, T_INT(0), T_INT(proc->frame_rx), T_INT(proc->subframe_rx));

    proc_rxtx->subframe_rx = proc->subframe_rx;
    proc_rxtx->frame_rx    = proc->frame_rx;
    proc_rxtx->subframe_tx = (proc->subframe_rx+4)%10;
    proc_rxtx->frame_tx    = (proc->subframe_rx>5) ? (1+proc->frame_rx)&1023 : proc->frame_rx;
    proc->frame_tx         = proc_rxtx->frame_tx;
    proc_rxtx->timestamp_tx = proc->timestamp_tx;

    if (rxtx(eNB,proc_rxtx,string) < 0) LOG_E(PHY,"eNB %d CC_id %d failed during execution\n",eNB->Mod_id,eNB->CC_id);
    LOG_D(PHY,"eNB_top out %p (proc %p, CC_id %d), frame %d, subframe %d, instance_cnt_prach %d\n",
	  (void*)pthread_self(), proc, eNB->CC_id, proc->frame_rx,proc->subframe_rx,proc->instance_cnt_prach);
  }
}

int wakeup_tx(PHY_VARS_eNB *eNB,RU_proc_t *ru_proc) {

  eNB_proc_t *proc=&eNB->proc;

  eNB_rxtx_proc_t *proc_rxtx1=&proc->proc_rxtx[1];//*proc_rxtx=&proc->proc_rxtx[proc->frame_rx&1];
  eNB_rxtx_proc_t *proc_rxtx0=&proc->proc_rxtx[0];

  LTE_DL_FRAME_PARMS *fp = &eNB->frame_parms;
  
  struct timespec wait;
  wait.tv_sec=0;
  wait.tv_nsec=5000000L;
  
  if (proc_rxtx1->instance_cnt_rxtx == 0) {
    LOG_E(PHY,"Frame %d, subframe %d: TX1 thread busy, dropping\n",proc_rxtx1->frame_rx,proc_rxtx1->subframe_rx);
    return(-1);
  }
  
  if (pthread_mutex_timedlock(&proc_rxtx1->mutex_rxtx,&wait) != 0) {
    LOG_E( PHY, "[eNB] ERROR pthread_mutex_lock for eNB TX1 thread %d (IC %d)\n", proc_rxtx1->subframe_rx&1,proc_rxtx1->instance_cnt_rxtx );
    exit_fun( "error locking mutex_tx" );
    return(-1);
  }

  ++proc_rxtx1->instance_cnt_rxtx;

  
  proc_rxtx1->subframe_rx   = proc_rxtx0->subframe_rx;
  proc_rxtx1->frame_rx      = proc_rxtx0->frame_rx;
  proc_rxtx1->subframe_tx   = proc_rxtx0->subframe_tx;
  proc_rxtx1->frame_tx      = proc_rxtx0->frame_tx;
  proc_rxtx1->timestamp_tx  = proc_rxtx0->timestamp_tx;
  
  // the thread can now be woken up
  if (pthread_cond_signal(&proc_rxtx1->cond_rxtx) != 0) {
    LOG_E( PHY, "[eNB] ERROR pthread_cond_signal for eNB TXnp4 thread\n");
    exit_fun( "ERROR pthread_cond_signal" );
    return(-1);
  }
  
  pthread_mutex_unlock( &proc_rxtx1->mutex_rxtx );

  return(0);
}

int wakeup_rxtx(PHY_VARS_eNB *eNB,RU_t *ru) {

  eNB_proc_t *proc=&eNB->proc;
  RU_proc_t *ru_proc=&ru->proc;

  eNB_rxtx_proc_t *proc_rxtx=&proc->proc_rxtx[0];//*proc_rxtx=&proc->proc_rxtx[proc->frame_rx&1];
  proc->ru_proc = &ru->proc;

  LTE_DL_FRAME_PARMS *fp = &eNB->frame_parms;

  int i;
  struct timespec wait;
  
  pthread_mutex_lock(&proc->mutex_RU);
  for (i=0;i<eNB->num_RU;i++) {
    if (ru == eNB->RU_list[i]) {
      if ((proc->RU_mask&(1<<i)) > 0)
	LOG_E(PHY,"eNB %d frame %d, subframe %d : previous information from RU %d (num_RU %d,mask %x) has not been served yet!\n",
	      eNB->Mod_id,proc->frame_rx,proc->subframe_rx,ru->idx,eNB->num_RU,proc->RU_mask);
      proc->RU_mask |= (1<<i);
    }
  }
  if (proc->RU_mask != (1<<eNB->num_RU)-1) {  // not all RUs have provided their information so return
    pthread_mutex_unlock(&proc->mutex_RU);
    return(0);
  }
  else { // all RUs have provided their information so continue on and wakeup eNB processing
    proc->RU_mask = 0;
    pthread_mutex_unlock(&proc->mutex_RU);
  }




  wait.tv_sec=0;
  wait.tv_nsec=5000000L;

  /* accept some delay in processing - up to 5ms */
  if (proc_rxtx->instance_cnt_rxtx == 0) {
    LOG_E(PHY,"Frame %d, subframe %d: RXTX0 thread busy, dropping\n",proc_rxtx->frame_rx,proc_rxtx->subframe_rx);
    return(-1);
  }

  // wake up TX for subframe n+4
  // lock the TX mutex and make sure the thread is ready
  if (pthread_mutex_timedlock(&proc_rxtx->mutex_rxtx,&wait) != 0) {
    LOG_E( PHY, "[eNB] ERROR pthread_mutex_lock for eNB RXTX thread %d (IC %d)\n", proc_rxtx->subframe_rx&1,proc_rxtx->instance_cnt_rxtx );
    exit_fun( "error locking mutex_rxtx" );
    return(-1);
  }
  
  /*if (proc_rxtx->instance_cnt_rxtx == 0) {
    LOG_E(PHY,"Frame %d, subframe %d: RXTX0 thread busy, dropping\n",proc_rxtx->frame_rx,proc_rxtx->subframe_rx);
    return(-1);
  }*/

  ++proc_rxtx->instance_cnt_rxtx;
  
  // We have just received and processed the common part of a subframe, say n. 
  // TS_rx is the last received timestamp (start of 1st slot), TS_tx is the desired 
  // transmitted timestamp of the next TX slot (first).
  // The last (TS_rx mod samples_per_frame) was n*samples_per_tti, 
  // we want to generate subframe (n+4), so TS_tx = TX_rx+4*samples_per_tti,
  // and proc->subframe_tx = proc->subframe_rx+4
  /*proc_rxtx->timestamp_tx = proc->timestamp_rx + (4*fp->samples_per_tti);
  proc_rxtx->frame_rx     = proc->frame_rx;
  proc_rxtx->subframe_rx  = proc->subframe_rx;
  proc_rxtx->frame_tx     = (proc_rxtx->subframe_rx > 5) ? (proc_rxtx->frame_rx+1)&1023 : proc_rxtx->frame_rx;
  proc_rxtx->subframe_tx  = (proc_rxtx->subframe_rx + 4)%10;
  */
  
  proc_rxtx->subframe_rx = ru_proc->subframe_rx;
  proc_rxtx->frame_rx    = ru_proc->frame_rx;
  proc_rxtx->subframe_tx = (ru_proc->subframe_rx+4)%10;
  proc_rxtx->frame_tx    = (ru_proc->subframe_rx>5) ? (1+ru_proc->frame_rx)&1023 : ru_proc->frame_rx;
  proc->frame_tx         = proc_rxtx->frame_tx;
  proc->frame_rx         = proc_rxtx->frame_rx;
  proc_rxtx->timestamp_tx = ru_proc->timestamp_rx+(4*fp->samples_per_tti);
  
  // the thread can now be woken up
  if (pthread_cond_signal(&proc_rxtx->cond_rxtx) != 0) {
    LOG_E( PHY, "[eNB] ERROR pthread_cond_signal for eNB RXn-TXnp4 thread\n");
    exit_fun( "ERROR pthread_cond_signal" );
    return(-1);
  }
  
  pthread_mutex_unlock( &proc_rxtx->mutex_rxtx );

  return(0);
}

void wakeup_prach_eNB(PHY_VARS_eNB *eNB,RU_t *ru,int frame,int subframe) {

  eNB_proc_t *proc = &eNB->proc;
  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  int i;

  if (ru!=NULL) {
    pthread_mutex_lock(&proc->mutex_RU_PRACH);
    for (i=0;i<eNB->num_RU;i++) {
      if (ru == eNB->RU_list[i]) {
	LOG_D(PHY,"frame %d, subframe %d: RU %d for eNB %d signals PRACH (mask %x, num_RU %d)\n",frame,subframe,i,eNB->Mod_id,proc->RU_mask_prach,eNB->num_RU);
	if ((proc->RU_mask_prach&(1<<i)) > 0)
	  LOG_E(PHY,"eNB %d frame %d, subframe %d : previous information (PRACH) from RU %d (num_RU %d, mask %x) has not been served yet!\n",
		eNB->Mod_id,frame,subframe,ru->idx,eNB->num_RU,proc->RU_mask_prach);
	proc->RU_mask_prach |= (1<<i);
      }
    }
    if (proc->RU_mask_prach != (1<<eNB->num_RU)-1) {  // not all RUs have provided their information so return
      pthread_mutex_unlock(&proc->mutex_RU_PRACH);
      return;
    }
    else { // all RUs have provided their information so continue on and wakeup eNB processing
      proc->RU_mask_prach = 0;
      pthread_mutex_unlock(&proc->mutex_RU_PRACH);
    }
  }
    
  // check if we have to detect PRACH first
  if (is_prach_subframe(fp,frame,subframe)>0) { 
    LOG_D(PHY,"Triggering prach processing, frame %d, subframe %d\n",frame,subframe);
    if (proc->instance_cnt_prach == 0) {
      LOG_W(PHY,"[eNB] Frame %d Subframe %d, dropping PRACH\n", frame,subframe);
      return;
    }
    
    // wake up thread for PRACH RX
    if (pthread_mutex_lock(&proc->mutex_prach) != 0) {
      LOG_E( PHY, "[eNB] ERROR pthread_mutex_lock for eNB PRACH thread %d (IC %d)\n", proc->thread_index, proc->instance_cnt_prach);
      exit_fun( "error locking mutex_prach" );
      return;
    }
    
    ++proc->instance_cnt_prach;
    // set timing for prach thread
    proc->frame_prach = frame;
    proc->subframe_prach = subframe;
    
    // the thread can now be woken up
    if (pthread_cond_signal(&proc->cond_prach) != 0) {
      LOG_E( PHY, "[eNB] ERROR pthread_cond_signal for eNB PRACH thread %d\n", proc->thread_index);
      exit_fun( "ERROR pthread_cond_signal" );
      return;
    }
    
    pthread_mutex_unlock( &proc->mutex_prach );
  }

}

#ifdef Rel14
void wakeup_prach_eNB_br(PHY_VARS_eNB *eNB,RU_t *ru,int frame,int subframe) {

  eNB_proc_t *proc = &eNB->proc;
  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  int i;

  if (ru!=NULL) {
    pthread_mutex_lock(&proc->mutex_RU_PRACH_br);
    for (i=0;i<eNB->num_RU;i++) {
      if (ru == eNB->RU_list[i]) {
	LOG_D(PHY,"frame %d, subframe %d: RU %d for eNB %d signals PRACH BR (mask %x, num_RU %d)\n",frame,subframe,i,eNB->Mod_id,proc->RU_mask_prach_br,eNB->num_RU);
	if ((proc->RU_mask_prach_br&(1<<i)) > 0)
	  LOG_E(PHY,"eNB %d frame %d, subframe %d : previous information (PRACH BR) from RU %d (num_RU %d, mask %x) has not been served yet!\n",
		eNB->Mod_id,frame,subframe,ru->idx,eNB->num_RU,proc->RU_mask_prach_br);
	proc->RU_mask_prach_br |= (1<<i);
      }
    }
    if (proc->RU_mask_prach_br != (1<<eNB->num_RU)-1) {  // not all RUs have provided their information so return
      pthread_mutex_unlock(&proc->mutex_RU_PRACH_br);
      return;
    }
    else { // all RUs have provided their information so continue on and wakeup eNB processing
      proc->RU_mask_prach_br = 0;
      pthread_mutex_unlock(&proc->mutex_RU_PRACH_br);
    }
  }
    
  // check if we have to detect PRACH first
  if (is_prach_subframe(fp,frame,subframe)>0) { 
    LOG_D(PHY,"Triggering prach br processing, frame %d, subframe %d\n",frame,subframe);
    if (proc->instance_cnt_prach_br == 0) {
      LOG_W(PHY,"[eNB] Frame %d Subframe %d, dropping PRACH BR\n", frame,subframe);
      return;
    }
    
    // wake up thread for PRACH RX
    if (pthread_mutex_lock(&proc->mutex_prach_br) != 0) {
      LOG_E( PHY, "[eNB] ERROR pthread_mutex_lock for eNB PRACH thread %d (IC %d)\n", proc->thread_index, proc->instance_cnt_prach_br);
      exit_fun( "error locking mutex_prach" );
      return;
    }
    
    ++proc->instance_cnt_prach_br;
    // set timing for prach thread
    proc->frame_prach_br = frame;
    proc->subframe_prach_br = subframe;
    
    // the thread can now be woken up
    if (pthread_cond_signal(&proc->cond_prach_br) != 0) {
      LOG_E( PHY, "[eNB] ERROR pthread_cond_signal for eNB PRACH BR thread %d\n", proc->thread_index);
      exit_fun( "ERROR pthread_cond_signal" );
      return;
    }
    
    pthread_mutex_unlock( &proc->mutex_prach_br );
  }

}
#endif

/*!
 * \brief The prach receive thread of eNB.
 * \param param is a \ref eNB_proc_t structure which contains the info what to process.
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */
static void* eNB_thread_prach( void* param ) {
  static int eNB_thread_prach_status;


  PHY_VARS_eNB *eNB= (PHY_VARS_eNB *)param;
  eNB_proc_t *proc = &eNB->proc;

  // set default return value
  eNB_thread_prach_status = 0;

  thread_top_init("eNB_thread_prach",1,500000,1000000,20000000);

  wait_sync("eNB_thread_prach");

  while (!oai_exit) {
    
    if (oai_exit) break;

    
    if (wait_on_condition(&proc->mutex_prach,&proc->cond_prach,&proc->instance_cnt_prach,"eNB_prach_thread") < 0) break;

    LOG_D(PHY,"Running eNB prach procedures\n");
    prach_procedures(eNB
#ifdef Rel14
		     ,0
#endif
		     );
    
    if (release_thread(&proc->mutex_prach,&proc->instance_cnt_prach,"eNB_prach_thread") < 0) break;
  }

  LOG_I(PHY, "Exiting eNB thread PRACH\n");

  eNB_thread_prach_status = 0;
  return &eNB_thread_prach_status;
}

#ifdef Rel14
/*!
 * \brief The prach receive thread of eNB for BL/CE UEs.
 * \param param is a \ref eNB_proc_t structure which contains the info what to process.
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */
static void* eNB_thread_prach_br( void* param ) {
  static int eNB_thread_prach_status;


  PHY_VARS_eNB *eNB= (PHY_VARS_eNB *)param;
  eNB_proc_t *proc = &eNB->proc;

  // set default return value
  eNB_thread_prach_status = 0;

  thread_top_init("eNB_thread_prach_br",1,500000,1000000,20000000);

  while (!oai_exit) {
    
    if (oai_exit) break;
    

    if (wait_on_condition(&proc->mutex_prach_br,&proc->cond_prach_br,&proc->instance_cnt_prach_br,"eNB_prach_thread_br") < 0) break;

    LOG_D(PHY,"Running eNB prach procedures for BL/CE UEs\n");
    prach_procedures(eNB,1);
    
    if (release_thread(&proc->mutex_prach_br,&proc->instance_cnt_prach_br,"eNB_prach_thread_br") < 0) break;
  }

  LOG_I(PHY, "Exiting eNB thread PRACH BR\n");

  eNB_thread_prach_status = 0;
  return &eNB_thread_prach_status;
}

#endif



extern void init_td_thread(PHY_VARS_eNB *, pthread_attr_t *);
extern void init_te_thread(PHY_VARS_eNB *);
//////////////////////////////////////need to modified////////////////*****

static void* process_stats_thread(void* param) {

  PHY_VARS_eNB     *eNB      = (PHY_VARS_eNB*)param;

  wait_sync("process_stats_thread");

  while (!oai_exit) {
     sleep(1);
     if (opp_enabled == 1) {
       if (eNB->td) print_meas(&eNB->ulsch_decoding_stats,"ulsch_decoding",NULL,NULL);
       if (eNB->te)
       {
         print_meas(&eNB->dlsch_turbo_encoding_preperation_stats,"dlsch_coding_prepare",NULL,NULL);
         print_meas(&eNB->dlsch_turbo_encoding_segmentation_stats,"dlsch_segmentation",NULL,NULL);
         print_meas(&eNB->dlsch_encoding_stats,"dlsch_encoding",NULL,NULL);
         print_meas(&eNB->dlsch_turbo_encoding_signal_stats,"coding_signal",NULL,NULL);
         print_meas(&eNB->dlsch_turbo_encoding_main_stats,"coding_main",NULL,NULL);
         print_meas(&eNB->dlsch_turbo_encoding_waiting_stats,"coding_wait",NULL,NULL);
         print_meas(&eNB->dlsch_turbo_encoding_wakeup_stats0,"coding_worker_0",NULL,NULL);
         print_meas(&eNB->dlsch_turbo_encoding_wakeup_stats1,"coding_worker_1",NULL,NULL);
	   }
       print_meas(&eNB->dlsch_modulation_stats,"dlsch_modulation",NULL,NULL);
     }
  }
  return(NULL);
}


void init_eNB_proc(int inst) {
  
  int i=0;
  int CC_id;
  PHY_VARS_eNB *eNB;
  eNB_proc_t *proc;
  eNB_rxtx_proc_t *proc_rxtx;
  pthread_attr_t *attr0=NULL,*attr1=NULL,*attr_prach=NULL,*attr_td=NULL;//,*attr_te=NULL,*attr_te1=NULL;
#ifdef Rel14
  pthread_attr_t *attr_prach_br=NULL;
#endif

  for (CC_id=0; CC_id<RC.nb_CC[inst]; CC_id++) {
    eNB = RC.eNB[inst][CC_id];
#ifndef OCP_FRAMEWORK
    LOG_I(PHY,"Initializing eNB processes %d CC_id %d \n",inst,CC_id);
#endif
    proc = &eNB->proc;

    proc_rxtx                      = proc->proc_rxtx;
    proc_rxtx[0].instance_cnt_rxtx = -1;
    proc_rxtx[1].instance_cnt_rxtx = -1;
    proc->instance_cnt_prach       = -1;
    proc->instance_cnt_asynch_rxtx = -1;
    proc->instance_cnt_synch       = -1;
    proc->CC_id                    = CC_id;    

    proc->first_rx=1;
    proc->first_tx=1;
    proc->RU_mask=0;
    proc->RU_mask_prach=0;

    pthread_mutex_init( &eNB->UL_INFO_mutex, NULL);
    pthread_mutex_init( &proc_rxtx[0].mutex_rxtx, NULL);
    pthread_mutex_init( &proc_rxtx[1].mutex_rxtx, NULL);
    pthread_cond_init( &proc_rxtx[0].cond_rxtx, NULL);
    pthread_cond_init( &proc_rxtx[1].cond_rxtx, NULL);

    pthread_mutex_init( &proc->mutex_prach, NULL);
    pthread_mutex_init( &proc->mutex_asynch_rxtx, NULL);
    pthread_mutex_init( &proc->mutex_RU,NULL);
    pthread_mutex_init( &proc->mutex_RU_PRACH,NULL);

    pthread_cond_init( &proc->cond_prach, NULL);
    pthread_cond_init( &proc->cond_asynch_rxtx, NULL);

    pthread_attr_init( &proc->attr_prach);
    pthread_attr_init( &proc->attr_asynch_rxtx);
    pthread_attr_init( &proc->attr_td);
    //pthread_attr_init( &proc->attr_te[0]);
    //pthread_attr_init( &proc->attr_te[1]);
    pthread_attr_init( &proc_rxtx[0].attr_rxtx);
    pthread_attr_init( &proc_rxtx[1].attr_rxtx);
#ifdef Rel14
    proc->instance_cnt_prach_br    = -1;
    proc->RU_mask_prach_br=0;
    pthread_mutex_init( &proc->mutex_prach_br, NULL);
    pthread_mutex_init( &proc->mutex_RU_PRACH_br,NULL);
    pthread_cond_init( &proc->cond_prach_br, NULL);
    pthread_attr_init( &proc->attr_prach_br);
#endif
#ifndef DEADLINE_SCHEDULER
    attr0       = &proc_rxtx[0].attr_rxtx;
    attr1       = &proc_rxtx[1].attr_rxtx;
    attr_prach  = &proc->attr_prach;
#ifdef Rel14
    attr_prach_br  = &proc->attr_prach_br;
#endif

    //    attr_td     = &proc->attr_td;
    //    attr_te     = &proc->attr_te; 
#endif
	attr_td     = &proc->attr_td;
	//attr_te     = &proc->attr_te[0];
	//attr_te1    = &proc->attr_te[1];
	pthread_create( &proc_rxtx[0].pthread_rxtx, attr0, eNB_thread_rxtx, proc );
	pthread_create( &proc_rxtx[1].pthread_rxtx, attr1, tx_thread, proc);
    if (eNB->single_thread_flag==0) {
      pthread_create( &proc_rxtx[0].pthread_rxtx, attr0, eNB_thread_rxtx, &proc_rxtx[0] );
      pthread_create( &proc_rxtx[1].pthread_rxtx, attr1, eNB_thread_rxtx, &proc_rxtx[1] );
    }
    pthread_create( &proc->pthread_prach, attr_prach, eNB_thread_prach, eNB );
#ifdef Rel14
    pthread_create( &proc->pthread_prach_br, attr_prach_br, eNB_thread_prach_br, eNB );
#endif
    char name[16];
    if (eNB->single_thread_flag==0) {
      snprintf( name, sizeof(name), "RXTX0 %d", i );
      pthread_setname_np( proc_rxtx[0].pthread_rxtx, name );
      snprintf( name, sizeof(name), "RXTX1 %d", i );
      pthread_setname_np( proc_rxtx[1].pthread_rxtx, name );
    }

    AssertFatal(proc->instance_cnt_prach == -1,"instance_cnt_prach = %d\n",proc->instance_cnt_prach);
	
	
	//////////////////////////////////////need to modified////////////////*****
    //printf("//////////////////////////////////////////////////////////////////**************************************************************** codingw = %d\n",codingw);
    if(get_nprocs() > 2 && codingw)
    {
      init_te_thread(eNB);
      init_td_thread(eNB,attr_td);
    }
    if (opp_enabled == 1) pthread_create(&proc->process_stats_thread,NULL,process_stats_thread,(void*)eNB);

    
  }

  //for multiple CCs: setup master and slaves
  /* 
     for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
     eNB = PHY_vars_eNB_g[inst][CC_id];

     if (eNB->node_timing == synch_to_ext_device) { //master
     eNB->proc.num_slaves = MAX_NUM_CCs-1;
     eNB->proc.slave_proc = (eNB_proc_t**)malloc(eNB->proc.num_slaves*sizeof(eNB_proc_t*));

     for (i=0; i< eNB->proc.num_slaves; i++) {
     if (i < CC_id)  eNB->proc.slave_proc[i] = &(PHY_vars_eNB_g[inst][i]->proc);
     if (i >= CC_id)  eNB->proc.slave_proc[i] = &(PHY_vars_eNB_g[inst][i+1]->proc);
     }
     }
     }
  */

  /* setup PHY proc TX sync mechanism */
  pthread_mutex_init(&sync_phy_proc.mutex_phy_proc_tx, NULL);
  pthread_cond_init(&sync_phy_proc.cond_phy_proc_tx, NULL);
  sync_phy_proc.phy_proc_CC_id = 0;
  
  
}



/*!
 * \brief Terminate eNB TX and RX threads.
 */
void kill_eNB_proc(int inst) {

  int *status;
  PHY_VARS_eNB *eNB;
  eNB_proc_t *proc;
  eNB_rxtx_proc_t *proc_rxtx;
  for (int CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    eNB=RC.eNB[inst][CC_id];
    
    proc = &eNB->proc;
    proc_rxtx = &proc->proc_rxtx[0];
    

    LOG_I(PHY, "Killing TX CC_id %d inst %d\n", CC_id, inst );

    if (eNB->single_thread_flag==0) {
      proc_rxtx[0].instance_cnt_rxtx = 0; // FIXME data race!
      proc_rxtx[1].instance_cnt_rxtx = 0; // FIXME data race!
      pthread_cond_signal( &proc_rxtx[0].cond_rxtx );    
      pthread_cond_signal( &proc_rxtx[1].cond_rxtx );
    }
    proc->instance_cnt_prach = 0;
    pthread_cond_signal( &proc->cond_prach );

    pthread_cond_broadcast(&sync_phy_proc.cond_phy_proc_tx);
    pthread_join( proc->pthread_prach, (void**)&status );    

    LOG_I(PHY, "Destroying prach mutex/cond\n");
    pthread_mutex_destroy( &proc->mutex_prach );
    pthread_cond_destroy( &proc->cond_prach );
#ifdef Rel14
    proc->instance_cnt_prach_br = 0;
    pthread_cond_signal( &proc->cond_prach_br );
    pthread_join( proc->pthread_prach_br, (void**)&status );    
    pthread_mutex_destroy( &proc->mutex_prach_br );
    pthread_cond_destroy( &proc->cond_prach_br );
#endif         
    LOG_I(PHY, "Destroying UL_INFO mutex\n");
    pthread_mutex_destroy(&eNB->UL_INFO_mutex);
    int i;
    if (eNB->single_thread_flag==0) {
      for (i=0;i<2;i++) {
	LOG_I(PHY, "Joining rxtx[%d] mutex/cond\n",i);
	pthread_join( proc_rxtx[i].pthread_rxtx, (void**)&status );
	LOG_I(PHY, "Destroying rxtx[%d] mutex/cond\n",i);
	pthread_mutex_destroy( &proc_rxtx[i].mutex_rxtx );
	pthread_cond_destroy( &proc_rxtx[i].cond_rxtx );
      }
    }
  }
}




void reset_opp_meas(void) {

  int sfn;
  reset_meas(&softmodem_stats_mt);
  reset_meas(&softmodem_stats_hw);
  
  for (sfn=0; sfn < 10; sfn++) {
    reset_meas(&softmodem_stats_rxtx_sf);
    reset_meas(&softmodem_stats_rx_sf);
  }
}


void print_opp_meas(void) {

  int sfn=0;
  print_meas(&softmodem_stats_mt, "Main ENB Thread", NULL, NULL);
  print_meas(&softmodem_stats_hw, "HW Acquisation", NULL, NULL);
  
  for (sfn=0; sfn < 10; sfn++) {
    print_meas(&softmodem_stats_rxtx_sf,"[eNB][total_phy_proc_rxtx]",NULL, NULL);
    print_meas(&softmodem_stats_rx_sf,"[eNB][total_phy_proc_rx]",NULL,NULL);
  }
}

void init_transport(PHY_VARS_eNB *eNB) {

  int i;
  int j;
  LTE_DL_FRAME_PARMS *fp = &eNB->frame_parms;

  for (i=0; i<NUMBER_OF_UE_MAX; i++) {
    LOG_I(PHY,"Allocating Transport Channel Buffers for DLSCH, UE %d\n",i);
    for (j=0; j<2; j++) {
      eNB->dlsch[i][j] = new_eNB_dlsch(1,8,NSOFT,fp->N_RB_DL,0,fp);
      if (!eNB->dlsch[i][j]) {
	LOG_E(PHY,"Can't get eNB dlsch structures for UE %d \n", i);
	exit(-1);
      } else {
	LOG_D(PHY,"dlsch[%d][%d] => %p\n",i,j,eNB->dlsch[i][j]);
	eNB->dlsch[i][j]->rnti=0;
      }
    }
    
    LOG_I(PHY,"Allocating Transport Channel Buffer for ULSCH, UE %d\n",i);
    eNB->ulsch[1+i] = new_eNB_ulsch(MAX_TURBO_ITERATIONS,fp->N_RB_UL, 0);
    
    if (!eNB->ulsch[1+i]) {
      LOG_E(PHY,"Can't get eNB ulsch structures\n");
      exit(-1);
    }
    
    // this is the transmission mode for the signalling channels
    // this will be overwritten with the real transmission mode by the RRC once the UE is connected
    eNB->transmission_mode[i] = fp->nb_antenna_ports_eNB==1 ? 1 : 2;
  }
  // ULSCH for RA
  eNB->ulsch[0] = new_eNB_ulsch(MAX_TURBO_ITERATIONS, fp->N_RB_UL, 0);
  
  if (!eNB->ulsch[0]) {
    LOG_E(PHY,"Can't get eNB ulsch structures\n");
    exit(-1);
  }
  eNB->dlsch_SI  = new_eNB_dlsch(1,8,NSOFT,fp->N_RB_DL, 0, fp);
  LOG_D(PHY,"eNB %d.%d : SI %p\n",eNB->Mod_id,eNB->CC_id,eNB->dlsch_SI);
  eNB->dlsch_ra  = new_eNB_dlsch(1,8,NSOFT,fp->N_RB_DL, 0, fp);
  LOG_D(PHY,"eNB %d.%d : RA %p\n",eNB->Mod_id,eNB->CC_id,eNB->dlsch_ra);
  eNB->dlsch_MCH = new_eNB_dlsch(1,8,NSOFT,fp->N_RB_DL, 0, fp);
  LOG_D(PHY,"eNB %d.%d : MCH %p\n",eNB->Mod_id,eNB->CC_id,eNB->dlsch_MCH);
  
  
  eNB->rx_total_gain_dB=130;
  
  for(i=0; i<NUMBER_OF_UE_MAX; i++)
    eNB->mu_mimo_mode[i].dl_pow_off = 2;
  
  eNB->check_for_total_transmissions = 0;
  
  eNB->check_for_MUMIMO_transmissions = 0;
  
  eNB->FULL_MUMIMO_transmissions = 0;
  
  eNB->check_for_SUMIMO_transmissions = 0;
  
  fp->pucch_config_common.deltaPUCCH_Shift = 1;
    
} 

void init_eNB_afterRU(void) {

  int inst,CC_id,ru_id,i,aa;
  PHY_VARS_eNB *eNB;


  for (inst=0;inst<RC.nb_inst;inst++) {
    for (CC_id=0;CC_id<RC.nb_CC[inst];CC_id++) {
      eNB                                  =  RC.eNB[inst][CC_id];
      phy_init_lte_eNB(eNB,0,0);
      // map antennas and PRACH signals to eNB RX
      AssertFatal(eNB->num_RU>0,"Number of RU attached to eNB %d is zero\n",eNB->Mod_id);
      LOG_I(PHY,"Mapping RX ports from %d RUs to eNB %d\n",eNB->num_RU,eNB->Mod_id);
      eNB->frame_parms.nb_antennas_rx       = 0;
      eNB->prach_vars.rxsigF[0] = (int16_t**)malloc16(64*sizeof(int16_t*));
#ifdef Rel14
      for (int ce_level=0;ce_level<4;ce_level++)
	eNB->prach_vars_br.rxsigF[ce_level] = (int16_t**)malloc16(64*sizeof(int16_t*));
#endif
      for (ru_id=0,aa=0;ru_id<eNB->num_RU;ru_id++) {
	eNB->frame_parms.nb_antennas_rx    += eNB->RU_list[ru_id]->nb_rx;

	AssertFatal(eNB->RU_list[ru_id]->common.rxdataF!=NULL,
		    "RU %d : common.rxdataF is NULL\n",
		    eNB->RU_list[ru_id]->idx);

	AssertFatal(eNB->RU_list[ru_id]->prach_rxsigF!=NULL,
		    "RU %d : prach_rxsigF is NULL\n",
		    eNB->RU_list[ru_id]->idx);

	for (i=0;i<eNB->RU_list[ru_id]->nb_rx;aa++,i++) { 
	  LOG_I(PHY,"Attaching RU %d antenna %d to eNB antenna %d\n",eNB->RU_list[ru_id]->idx,i,aa);
	  eNB->prach_vars.rxsigF[0][aa]    =  eNB->RU_list[ru_id]->prach_rxsigF[i];
#ifdef Rel14
	  for (int ce_level=0;ce_level<4;ce_level++)
	    eNB->prach_vars_br.rxsigF[ce_level][aa] = eNB->RU_list[ru_id]->prach_rxsigF_br[ce_level][i];
#endif
	  eNB->common_vars.rxdataF[aa]     =  eNB->RU_list[ru_id]->common.rxdataF[i];
	}
      }
      AssertFatal(eNB->frame_parms.nb_antennas_rx >0,
		  "inst %d, CC_id %d : nb_antennas_rx %d\n",inst,CC_id,eNB->frame_parms.nb_antennas_rx);
      LOG_I(PHY,"inst %d, CC_id %d : nb_antennas_rx %d\n",inst,CC_id,eNB->frame_parms.nb_antennas_rx);

      init_transport(eNB);
      //init_precoding_weights(RC.eNB[inst][CC_id]);
    }
    init_eNB_proc(inst);
  }

  for (ru_id=0;ru_id<RC.nb_RU;ru_id++) {

    AssertFatal(RC.ru[ru_id]!=NULL,"ru_id %d is null\n",ru_id);
    
    RC.ru[ru_id]->wakeup_rxtx         = wakeup_rxtx;
    RC.ru[ru_id]->wakeup_prach_eNB    = wakeup_prach_eNB;
    RC.ru[ru_id]->wakeup_prach_eNB_br = wakeup_prach_eNB_br;
    RC.ru[ru_id]->eNB_top             = eNB_top;
  }
}

void init_eNB(int single_thread_flag,int wait_for_sync) {
  
  int CC_id;
  int inst;
  PHY_VARS_eNB *eNB;

  if (RC.eNB == NULL) RC.eNB = (PHY_VARS_eNB***) malloc(RC.nb_L1_inst*sizeof(PHY_VARS_eNB **));
  for (inst=0;inst<RC.nb_L1_inst;inst++) {
    if (RC.eNB[inst] == NULL) RC.eNB[inst] = (PHY_VARS_eNB**) malloc(RC.nb_CC[inst]*sizeof(PHY_VARS_eNB *));
    for (CC_id=0;CC_id<RC.nb_L1_CC[inst];CC_id++) {
      if (RC.eNB[inst][CC_id] == NULL) RC.eNB[inst][CC_id] = (PHY_VARS_eNB*) malloc(sizeof(PHY_VARS_eNB));
      eNB                     = RC.eNB[inst][CC_id]; 
      eNB->abstraction_flag   = 0;
      eNB->single_thread_flag = single_thread_flag;


#ifndef OCP_FRAMEWORK
      LOG_I(PHY,"Initializing eNB %d CC_id %d\n",inst,CC_id);
#endif


      eNB->td                   = ulsch_decoding_data_all;//(get_nprocs()<=4) ? ulsch_decoding_data : ulsch_decoding_data_2thread;
      eNB->te                   = dlsch_encoding_all;//(get_nprocs()<=4) ? dlsch_encoding : dlsch_encoding_2threads;

      
      LOG_I(PHY,"Registering with MAC interface module\n");
      AssertFatal((eNB->if_inst         = IF_Module_init(inst))!=NULL,"Cannot register interface");
      eNB->if_inst->schedule_response   = schedule_response;
      eNB->if_inst->PHY_config_req      = phy_config_request;
      memset((void*)&eNB->UL_INFO,0,sizeof(eNB->UL_INFO));
      memset((void*)&eNB->Sched_INFO,0,sizeof(eNB->Sched_INFO));
      LOG_I(PHY,"Setting indication lists\n");
      eNB->UL_INFO.rx_ind.rx_pdu_list   = eNB->rx_pdu_list;
      eNB->UL_INFO.crc_ind.crc_pdu_list = eNB->crc_pdu_list;
      eNB->UL_INFO.sr_ind.sr_pdu_list = eNB->sr_pdu_list;
      eNB->UL_INFO.harq_ind.harq_pdu_list = eNB->harq_pdu_list;
      eNB->UL_INFO.cqi_ind.cqi_pdu_list = eNB->cqi_pdu_list;
      eNB->UL_INFO.cqi_ind.cqi_raw_pdu_list = eNB->cqi_raw_pdu_list;
    }

  }


  LOG_I(PHY,"[lte-softmodem.c] eNB structure allocated\n");
  


}


void stop_eNB(int nb_inst) {

  for (int inst=0;inst<nb_inst;inst++) {
    LOG_I(PHY,"Killing eNB %d processing threads\n",inst);
    kill_eNB_proc(inst);
  }
}
