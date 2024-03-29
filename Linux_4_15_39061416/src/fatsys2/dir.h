#ifndef _FS_DIR_H
#define _FS_DIR_H

#include "global.h"
#include "globalstruct.h"

/* Return value definitions. */
#define MAX_PATH_TOKEN			(64)
#define DIR_OK					(1)
#define ERR_DIR_INVALID_PATH	(-1)
#define ERR_DIR_INVALID_DEVICE	(-2)
#define ERR_DIR_NO_FREE_SPACE	(-3)
#define ERR_DIR_INITIALIZE_FAIL	(-4)

/* Interface declaration. */

int32
dir_init(
	IN	fs_t * pfs,
	IN	byte * path,
	OUT	tdir_t ** ppdir
);

int32
dir_init_by_clus(
	IN	fs_t * pfs,
	IN	uint32 clus,
	OUT	tdir_t ** ppdir
);

void
dir_destroy(
	IN	tdir_t * pdir
);

int32
dir_append_direntry(
	IN	tdir_t * pdir,
	IN	tdir_entry_t * pdir_entry
);

int32
dir_del_direntry(
	IN	tdir_t * pdir,
	IN	byte * fname
);

int32
dir_update_direntry(
	IN	tdir_t * pdir,
	IN	tdir_entry_t * pdir_entry
);

int32
dir_read_sector(
	IN	tdir_t * pdir
);

int32
dir_write_sector(
	IN	tdir_t * pdir
);

#endif
