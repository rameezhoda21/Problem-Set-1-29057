#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

void memdump(char *fmt, char *data);

int
main(int argc, char *argv[])
{
  if(argc == 1){
    printf("Example 1:\n");
    int a[2] = { 61810, 2025 };
    memdump("ii", (char*) a);
    
    printf("Example 2:\n");
    memdump("S", "a string");
    
    printf("Example 3:\n");
    char *s = "another";
    memdump("s", (char *) &s);

    struct sss {
      char *ptr;
      int num1;
      short num2;
      char byte;
      char bytes[8];
    } example;
    
    example.ptr = "hello";
    example.num1 = 1819438967;
    example.num2 = 100;
    example.byte = 'z';
    strcpy(example.bytes, "xyzzy");
    
    printf("Example 4:\n");
    memdump("pihcS", (char*) &example);
    
    printf("Example 5:\n");
    memdump("sccccc", (char*) &example);
  } else if(argc == 2){
    // format in argv[1], up to 512 bytes of data from standard input.
    char data[512];
    int n = 0;
    memset(data, '\0', sizeof(data));
    while(n < sizeof(data)){
      int nn = read(0, data + n, sizeof(data) - n);
      if(nn <= 0)
        break;
      n += nn;
    }
    memdump(argv[1], data);
  } else {
    printf("Usage: memdump [format]\n");
    exit(1);
  }
  exit(0);
}

static void print_hex64(uint64 v) {
  // Uppercase hex, no 0x, no leading zeros; print "0" if v == 0
  if (v == 0) {
    printf("0\n");
    return;
  }
  char buf[17]; // up to 16 hex digits + NUL
  int j = 0;
  int started = 0;
  for (int shift = 60; shift >= 0; shift -= 4) {
    int d = (v >> shift) & 0xF;
    if (!started) {
      if (d == 0) continue;
      started = 1;
    }
    buf[j++] = (d < 10) ? ('0' + d) : ('A' + (d - 10));
  }
  buf[j] = 0;
  printf("%s\n", buf);
}
void
memdump(char *fmt, char *data)
{
  for (int i = 0; fmt[i]; i++) {
    char f = fmt[i];

    switch (f) {
    case 'i': { // 4-byte int, decimal
      int v = 0;
      memmove(&v, data, sizeof(int));
      printf("%d\n", v);
      data += sizeof(int);
      break;
    }

    case 'p': { // next 8 bytes as 64-bit integer, hex
      uint64 v = 0;
      memmove(&v, data, sizeof(uint64));
      print_hex64(v);
      data += sizeof(uint64);
      break;
    }

    case 'h': { // 2-byte short, decimal
      short v = 0;
      memmove(&v, data, sizeof(short));
      printf("%d\n", (int)v);
      data += sizeof(short);
      break;
    }

    case 'c': { // 1-byte ASCII char
      unsigned char v = *(unsigned char*)data;
      printf("%c\n", (char)v);
      data += 1;
      break;
    }

    case 's': { // next 8 bytes is a pointer to C-string; print the string
      uint64 ptr = 0;
      memmove(&ptr, data, sizeof(uint64));
      if (ptr == 0) {
        printf("(null)\n");
      } else {
        printf("%s\n", (char*)ptr);
      }
      data += sizeof(uint64);
      break;
    }

    case 'S': { // inline C-string starting at data; print until NUL
      char *s = (char*)data;
      printf("%s\n", s);
      // Advance data past the NUL terminator (S usually ends the format)
      while (*data != '\0') data++;
      data++; // skip NUL
      break;
    }

    default:
      // Unknown specifier: ignore or print an error
      // fprintf(2, "memdump: unknown format '%c'\n", f);
      break;
    }
  }
}
