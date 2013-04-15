#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

void print_args(int argc, char *argv[]);
void create_child(void);
int child(void);
int parent(void);

int main(int argc, char *argv[])
{
  print_args(argc, argv);
  create_child();
  return 0;
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
  childPid = fork();

  if (childPid != 0) {
    printf("I'm the parent! My child is %d\n", childPid);
    parent();
  } else {
    printf("I'm the child. I have no idea what's going on.\n");
    child();
  }
}

int child(void) {
  char *program = "/usr/bin/true";
  char *nothing[0];
  int ret;
  ret = execv(program, nothing);
  return ret;
}

int parent(void) {
  pid_t finished;
  int status;
  finished = wait(status);
  printf("Child process with PID %d terminated with exit status %d\n.", finished, status);
  return status;
}
