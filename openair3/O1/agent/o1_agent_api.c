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

#include "o1_agent_api.h"
#include "o1_agent.h"
#include <pthread.h>

static o1_agent_t* agent = NULL;

static pthread_t thrd_agent;

static inline void* static_start_agent(void* a)
{
  (void)a;
  // Blocking...
  o1_start_agent(agent);
  return NULL;
}

void init_o1_agent_api(o1_agent_args_t const* args)
{
  assert(agent == NULL);
  agent = o1_init_agent(args->url, args->report_interval);
  //printf("O1 agent: %")
  // Spawn a new thread for the agent
  const int rc = pthread_create(&thrd_agent, NULL, static_start_agent, NULL);
  // alternative using itti task:
  // itti_create_task(TASK_O1, nr_gNB_O1_reporting, (void*)&args);
  assert(rc == 0);
}

void stop_o1_agent_api(void)
{
  assert(agent != NULL);
  o1_free_agent(agent);
  int const rc = pthread_join(thrd_agent, NULL);
  assert(rc == 0);
}
