#ifndef _FS_FAT_H
#define _FS_FAT_H

#include "global.h"
#include "globalstruct.h"

/* Return code definitions. */
#define FAT_OK						(1)
#define ERR_FAT_LAST_CLUS			(-1)
#define ERR_FAT_DEVICE_FAIL			(-2)
#define ERR_FAT_NO_FREE_CLUSTER		(-3)


/* Public interface. */
tfat_t *
fat_init(
	IN	fs_t * pfs
);

void
fat_destroy(
	IN	tfat_t * pfat
);

/*
 * Return:
 *	FAT_OK
 *	ERR_FAT_DEVICE_FAIL
 *	ERR_FAT_LAST_CLUS
 */
int32
fat_get_next_clus(
	IN	tfat_t * pfat,
	IN	uint32 clus
);

/*
 * Return:
 *	TRUE
 *	FALSE
 */
BOOL
fat_get_next_sec(
	IN	tfat_t * pfat,
	OUT	uint32 * pcur_clus,
	OUT uint32 * pcur_sec
);

/*
 * Return:
 *	FAT_OK
 *	ERR_FAT_DEVICE_FAIL
 *	ERR_FAT_NO_FREE_CLUSTER
 */
int32
fat_malloc_clus(
	IN	tfat_t * pfat,
	IN	uint32 cur_clus,
	OUT	uint32 * pnew_clus
);

int32
fat_free_clus(
	IN	tfat_t * pfat,
	IN	uint32 clus
);

#endif
