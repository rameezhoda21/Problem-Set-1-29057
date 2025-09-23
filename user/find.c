#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/param.h"   // MAXARG
#include "user/user.h"

// ==================== Regex functions (from grep.c) ====================
// Supports ., ^, $, and *
static int matchhere(char *re, char *text);
static int matchstar(int c, char *re, char *text);

int
match(char *re, char *text)
{
  if(re[0] == '^')
    return matchhere(re+1, text);
  do {  // must look even if string is empty
    if(matchhere(re, text))
      return 1;
  } while(*text++ != '\0');
  return 0;
}

static int
matchhere(char *re, char *text)
{
  if(re[0] == '\0')
    return 1;
  if(re[1] == '*')
    return matchstar(re[0], re+2, text);
  if(re[0] == '$' && re[1] == '\0')
    return *text == '\0';
  if(*text!='\0' && (re[0]=='.' || re[0]==*text))
    return matchhere(re+1, text+1);
  return 0;
}

static int
matchstar(int c, char *re, char *text)
{
  do {  // a * matches zero or more instances
    if(matchhere(re, text))
      return 1;
  } while(*text!='\0' && (*text++==c || c=='.'));
  return 0;
}

// ==================== Exec helper ====================

static void
run_exec(char *filepath, char **cmd, int cmdn)
{
  if (cmdn + 2 > MAXARG) {
    fprintf(2, "find: too many args for -exec\n");
    return;
  }

  char *argv[MAXARG];
  int i = 0;
  for (; i < cmdn; i++) argv[i] = cmd[i];
  argv[i++] = filepath;
  argv[i] = 0;

  int pid = fork();
  if (pid < 0) {
    fprintf(2, "find: fork failed\n");
    return;
  }
  if (pid == 0) {
    exec(argv[0], argv);
    fprintf(2, "find: exec %s failed\n", argv[0]);
    exit(1);
  }
  wait(0);
}

// ==================== Find implementation ====================

static void
findrec(char *path, char *pattern, int do_exec, char **cmd, int cmdn)
{
  char buf[512], *p;
  int fd;
  struct stat st;
  struct dirent de;

  if((fd = open(path, 0)) < 0){
    // Optional: warn
    // fprintf(2, "find: cannot open %s\n", path);
    return;
  }
  if(fstat(fd, &st) < 0){
    // fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  if(st.type == T_FILE){
    // extract leaf name from path
    char *leaf = path + strlen(path);
    while(leaf > path && *(leaf-1) != '/')
      leaf--;
    if(match(pattern, leaf)){
      if (do_exec) run_exec(path, cmd, cmdn);
      else printf("%s\n", path);
    }
    close(fd);
    return;
  }

  if(st.type == T_DIR){
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)){
      // fprintf(2, "find: path too long\n");
      close(fd);
      return;
    }
    strcpy(buf, path);
    p = buf + strlen(buf);
    if(p == buf || *(p-1) != '/'){
      *p++ = '/';
      *p = 0;
    }

    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;

      char name[DIRSIZ+1];
      memmove(name, de.name, DIRSIZ);
      name[DIRSIZ] = 0;

      if(strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
        continue;

      // build buf = "<path>/<name>"
      *p = 0;
      if(strlen(buf) + strlen(name) + 1 >= sizeof(buf)){
        // fprintf(2, "find: path too long\n");
        continue;
      }
      strcpy(p, name);

      if(stat(buf, &st) < 0)
        continue;

      if(match(pattern, name)){
        if (do_exec) run_exec(buf, cmd, cmdn);
        else printf("%s\n", buf);
      }

      if(st.type == T_DIR)
        findrec(buf, pattern, do_exec, cmd, cmdn);
    }
    close(fd);
  }
}

int
main(int argc, char *argv[])
{
  if(argc < 3){
    fprintf(2, "usage: find <start-path> <pattern> [-exec <cmd> [args...]]\n");
    exit(1);
  }

  char *start   = argv[1];
  char *pattern = argv[2];

  int do_exec = 0;
  char **cmd = 0;
  int cmdn = 0;

  // Optional -exec after pattern: find <path> <pattern> -exec <cmd> [args...]
  if (argc >= 4 && strcmp(argv[3], "-exec") == 0) {
    if (argc < 5) {
      fprintf(2, "find: -exec requires a command\n");
      exit(1);
    }
    do_exec = 1;
    cmd = &argv[4];
    cmdn = argc - 4;
  }

  findrec(start, pattern, do_exec, cmd, cmdn);
  exit(0);
}

