/*
 * Copyright 2013 Murali Suriar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <arpa/inet.h>
#include <err.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/types.h>
#include <unistd.h>

#define RIP_ROUTERS "224.0.0.9"
#define BUFLEN 512
#define RIP_PORT 520

#define RIP_CMD_RESPONSE 2

#define RIP_METRIC_UNREACH 16

#define SLEEP_INTERVAL 30
#define CMD_TIMEOUT 25

// Global variables.
volatile int kill_child = -1;
int successes = 0;
int failures = 0;
int healthy = 0;


struct RIPPacket {
  uint8_t command;
  uint8_t version;
  uint16_t pad1;
  uint16_t afi;
  uint16_t pad2;
  uint32_t ip_address;
  uint32_t subnet_mask;
  uint32_t next_hop;
  uint32_t metric;
};

static void catch_child(int sig) {
  (void)sig;
  kill_child = 0;
}

static char *get_net_from_prefix(const char *prefix) {
  char *loc = strchr(prefix, '/');

  char *network = strndup(prefix, loc-prefix);
  return network;
}


static int get_prefix_len_from_prefix(const char *prefix) {
  int a,b,c,d,len;
  sscanf(prefix, "%i.%i.%i.%i/%i", &a, &b, &c, &d, &len);
  return len;
}


static uint32_t prefix_len_to_subnet_mask(int prefix_len) {
    uint32_t netmask = UINT32_MAX;
    netmask <<= 32 - prefix_len;
    return netmask;
}


static struct RIPPacket *create_packet(const char *prefix, int metric) {
  struct RIPPacket *data = malloc(sizeof(struct RIPPacket));

  data->command = RIP_CMD_RESPONSE;
  data->version = 2;
  data->pad1 = 0;
  data->afi = htons((uint16_t) AF_INET);
  data->pad2 = 0;

  // IP address
  char *net = get_net_from_prefix(prefix);
  struct in_addr addr;
  inet_aton(net, &addr);
  data->ip_address = addr.s_addr;

  // Subnet mask
  int prefix_len = get_prefix_len_from_prefix(prefix);
  data->subnet_mask = htonl(prefix_len_to_subnet_mask(prefix_len));

  data->next_hop = 0;
  data->metric = htonl((uint32_t) metric);

  return data;
}


static void send_packet(struct RIPPacket *r) {
  struct sockaddr_in si_other;
  int s, slen=sizeof(si_other);
  char buf[BUFLEN];

  if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1) {
    err(50, "Unable to create socket.\n");
  }

  memset((char *) &si_other, 0, sizeof(si_other));
  si_other.sin_family = AF_INET;
  si_other.sin_port = htons(RIP_PORT);

  if (inet_aton(RIP_ROUTERS, &si_other.sin_addr) == 0) {
    err(51, "inet_aton failed\n");
  }

  memcpy(buf, r, sizeof(*r));
  if (sendto(s, buf, sizeof(*r), 0, (struct sockaddr *) &si_other, slen) == -1) {
    err(52, "Error sending packet.\n");
  }
}


static int run_child(int timeout, char *argv[]) {
  int status = 0;
  pid_t childPid = vfork();

  if (childPid != 0) {
    // Parent process
    // Assume child will be killed.
    kill_child = 1;

    signal(SIGCHLD, catch_child);
    // Sleep for specified timeout.
    sleep(timeout);

    if (kill_child) {
      // Kill the child process group.
      status = -10;
      int kill_result = killpg(childPid, SIGKILL);
      if (kill_result != 0) {
        err(errno, "Failed to kill child process group!\n");
      }
    } else {
      // Or don't
      pid_t finished;
      finished = wait(&status);
    }
    return status;
  } else {
    // Child. Set process group, then exec.
    setpgrp();
    if (execvp(argv[2], &argv[2]) == -1) {
      return errno;
    } else {
      // Here to suppress compiler warnings; exec has succeeded, so never gets
      // hit.
      return status;
    }
  }
}


static void run_loop(char *argv[]) {
  char *prefix = argv[1];

  struct RIPPacket *healthy_update = create_packet(prefix, 1);
  struct RIPPacket *unhealthy_update = create_packet(prefix, RIP_METRIC_UNREACH);

  while (1) {
    if (run_child(CMD_TIMEOUT, argv)) {
      /* Returned non-zero, therefore failure */
      successes = 0;
      failures += 1;
    } else {
      /* Returned 0; therefore success. */
      failures = 0;
      successes += 1;
    }

    if (successes >= 3) {
      healthy = 100;
    }

    if (failures >= 3) {
      healthy = 0;
    }

    if (healthy) {
      send_packet(healthy_update);
    } else {
      send_packet(unhealthy_update);
    }
    sleep(SLEEP_INTERVAL-CMD_TIMEOUT);
  }
}

int main(int argc, char *argv[])
{
  if (argc < 3) {
    err(10, "Usage: fabler <prefix>/<len> <healthcheck_cmd> [<healtcheck_args>...]");
  }

  run_loop(argv);
  exit(0);
}
