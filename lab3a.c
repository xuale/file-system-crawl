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

  unsigned int i = 0;
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
  uint32_t last_block = group_cnt > 1 ? superblock.s_blocks_count % (group_cnt - 1) : superblock.s_blocks_count;
  uint32_t last_inode = group_cnt > 1 ? superblock.s_inodes_count % (group_cnt - 1) : superblock.s_inodes_count;

  struct ext2_group_desc *last_gd = groupdesc + group_cnt - 1;
  printf("GROUP,%u,%u,%u,%u,%u,%u,%u,%u\n",
         i,
         last_block,
         last_inode,
         last_gd->bg_free_blocks_count,
         last_gd->bg_free_inodes_count,
         last_gd->bg_block_bitmap,
         last_gd->bg_inode_bitmap,
         last_gd->bg_inode_table);
}

void read_freebm()
{
  bm_inodes = malloc(sizeof(uint8_t) * group_cnt * blocksize);
  if (bm_inodes == NULL)
  {
    dump_error("malloc failed for bm_inodes", 2);
  }
  unsigned int i, j, k;
  uint8_t blockByte, inodeByte;
  int bitmask;
  for (i = 0; i < group_cnt; i++)
  {
    for (j = 0; j < blocksize; j++)
    {
      if (pread(img_fd, &blockByte, 1, (blocksize * groupdesc[i].bg_block_bitmap) + j) == -1)
      {
        dump_error("Could not read bg_block_bitmap from image", 2);
      }
      if (pread(img_fd, &inodeByte, 1, (blocksize * groupdesc[i].bg_inode_bitmap) + j) == -1)
      {
        dump_error("Could not read bg_inode_bitmap from image", 2);
      }
      bm_inodes[i + j] = inodeByte;
      bitmask = 1;
      for (k = 0; k < 8; k++)
      {
        if ((blockByte & bitmask) == 0)
          printf("BFREE,%u\n", i * superblock.s_blocks_per_group + j * 8 + k + 1);
        if ((inodeByte & bitmask) == 0)
          printf("IFREE,%u\n", i * superblock.s_inodes_per_group + j * 8 + k + 1);
        bitmask <<= 1;
      }
    }
  }
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
  read_groups();
  read_freebm();
  exit(0);
}
