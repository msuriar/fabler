#include <arpa/inet.h>
#include <netinet/in.h>
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

char *get_net_from_prefix(char *prefix);
int get_prefix_len_from_prefix(char *prefix);

uint32_t prefix_len_to_subnet_mask(int prefix_len) {
    uint32_t netmask = UINT32_MAX;
    netmask <<= 32 - prefix_len;
    return netmask;
}

struct RIPPacket *create_packet(char *prefix, int metric) {
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
  printf("Prefix length: %d\n", prefix_len);
  data->subnet_mask = htonl(prefix_len_to_subnet_mask(prefix_len));

  data->next_hop = 0;
  data->metric = htonl((uint32_t) metric);

  return data;
}

void htonRIPPacket(struct RIPPacket *r, char b[BUFLEN]) {
  memcpy(b, r, sizeof(*r));
}

void print_args(int argc, char *argv[]);
void create_child(void);
int child(void);
int parent(void);
void send_packet(struct RIPPacket *r);

int run_child(int argc, char *argv[]);

int get_prefix_len_from_prefix(char *prefix) {
  int a,b,c,d,len;
  sscanf(prefix, "%i.%i.%i.%i/%i", &a, &b, &c, &d, &len);
  return len;
}

char *get_net_from_prefix(char *prefix) {
  char *loc = strchr(prefix, '/');

  char *network = malloc((loc-prefix)*sizeof(char));
  strlcpy(network, prefix, loc-prefix+1);
  return network;
}

void diep(char *s) {
  perror(s);
}

void run_loop(int argc, char *argv[]) {
  int successes = 0;
  int failures = 0;
  int healthy = 0;

  struct RIPPacket *healthy_update = create_packet(argv[1], 1);
  struct RIPPacket *unhealthy_update = create_packet(argv[1], RIP_METRIC_UNREACH);

  for (int i=0; i < 10; i++) {
    if (run_child(argc, argv)) {
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
    sleep(1);
  }
}

void print_args(int argc, char *argv[]) {
  printf("Hello Murali.\n");
  printf("The number of arguments is: %d\n", argc);
  for ( int i = 0; i < argc; i++ ) {
    printf("argv[%d]: %s\n", i, argv[i]);
  }
}

void create_child(void) {
  pid_t childPid;
  childPid = vfork();

  if (childPid != 0) {
    printf("I'm the parent! My child is %d\n", childPid);
    parent();
  } else {
    printf("I'm the child. I have no idea what's going on.\n");
    child();
  }
}

int child(void) {
  char *program = "true";
  char *argv[] = { program, NULL };
  int ret;
  ret = execvp(program, argv);
  return ret;
}

int parent(void) {
  pid_t finished;
  int status = 0;
  finished = wait(&status);
  printf("Child process with PID %d terminated with exit status %d.\n", finished, status);
  return status;
}

int run_child(int argc, char *argv[]) {
  int status = 0;
  pid_t childPid;

  childPid = vfork();

  if (childPid != 0) {
    pid_t finished;
    finished = wait(&status);
    return status;
  } else {
    execvp(argv[2], &argv[2]);
    return status;
  }
}

void send_packet(struct RIPPacket *r) {
  struct sockaddr_in si_other;
  int s, slen=sizeof(si_other);
  char buf[BUFLEN];

  if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1) {
    diep("socket");
  }

  memset((char *) &si_other, 0, sizeof(si_other));
  si_other.sin_family = AF_INET;
  si_other.sin_port = htons(RIP_PORT);

  if (inet_aton(RIP_ROUTERS, &si_other.sin_addr) == 0) {
    fprintf(stderr, "inet_aton() failed \n");
    exit(1);
  }

  if (r != NULL) {
    printf("Created RIP packet");
  } else {
    printf("Didn't create RIP packet.");
  }
  htonRIPPacket(r, buf);
  if (sendto(s, buf, sizeof(*r), 0, (struct sockaddr *) &si_other, slen) == -1) {
    diep("sendto()");
  }
}

int main(int argc, char *argv[])
{
  //  char *prefix = argv[1];
  //  printf("prefix: %s\n", prefix);
  //  char *net = get_net_from_prefix(prefix);
  //  int pfl = get_prefix_len_from_prefix(prefix);
  //  printf("net: %s\n", net);
  //  printf("pfl: %i\n", pfl);
  char *prefix = argv[1];
  struct RIPPacket *r = create_packet(prefix, 1);
  send_packet(r);
  exit(0);
}
