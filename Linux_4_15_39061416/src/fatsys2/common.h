
#ifndef _COMMON_H
#define _COMMON_H

#include "global.h"
#include "globalstruct.h"

uint32
clus2sec(
	IN	fs_t * pfs,
	IN	uint32 clus
);

uint32
tokenize(
	IN	byte * string,
	IN	byte sep,
	OUT	byte * tokens[]
);

uint32
copy_from_unicode(
	IN	uint16 * psrc,
	IN	uint32 len,
	OUT	byte * pdst
);

uint32
copy_to_unicode(
	IN	ubyte * psrc,
	IN	uint32 len,
	OUT	uint16 * pdst
);

byte *
dup_string(
	IN	byte * pstr
);

void
trip_blanks(
	IN	byte * pstr
);

BOOL
divide_path(
	IN	byte * file_path,
	OUT	byte * pfname
);

#define min(a, b) (a)<(b)?(a):(b)

#endif
