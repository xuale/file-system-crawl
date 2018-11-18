// NAME: Richard Yang, Alexander Xu
// EMAIL: ryang72@ucla.edu, xualexander1@gmail.com
// ID: 704936219, 504966392

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include "ext2_fs.h"

int img_fd;

void dump_error(char *message, int exit_code)
{
  // Utility for printing to stderr
  fprintf(stderr, "%s\nerrno = %d\n%s", message, errno, strerror(errno));
  exit(exit_code);
}

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    dump_error("Must provide image file as argument", 1);
  }

  const char* img = argv[1];
  img_fd = open(img, O_RDONLY);
  if (img_fd < 0) 
  {
    dump_error("Could not open file system image", 2);
  }

  exit(0);
}