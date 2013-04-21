#include <arpa/inet.h>
#include <netinet/in.h>
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


struct RIPPacket {
  int command;
  int version;
  int afi;
  int ip_address;
  int subnet_mask;
  int next_hop;
  int metric;
};

void print_args(int argc, char *argv[]);
void create_child(void);
int child(void);
int parent(void);
void send_packet(void);

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

  for (int i=0; i < 1; i++) {
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
      // send_healthy(*prefix);
    } else {
      // send_unhealthy(*prefix);
    }
    send_packet();
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

void send_packet(void) {
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

  sprintf(buf, "Helloooo!!!!\n");
  if (sendto(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, slen) == -1) {
    diep("sendto()");
  }
}

int main(int argc, char *argv[])
{
  char *prefix = argv[1];
  printf("prefix: %s\n", prefix);
  char *net = get_net_from_prefix(prefix);
  int pfl = get_prefix_len_from_prefix(prefix);
  printf("net: %s\n", net);
  printf("pfl: %i\n", pfl);
  // run_loop(argc, argv);
  exit(0);
}
