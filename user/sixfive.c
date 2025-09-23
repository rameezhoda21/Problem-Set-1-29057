#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

// Check if character is a delimiter
int is_delim(char c) {
  char *delims = " -\r\t\n./,";
  for (int i = 0; delims[i]; i++) {
    if (c == delims[i]) return 1;
  }
  return 0;
}

// Convert string of digits to int
int toint(char *s, int len) {
  int n = 0;
  for (int i = 0; i < len; i++) {
    n = n * 10 + (s[i] - '0');
  }
  return n;
}

void process_fd(int fd) {
  char buf[512];
  int n, i;
  char numbuf[64];
  int numlen = 0;

  while ((n = read(fd, buf, sizeof(buf))) > 0) {
    for (i = 0; i < n; i++) {
      char c = buf[i];
      if (c >= '0' && c <= '9') {
        if (numlen < sizeof(numbuf)-1) {
          numbuf[numlen++] = c;
        }
      } else {
        if (numlen > 0) {
          int val = toint(numbuf, numlen);
          if (val % 5 == 0 || val % 6 == 0) {
            printf("%d\n", val);
          }
          numlen = 0;
        }
      }
    }
  }
  // Handle trailing number
  if (numlen > 0) {
    int val = toint(numbuf, numlen);
    if (val % 5 == 0 || val % 6 == 0) {
      printf("%d\n", val);
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(2, "usage: sixfive file...\n");
    exit(1);
  }
  for (int i = 1; i < argc; i++) {
    int fd = open(argv[i], O_RDONLY);
    if (fd < 0) {
      fprintf(2, "sixfive: cannot open %s\n", argv[i]);
      continue;
    }
    process_fd(fd);
    close(fd);
  }
  exit(0);
}

