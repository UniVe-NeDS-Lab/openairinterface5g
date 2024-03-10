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

/*
 * o1_messages_def.h
 *
 *  Created on: 2023
 *      Author: Gabriele Gemmi
 *      Email: gabriele.gemmi@unive.it
 */

MESSAGE_DEF(O1_RLC_FAIL, MESSAGE_PRIORITY_MED, O1RlcFailMessage, o1_rlc_fail)
MESSAGE_DEF(O1_RLC_COMPLETE, MESSAGE_PRIORITY_MED, O1RlcCompleteMessage, o1_rlc_complete)
MESSAGE_DEF(O1_ULSCH_FAIL, MESSAGE_PRIORITY_MED, O1ulschFailMessage, o1_ulsch_fail)