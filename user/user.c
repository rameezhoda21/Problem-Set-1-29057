// user/find.c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user/user.h"

static void find(char *path, char *target) {
  char buf[512], *p;
  int fd;
  struct stat st;
  struct dirent de;

  if ((fd = open(path, 0)) < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }
  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch (st.type) {
  case T_FILE: {
    // Compare the leaf name of path with target
    char *leaf = path + strlen(path);
    while (leaf > path && *(leaf - 1) != '/')
      leaf--;
    if (strcmp(leaf, target) == 0)
      printf("%s\n", path);
    break;
  }
  case T_DIR:
    // Prepare buf = "path/"
    if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)) {
      fprintf(2, "find: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf + strlen(buf);
    if (*(p - 1) != '/') {
      *p++ = '/';
      *p = 0;
    }
    // Read entries
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
      if (de.inum == 0)
        continue;

      // Copy name into a temp C-string (dirent name may not be NUL-terminated)
      char name[DIRSIZ + 1];
      memmove(name, de.name, DIRSIZ);
      name[DIRSIZ] = 0;

      // Skip "." and ".."
      if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
        continue;

      // Build child path into buf
      char *q = p;
      // reset tail after last '/'
      *q = 0;
      if (strlen(buf) + strlen(name) + 1 > sizeof(buf)) {
        fprintf(2, "find: path too long\n");
        continue;
      }
      strcpy(q, name);

      // Stat the child to decide whether to print/recurse
      if (stat(buf, &st) < 0) {
        // If stat fails (e.g., deleted entry), skip
        continue;
      }

      // If the child's leaf name matches target, print it
      if (strcmp(name, target) == 0)
        printf("%s\n", buf);

      // Recurse into subdirectories
      if (st.type == T_DIR)
        find(buf, target);
    }
    break;
  }
  close(fd);
}

int
main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(2, "usage: find <start-path> <name>\n");
    exit(1);
  }
  find(argv[1], argv[2]);
  exit(0);
}

