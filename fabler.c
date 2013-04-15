#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

void print_args(int argc, char *argv[]);
void create_child(void);
int child(void);
int parent(void);

int run_child(int argc, char *argv[]);

int main(int argc, char *argv[])
{
  char *prefix = argv[1];
  int ret = 1000;
  ret = run_child(argc, argv);
  printf("Prefix: %s\n", prefix);
  printf("Ret: %d\n", ret);
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
