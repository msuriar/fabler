#include <string.h>
#include <stdio.h>

void split_str(char* arg) {
  printf("argv[1]: %s\n", arg);
  char *loc = strchr(arg, '/');
  if (loc != NULL) {
    printf("loc: %p\n", loc);
  } else {
    printf("Null pointer. Probably not ideal.");
  }

  printf("arg: %p ;\n", arg);

  char *network = malloc((loc-arg)*sizeof(char));
  if (network == NULL) {
    printf("Null pointer: %p\n", network);
  } else {
    printf("Not null pointer: %p\n", network);
  }
  printf("loc - arg = %p",  loc-arg);
  strlcpy(network, arg, loc - arg + 1);
  printf("network: %s\n", network);
}

int main(int argc, char* argv[]) {
  split_str(argv[1]);
}
