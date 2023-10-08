/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
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

#include "cuup_cucp_if.h"
#include "e1ap_messages_types.h"
#include "intertask_interface.h"

static void bearer_setup_response_direct(const e1ap_bearer_setup_resp_t *resp)
{
  MessageDef *msg = itti_alloc_new_message(TASK_MAC_GNB, 0, E1AP_BEARER_CONTEXT_SETUP_RESP);
  e1ap_bearer_setup_resp_t *msg_resp = &E1AP_BEARER_CONTEXT_SETUP_RESP(msg);
  *msg_resp = *resp;
  itti_send_msg_to_task(TASK_RRC_GNB, 0, msg);
}

static void bearer_modif_response_direct(const e1ap_bearer_modif_resp_t *resp)
{
  MessageDef *msg = itti_alloc_new_message(TASK_MAC_GNB, 0, E1AP_BEARER_CONTEXT_MODIFICATION_RESP);
  e1ap_bearer_modif_resp_t *msg_resp = &E1AP_BEARER_CONTEXT_MODIFICATION_RESP(msg);
  *msg_resp = *resp;
  itti_send_msg_to_task(TASK_RRC_GNB, 0, msg);
}

static void bearer_release_complete_direct(const e1ap_bearer_release_cplt_t *cplt)
{
  MessageDef *msg = itti_alloc_new_message(TASK_MAC_GNB, 0, E1AP_BEARER_CONTEXT_RELEASE_CPLT);
  e1ap_bearer_release_cplt_t *msg_cplt = &E1AP_BEARER_CONTEXT_RELEASE_CPLT(msg);
  *msg_cplt = *cplt;
  itti_send_msg_to_task(TASK_RRC_GNB, 0, msg);
}

void cuup_cucp_init_direct(e1_if_t *iface)
{
  iface->bearer_setup_response = bearer_setup_response_direct;
  iface->bearer_modif_response = bearer_modif_response_direct;
  iface->bearer_release_complete = bearer_release_complete_direct;
}
