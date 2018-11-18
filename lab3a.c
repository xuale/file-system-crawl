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
#include <unistd.h>

#include "ext2_fs.h"

int img_fd;
struct ext2_super_block superblock;
struct ext2_group_desc *groupdesc;
unsigned int blocksize = 0;
unsigned int n_groups = 0;
int *inode_bitmap;

void dump_error(char *message, int exit_code)
{
  // Utility for printing to stderr
  fprintf(stderr, "%s\nerrno = %d\n%s", message, errno, strerror(errno));
  exit(exit_code);
}

void read_superblock()
{
  if (pread(img_fd, &superblock, sizeof(struct ext2_super_block), 1024) == -1)
  {
    dump_error("Could not read superblock", 2);
  }
  if (superblock.s_magic != EXT2_SUPER_MAGIC)
  {
    dump_error("Argument not a ext2 file system", 2);
  }
  blocksize = EXT2_MIN_BLOCK_SIZE << superblock.s_log_block_size;
  printf("SUPERBLOCK,%u,%u,%u,%u,%u,%u,%u\n",
         superblock.s_blocks_count,
         superblock.s_inodes_count,
         blocksize,
         superblock.s_inode_size,
         superblock.s_blocks_per_group,
         superblock.s_inodes_per_group,
         superblock.s_first_ino);
}

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    dump_error("Must provide image file as argument", 1);
  }

  const char *img = argv[1];
  img_fd = open(img, O_RDONLY);
  if (img_fd < 0)
  {
    dump_error("Could not open file system image", 2);
  }
  read_superblock();
  exit(0);
}
