#ifndef _FS_FILE_H
#define _FS_FILE_H

#include "global.h"
#include "globalstruct.h"

#define FILE_OK						(1)
#define ERR_FS_INVALID_PARAM		(-1)
#define ERR_FILE_DEVICE_FAIL		(-2)
#define ERR_FILE_EOF				(-3)
#define ERR_FILE_NO_FREE_CLUSTER	(-4)

int32
file_read_sector(
	IN	tfile_t * pfile
);

int32
file_write_sector(
	IN	tfile_t * pfile
);

#endif
