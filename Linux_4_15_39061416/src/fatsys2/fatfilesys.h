#ifndef _FS_H
#define _FS_H

#include "global.h"

/*
 * Structure definitions
 */
typedef struct {}* fs_handle_t;
typedef struct {}* tdir_handle_t;
typedef struct {}* tfile_handle_t;

#define DIR_ATTR_READ_ONLY  0x01
#define DIR_ATTR_HIDDEN     0x02
#define DIR_ATTR_SYSTEM     0x04
#define DIR_ATTR_VOLUME_ID  0x08
#define DIR_ATTR_DIRECTORY  0x10
#define DIR_ATTR_ARCHIVE    0x20

#define DNAME_MAX			64
#define DNAME_SHORT_MAX		13



typedef struct _fs_time {
	int32 year;
	int32 month;
	int32 day;
	int32 hour;
	int32 min;
	int32 sec;
}fs_time_t;

typedef struct _dirent {
	byte		d_name[DNAME_MAX];
	byte		d_name_short[DNAME_SHORT_MAX];
	ubyte		dir_attr;
	uint32		dir_file_size;
	fs_time_t crttime;
}dirent_t;

/*
 * Error code definitions
 */
#define FS_OK						(1)
#define ERR_FS_INVALID_PARAM 		(-1)
#define ERR_FS_DEVICE_FAIL 		(-2)
#define ERR_FS_BAD_BOOTSECTOR		(-3)
#define ERR_FS_BAD_FAT			(-4)
#define ERR_FS_INVALID_PATH		(-5)
#define ERR_FS_LAST_DIRENTRY		(-6)
#define ERR_FS_INVALID_OPENMODE	(-7)
#define ERR_FS_FILE_NOT_EXIST		(-8)
#define ERR_FS_FILE_OPEN_FAIL		(-9)
#define ERR_FS_NO_FREE_SPACE		(-10)
#define ERR_FS_READONLY			(-11)
#define ERR_FS_FILE_EOF			(-12)
#define ERR_FS_FAT				(-13)
#define ERR_FS_DIR_ALREADY_EXIST	(-14)
#define ERR_FS_INITIAL_DIR_FAIL	(-15)
#define ERR_FS_NO_SUCH_FILE		(-16)
#define ERR_FS_IS_NOT_A_FILE		(-17)
#define ERR_FS_REMOVE_FILE_FAIL	(-18)
#define ERR_FS_IS_NOT_A_DIRECTORY	(-19)
#define ERR_FS_NOT_EMPTY_DIR		(-20)
#define ERR_FS_REMOVE_DIR_FAIL	(-21)


/*
 * Interface
 */
int32
FS_mount(
	IN	byte * dev,
	OUT	fs_handle_t * phfs
);

int32
FS_opendir(
	IN	fs_handle_t hfs,
	IN	byte * path,
	OUT	tdir_handle_t * phdir
);

int32
FS_readdir(
	IN	tdir_handle_t hdir,
	OUT	dirent_t * pdirent
);

int32
FS_closedir(
	IN	tdir_handle_t hdir
);

int32 
FS_fopen(
	IN	fs_handle_t hfs,
	IN	byte * file_path,
	IN	byte * open_mode,
	OUT	tfile_handle_t * phfile
);

int32
FS_fclose(
	IN	tfile_handle_t hfile
);

int32
FS_fread(
	IN	tfile_handle_t hfile,
	IN	uint32 buflen,
	OUT	ubyte * ptr
);

int32
FS_fwrite(
	IN	tfile_handle_t hfile,
	IN	uint32 buflen,
	IN	ubyte * ptr
);

int32
FS_mkdir(
	IN	fs_handle_t hfs,
	IN	byte * dir_path
);

int32
FS_chdir(
	IN	fs_handle_t hfs,
	IN	byte * dir_path
);

int32
FS_rmdir(
	IN	fs_handle_t hfs,
	IN	byte * dir_path
);

int32
FS_rmfile(
	IN	fs_handle_t hfs,
	IN	byte * file_path
);

int32
FS_umount(
	IN	fs_handle_t  hfs
);

#endif

