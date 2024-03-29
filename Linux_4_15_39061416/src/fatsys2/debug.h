#ifndef _DEBUG_H
#define _DEBUG_H

#include "global.h"

//#define DBG_CACHE
//#define DBG_DIR
//#define DBG_DIRENT
//#define DBG_FAT
//#define DBG_FILE
#define DBG_INITFS

#define DBG 	printf
#define WARN	printf
#define ERR		printf
#define INFO 	printf

#define ASSERT(f)	if(!(f)) { \
						printf("ASSERT failed at %s():%d\n", __FUNCTION__, __LINE__); \
						exit(0); \
					}

void print_sector(ubyte * secbuf, ubyte bychar);
inline void nulldbg(char * fmt, ...);

#endif
