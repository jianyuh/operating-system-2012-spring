#include "fat.h"
#include "debug.h"

#ifndef DBG_FAT
#undef DBG
#define DBG nulldbg
#endif

/* Internal routines declaration. */

static uint32
_clus2fatsec(
	IN	tfat_t * pfat,
	IN	uint32 clus
);

static int32
_lookup_free_clus(
	IN	tfat_t * pfat,
	OUT	uint32 * pfree_clus
);

static uint32
_get_fat_entry(
	IN	tfat_t * pfat,
	IN	uint32 clus
);

static void
_set_fat_entry(
	IN	tfat_t * pfat,
	IN	uint32 clus,
	IN	uint32 entry_val
);

static uint16
_get_fat_entry_len(
	IN	tfat_t * pfat
);

static BOOL
_read_fat_sector(
	IN	tfat_t * pfat,
	IN	uint32 clus
);

static BOOL
_write_fat_sector(
	IN	tfat_t * pfat,
	IN	uint32 clus
);

static BOOL
_is_entry_free(
	IN	tfat_t * pfat,
	IN	uint32 entry_val
);

static BOOL
_is_entry_eof(
	IN	tfat_t * pfat,
	IN	uint32 entry_val
);

/*----------------------------------------------------------------------------------------------------*/

tfat_t *
fat_init(
	IN	fs_t * pfs)
{
	tfat_t * pfat;

	ASSERT(pfs);
	pfat = (tfat_t *)Malloc(sizeof(tfat_t));
	pfat->pfs = pfs;
    pfat->last_free_clus = 0;
    pfat->secbuf = (ubyte *)Malloc(pfs->pbs->byts_per_sec * 2);

	return pfat;
}

void
fat_destroy(
	IN	tfat_t * pfat)
{
	Free(pfat->secbuf);
	Free(pfat);
}

int32
fat_get_next_clus(
	IN	tfat_t * pfat,
	IN	uint32 clus)
{
	uint32 next_clus;

	ASSERT(pfat);
	if (!_read_fat_sector(pfat, _clus2fatsec(pfat, clus)))
		return ERR_FAT_DEVICE_FAIL;

	next_clus = _get_fat_entry(pfat, clus);
	if (_is_entry_eof(pfat, next_clus))
		return ERR_FAT_LAST_CLUS;

	return next_clus;
}

BOOL
fat_get_next_sec(
	IN	tfat_t * pfat,
	OUT	uint32 * pcur_clus,
	OUT uint32 * pcur_sec)
{
	fs_t * pfs = pfat->pfs;
	int32 next_clus;

	DBG("%s(): get next sector for cluster 0x%x\n", __FUNCTION__, *pcur_clus);
	if (*pcur_clus == ROOT_DIR_CLUS_FAT16 && 
		(pfs->fat_type == FT_FAT12 || pfs->fat_type == FT_FAT16)) {
		if (*pcur_sec + 1 < pfs->root_dir_sectors) {
			(*pcur_sec)++;
		}
		else {
			return FALSE;
		}
	}
	else if (*pcur_sec + 1 < pfs->pbs->sec_per_clus) {
		(*pcur_sec)++;
	}
	else {
		next_clus = fat_get_next_clus(pfat, *pcur_clus);
		if (next_clus == ERR_FAT_LAST_CLUS) {
			return FALSE;
		}
		DBG("%s(): next_clus = 0x%x\n", __FUNCTION__, next_clus);
		*pcur_clus = next_clus;
		*pcur_sec = 0;
	}

	return TRUE;
}

int32
fat_malloc_clus(
	IN	tfat_t * pfat,
	IN	uint32 cur_clus,
	OUT	uint32 * pnew_clus)
{
	uint32 new_clus;
	int32 ret;
	
	ASSERT(pfat && pnew_clus);

	if (cur_clus == FAT_INVALID_CLUS) {

		if ((ret = _lookup_free_clus(pfat, &new_clus)) != FAT_OK)
			return ret;
	}
	else {

		if ((ret = _lookup_free_clus(pfat, &new_clus)) != FAT_OK)
			return ret;

		if (!_read_fat_sector(pfat, _clus2fatsec(pfat, cur_clus)))
			return ERR_FAT_DEVICE_FAIL;

		_set_fat_entry(pfat, cur_clus, new_clus);

		if (!_write_fat_sector(pfat, _clus2fatsec(pfat, cur_clus)))
			return ERR_FAT_DEVICE_FAIL;
	}

	*pnew_clus = new_clus;
	DBG("%s(): new clus = %d\n", __FUNCTION__, new_clus);
	return FAT_OK;
}

int32
fat_free_clus(
	IN	tfat_t * pfat,
	IN	uint32 clus)
{
	uint32 last_clus;
	uint32 cur_clus;

	ASSERT(pfat);
	
	last_clus = clus;
	cur_clus = clus;

	if (!_read_fat_sector(pfat, _clus2fatsec(pfat, cur_clus)))
		return ERR_FAT_DEVICE_FAIL;

	while (1) {
		if (_clus2fatsec(pfat, cur_clus) != _clus2fatsec(pfat, last_clus)) {
			if (!_write_fat_sector(pfat, _clus2fatsec(pfat, last_clus)))
				return ERR_FAT_DEVICE_FAIL;
			if (!_read_fat_sector(pfat, _clus2fatsec(pfat, cur_clus)))
				return ERR_FAT_DEVICE_FAIL;
		}

		last_clus = cur_clus;
		cur_clus = _get_fat_entry(pfat, last_clus);
		if (_is_entry_eof(pfat, cur_clus)) {
			_set_fat_entry(pfat, last_clus, 0);
			break;
		}
		
		_set_fat_entry(pfat, last_clus, 0);
	}

	if (!_write_fat_sector(pfat, _clus2fatsec(pfat, last_clus)))
		return ERR_FAT_DEVICE_FAIL;

	return FAT_OK;
}

/*----------------------------------------------------------------------------------------------------*/

static uint32
_get_fat_entry(
	IN	tfat_t * pfat,
	IN	uint32 clus)
{
	fs_t * pfs = pfat->pfs;
	int16 entry_len;
	void * pclus;
	uint32 entry_val;

	entry_len = _get_fat_entry_len(pfat);
	pclus = pfat->secbuf + (clus * entry_len / 8) % pfs->pbs->byts_per_sec;
	
	if (pfs->fat_type == FT_FAT12) {
		uint16 fat_entry;

		fat_entry = *((uint16 *)pclus);
		if (clus & 1) {
			entry_val = fat_entry >> 4;
		}
		else {
			entry_val = fat_entry & 0x0FFF;
		}
	}
	else if (pfs->fat_type == FT_FAT16) {
		entry_val = *((uint16 *)pclus);
	}
	else if (pfs->fat_type == FT_FAT32) {
		entry_val = *((uint32 *)pclus) & 0x0FFFFFFF;
	}

	return entry_val;
}

static void
_set_fat_entry(
	IN	tfat_t * pfat,
	IN	uint32 clus,
	IN	uint32 entry_val)
{
	fs_t * pfs = pfat->pfs;
	int16 entry_len;
	void * pclus;

	entry_len = _get_fat_entry_len(pfat);
	pclus = pfat->secbuf + (clus * entry_len / 8) % pfs->pbs->byts_per_sec;
	
	if (pfs->fat_type == FT_FAT12) {
		uint16 fat12_entry_val = entry_val & 0xFFFF;

		if (clus & 1) {
			fat12_entry_val = fat12_entry_val << 4;
			*((uint16 *)pclus) = *((uint16 *)pclus) & 0x000F;
		}
		else {
			fat12_entry_val = fat12_entry_val & 0x0FFF;
			*((uint16 *)pclus) = *((uint16 *)pclus) & 0xF000;
		}
		*((uint16 *)pclus) = *((uint16 *)pclus) | fat12_entry_val;
	}
	else if (pfs->fat_type == FT_FAT16) {
		*((uint16 *)pclus) = entry_val & 0xFFFF;
	}
	else if (pfs->fat_type == FT_FAT32) {
		*((uint32 *)pclus) = *((uint32 *)pclus) & 0xF0000000;
		*((uint32 *)pclus) = *((uint32 *)pclus) | (entry_val & 0x0FFFFFFF);
	}
}

static uint16
_get_fat_entry_len(
	IN	tfat_t * pfat)
{
	fs_t * pfs = pfat->pfs;
	uint16 entry_len;

	switch (pfs->fat_type) {
	case FT_FAT12:
		entry_len = 12;
		break;
	case FT_FAT16:
		entry_len = 16;
		break;
	case FT_FAT32:
		entry_len = 32;
		break;
	default:
		ASSERT(0);
	}
	return entry_len;
}

static BOOL
_read_fat_sector(
	IN	tfat_t * pfat,
	IN	uint32 fat_sec)
{
	fs_t * pfs = pfat->pfs;

	//if (fat_sec == cur_fat_sec) 
	//	return TRUE;

	if (HAI_readsector(pfs->hdev, fat_sec, pfat->secbuf) != HAI_OK) {
		return FALSE;
	}

	if (pfs->fat_type == FT_FAT12) {

	    /* This cluster access spans a sector boundary in the FAT      */
   		/* There are a number of strategies to handling this. The      */
    	/* easiest is to always load FAT sectors into memory           */
    	/* in pairs if the volume is FAT12 (if you want to load        */
    	/* FAT sector N, you also load FAT sector N+1 immediately      */
    	/* following it in memory unless sector N is the last FAT      */
    	/* sector). It is assumed that this is the strategy used here  */
    	/* which makes this if test for a sector boundary span         */
    	/* unnecessary.                                                */
		if (HAI_readsector(pfs->hdev, fat_sec + 1, 
				pfat->secbuf + pfs->pbs->byts_per_sec) != HAI_OK) {
			return FALSE;
		}
	}

	pfat->cur_fat_sec = fat_sec;
	return TRUE;
}

static BOOL
_write_fat_sector(
	IN	tfat_t * pfat,
	IN	uint32 fat_sec)
{
	fs_t * pfs = pfat->pfs;

	if (HAI_writesector(pfs->hdev, fat_sec, pfat->secbuf) != HAI_OK) {
		return FALSE;
	}

	if (pfs->fat_type == FT_FAT12) {
		if (HAI_writesector(pfs->hdev, fat_sec + 1, 
				pfat->secbuf + pfs->pbs->byts_per_sec) != HAI_OK) {
			return FALSE;
		}
	}
	return TRUE;
}

static uint32
_clus2fatsec(
	IN	tfat_t * pfat,
	IN	uint32 clus)
{
	return (pfat->pfs->sec_fat + (clus * _get_fat_entry_len(pfat) / 8) / pfat->pfs->pbs->byts_per_sec);
}

static BOOL
_is_entry_free(
	IN	tfat_t * pfat,
	IN	uint32 entry_val)
{
	fs_t * pfs = pfat->pfs;

	if (pfs->fat_type == FT_FAT12) {
		return !(entry_val & 0x0FFF);
	}
	else if (pfs->fat_type == FT_FAT16) {
		return !(entry_val & 0xFFFF);
	}
	else if (pfs->fat_type == FT_FAT32) {
		return !(entry_val & 0x0FFFFFFF);
	}

	ASSERT(0);
	return FALSE;
}

static BOOL
_is_entry_eof(
	IN	tfat_t * pfat,
	IN	uint32 entry_val)
{
	fs_t * pfs = pfat->pfs;

	if (pfs->fat_type == FT_FAT12) {
		return (entry_val & 0x0FFF) > 0x0FF8;
	}
	else if (pfs->fat_type == FT_FAT16) {
		return (entry_val & 0xFFFF) > 0xFFF8;
	}
	else if (pfs->fat_type == FT_FAT32) {
		return (entry_val & 0x0FFFFFFF) > 0x0FFFFFF8;
	}

	ASSERT(0);
	return FALSE;
}

static int32
_lookup_free_clus(
	IN	tfat_t * pfat,
	OUT	uint32 * pfree_clus)
{
	fs_t * pfs = pfat->pfs;
	uint32 cur_clus;
	int32 ret;
	
	ret = FAT_OK;
	cur_clus = pfat->last_free_clus;
	if (!_read_fat_sector(pfat, _clus2fatsec(pfat,pfat->last_free_clus)))
		return ERR_FAT_DEVICE_FAIL;

	while (1) {
		uint32 entry_val;

		entry_val = _get_fat_entry(pfat, cur_clus);
		if (_is_entry_free(pfat, entry_val)) {

			_set_fat_entry(pfat, cur_clus, 0x0FFFFFFF);
			if (!_write_fat_sector(pfat, _clus2fatsec(pfat, cur_clus))) {
				ret = ERR_FAT_DEVICE_FAIL;
				break;
			}

			*pfree_clus = cur_clus;
			pfat->last_free_clus = cur_clus;
			break;
		}

		cur_clus++;
		if (cur_clus > pfs->total_clusters) {
			cur_clus = 0;
		}

		if (cur_clus == pfat->last_free_clus) {
			ret = ERR_FAT_NO_FREE_CLUSTER;
			break;
		}

		if (_clus2fatsec(pfat, cur_clus - 1) != _clus2fatsec(pfat, cur_clus)) {
			if (!_read_fat_sector(pfat, _clus2fatsec(pfat, cur_clus))) {
				ret = ERR_FAT_DEVICE_FAIL;
				break;
			}
		}
	}

	return ret;
}

