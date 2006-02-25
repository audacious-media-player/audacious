#ifndef TAG_BMP_VFS_H
#define TAG_BMP_VFS_H 1
#include <libaudacious/vfs.h>
#define fopen(fp, set) vfs_fopen(fp, set); feof_ctr = 1
#define fclose(fp) vfs_fclose(fp); feof_ctr = 0
#define fread feof_ctr = vfs_fread
#define fwrite vfs_fwrite
#define fseek vfs_fseek
#define ftell vfs_ftell
#define feof(fp) feof_ctr == 0
#define ferror(fp) feof_ctr == 0
#define FILE VFSFile

/* BMP VFS does not have vfs_feof, so we have a makeshift method. */

static size_t feof_ctr;

#endif
