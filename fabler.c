#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

void print_args(int argc, char *argv[]);
void create_child(void);

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
  } else {
    printf("I'm the child. I have no idea what's going on.");
  }
}
