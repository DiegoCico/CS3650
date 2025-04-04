#ifndef STAT_H
#define STAT_H

#define T_DIR  1   // Directory
#define T_FILE 2   // File
#define T_DEV  3   // Device

struct stat {
  short type;
  int dev;
  uint ino;
  short nlink;
  uint size;
};

struct iostats {
  uint read_bytes;  
  uint write_bytes; 
};

#endif
