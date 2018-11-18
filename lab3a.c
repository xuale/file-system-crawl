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
unsigned int group_cnt = 0;
int *bm_inodes;

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

void read_groups()
{
  group_cnt = (superblock.s_blocks_count - 1) / superblock.s_blocks_per_group + 1;
  groupdesc = malloc(group_cnt * sizeof(struct ext2_group_desc));

  int c = pread(img_fd, groupdesc, group_cnt * sizeof(struct ext2_group_desc), blocksize + 1024);
  if (c < 0)
  {
    dump_error("Could not load image", 2);
  }

  int i;
  for (i = 0; i < group_cnt - 1; i++)
  {
    struct ext2_group_desc *gd = groupdesc + i;
    printf("GROUP,%u,%u,%u,%u,%u,%u,%u,%u\n",
           i,
           superblock.s_blocks_per_group,
           superblock.s_inodes_per_group,
           gd->bg_free_blocks_count,
           gd->bg_free_inodes_count,
           gd->bg_block_bitmap,
           gd->bg_inode_bitmap,
           gd->bg_inode_table);
  }

  uint32_t last_block = superblock.s_blocks_count % (group_cnt - 1);
  uint32_t last_inode = superblock.s_inodes_count % (group_cnt - 1);
  struct ext2_group_desc *last_gd = groupdesc + group_cnt - 1;
  printf("GROUP,%u,%u,%u,%u,%u,%u,%u,%u\n",
           i,
           superblock.s_blocks_per_group,
           superblock.s_inodes_per_group,
           last_gd->bg_free_blocks_count,
           last_gd->bg_free_inodes_count,
           last_gd->bg_block_bitmap,
           last_gd->bg_inode_bitmap,
           last_gd->bg_inode_table);
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
