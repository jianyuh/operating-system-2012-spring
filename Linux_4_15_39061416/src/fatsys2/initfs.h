#ifndef _INIT_FS_H
#define _INIT_FS_H

#include "global.h"
#include "globalstruct.h"

/*
 * Function prototype
 */
int32
_validate_bs(
	IN	boot_sector_t * pbs
);

void
_parse_boot_sector(
    IN  boot_sector_t * pbs,
	OUT	fs_t * fs
);


#endif
