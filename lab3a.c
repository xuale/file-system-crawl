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
  groupdesc = malloc(sizeof(struct ext2_group_desc));

  int c = pread(img_fd, groupdesc, sizeof(struct ext2_group_desc), blocksize + 1024);
  if (c < 0)
  {
    dump_error("Could not load image", 2);
  }

  printf("GROUP,%u,%u,%u,%u,%u,%u,%u,%u\n",
         0,
         superblock.s_blocks_count,
         superblock.s_inodes_count,
         groupdesc->bg_free_blocks_count,
         groupdesc->bg_free_inodes_count,
         groupdesc->bg_block_bitmap,
         groupdesc->bg_inode_bitmap,
         groupdesc->bg_inode_table);
}

void read_freebm()
{
  bm_inodes = malloc(sizeof(uint8_t) * blocksize);
  if (bm_inodes == NULL)
  {
    dump_error("malloc failed for bm_inodes", 2);
  }
  unsigned int i, j;
  uint8_t blockByte, inodeByte;
  int bitmask;

  for (i = 0; i < blocksize; i++)
  {
    if (pread(img_fd, &blockByte, 1, (blocksize * groupdesc->bg_block_bitmap) + i) == -1)
    {
      dump_error("Could not read bg_block_bitmap from image", 2);
    }
    if (pread(img_fd, &inodeByte, 1, (blocksize * groupdesc->bg_inode_bitmap) + i) == -1)
    {
      dump_error("Could not read bg_inode_bitmap from image", 2);
    }
    bm_inodes[i] = inodeByte;
    bitmask = 1;
    for (j = 0; j < 8; j++)
    {
      if ((blockByte & bitmask) == 0)
        printf("BFREE,%u\n", i * 8 + j + 1);
      if ((inodeByte & bitmask) == 0)
        printf("IFREE,%u\n", i * 8 + j + 1);
      bitmask <<= 1;
    }
  }
}

void getTime(uint32_t sec, char *buf)
{
  time_t ticks = (time_t)sec;
  struct tm *timeInfo = gmtime(&ticks);
  strftime(buf, 18, "%m/%d/%y %H:%M:%S", timeInfo);
}

void read_inodes()
{
  unsigned int i, j;
  struct ext2_inode inode;
  for (i = 0; i < superblock.s_inodes_count; i++)
  {
    if (pread(img_fd, &inode, sizeof(struct ext2_inode), 1024 + (blocksize * (groupdesc->bg_inode_table) - 1) + (i * sizeof(struct ext2_inode))) == -1)
    {
      dump_error("Could not read bg_inode_bitmap from image", 2);
    }
    if (!inode.i_mode || !inode.i_links_count)
    {
      continue;
    }
    char fileType = '?';
    if (inode.i_mode & 0x4000)
    {
      fileType = 'd';
    }
    else if (inode.i_mode & 0x8000)
    {
      fileType = 'f';
    }
    else if (inode.i_mode & 0xA000)
    {
      fileType = 's';
    }
    char cTime[18];
    getTime(inode.i_ctime, cTime);
    char mTime[18];
    getTime(inode.i_mtime, mTime);
    char aTime[18];
    getTime(inode.i_atime, aTime);

    printf("INODE,%d,%c,%o,%u,%u,%u,%s,%s,%s,%u,%u\n",
           i + 1,
           fileType,
           inode.i_mode & 4095,
           inode.i_uid,
           inode.i_gid,
           inode.i_links_count,
           cTime,
           mTime,
           aTime,
           inode.i_size,
           inode.i_blocks);
    for (j = 0; j < 14; j++)
    {
      printf("%d,", inode.i_block[j]);
    }
    printf("%d\n", inode.i_block[14]);
    if (fileType == 'd' || fileType == 'f')
    {
      // call recursive function...
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
  read_inodes();
  exit(0);
}