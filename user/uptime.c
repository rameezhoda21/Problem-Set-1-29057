// user/uptime.c
#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int t = uptime();          // call the system call
  printf("%d ticks\n", t);   // print the result
  exit(0);
}
