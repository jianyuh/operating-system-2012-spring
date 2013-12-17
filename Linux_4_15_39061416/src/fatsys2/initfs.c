#include "initfs.h"
#include "debug.h"
#include "fatfilesys.h"
#include "hai.h"
#include "fat.h"
#include "dir.h"

#ifndef DBG_INITFS
#undef DBG
#define DBG nulldbg
#endif

int32
FS_mount(
    IN  byte * dev,
    OUT fs_handle_t * phfs)
{
	int32 ret;
	fs_t * fs;
	boot_sector_t * pbs = NULL;
	tdev_handle_t hdev;
	tdir_t * proot_dir = NULL;
	tdir_t * pcur_dir = NULL;
	uint32 rootdir_clus;
	tfat_t * pfat = NULL;
	tcache_t * pcache = NULL;
	
	if (!dev || !phfs)
		return ERR_FS_INVALID_PARAM;

	ret = FS_OK;
	fs = (fs_t *)Malloc(sizeof(fs_t));
	Memset(fs, 0, sizeof(fs_t));
	pbs = (boot_sector_t *)Malloc(sizeof(boot_sector_t));
	Memset(pbs, 0, sizeof(boot_sector_t));
	
	ASSERT(sizeof(boot_sector_t) == 512);
	
	hdev = HAI_initdevice(dev, 512);
	if (hdev == NULL) {
		ret = ERR_FS_DEVICE_FAIL;
		goto _release;
	}
	fs->hdev = hdev;

	if (HAI_readsector(hdev, 0, (ubyte *)pbs) != HAI_OK) {
		ret = ERR_FS_DEVICE_FAIL;
		goto _release;
	}

	if (!_validate_bs(pbs)) {
		ret = ERR_FS_BAD_BOOTSECTOR;
		goto _release;
	}

	fs->pbs = pbs;
	
	_parse_boot_sector(pbs, fs);
	DBG("fat_type:%d\n", fs->fat_type);
	DBG(">sec_fat:%d\n", fs->sec_fat);
	DBG("sec_root_dir:%d\n", fs->sec_root_dir);
	DBG("sec_first_data:%d\n", fs->sec_first_data);
	
	if ((pfat = fat_init(fs)) == NULL) {
		ret = ERR_FS_BAD_FAT;
		goto _release;
	}
	fs->pfat = pfat;

	if ((pcache = cache_init(hdev, 32, fs->pbs->byts_per_sec)) == NULL) {
		WARN("FS: cache is disable.\n");
		pcache = cache_init(hdev, 0, fs->pbs->byts_per_sec);
	}
	fs->pcache = pcache;

	rootdir_clus = fs->fat_type == FT_FAT32 ? fs->pbs->bh32.root_clus : ROOT_DIR_CLUS_FAT16;

	if (dir_init_by_clus(fs, rootdir_clus, &proot_dir) != DIR_OK ||
		dir_init_by_clus(fs, rootdir_clus, &pcur_dir) != DIR_OK) {
		ret = ERR_FS_DEVICE_FAIL;
		goto _release;
	}

	fs->root_dir = proot_dir;
	fs->cur_dir = pcur_dir;

	*phfs = (fs_handle_t)fs;
	INFO("file system mount OK.\n");

	return ret;

_release:
	if (pfat)
		fat_destroy(pfat);
	if (proot_dir)
		dir_destroy(proot_dir);
	if (pcur_dir)
		dir_destroy(pcur_dir);
	if (HAI_closedevice(fs->hdev) != HAI_OK)
		ret = ERR_FS_DEVICE_FAIL;
	Free(pbs);
	Free(fs);

	return ret;
}

int32
FS_umount(
	IN	fs_handle_t  hfs)
{
	fs_t * pfs;
	int32 ret;

	if (!hfs)
		return ERR_FS_INVALID_PARAM;
	
	pfs = (fs_t *)hfs;
	ret = FS_OK;

	if (pfs->pbs)
		Free(pfs->pbs);

	if (pfs->pfat)
		fat_destroy(pfs->pfat);

	if (pfs->root_dir)
		dir_destroy(pfs->root_dir);
	
	if (pfs->cur_dir)
		dir_destroy(pfs->cur_dir);
	
	if (pfs->pcache)
		ret = cache_destroy(pfs->pcache);

	if (ret > 0 && (HAI_closedevice(pfs->hdev) != HAI_OK))
		ret = ERR_FS_DEVICE_FAIL;

	Free(pfs);

	return ret;
}

int32
_validate_bs(
	IN	boot_sector_t * pbs)
{
	INFO("=================Boot sector====================\n");
	INFO("oem_name: %s\n", pbs->oem_name);
	INFO("byts_per_sec: %d\n", pbs->byts_per_sec);
	INFO("sec_per_clus: %d\n", pbs->sec_per_clus);
	INFO("resvd_sec_cnt: %d\n", pbs->resvd_sec_cnt);
	INFO("================================================\n");
	return TRUE;
}

void
_parse_boot_sector(
	IN	boot_sector_t * pbs,
	OUT	fs_t * fs)
{
	uint32 totsec;
	uint32 count_of_clusters;
	uint32 datasec;
	
	fs->root_dir_sectors = ((pbs->root_ent_cnt * 32) + (pbs->byts_per_sec - 1)) /  pbs->byts_per_sec;

	if (pbs->fatsz16 != 0) {
		fs->fatsz = pbs->fatsz16;
	}
	else {
		fs->fatsz = pbs->bh32.fatsz32;
	}

	if (pbs->tot_sec16 != 0) {
		totsec = pbs->tot_sec16;
	}
	else {
		totsec = pbs->tot_sec32;
	}

	datasec = totsec - (pbs->resvd_sec_cnt + (pbs->num_fats * fs->fatsz) + fs->root_dir_sectors);
	count_of_clusters = datasec / pbs->sec_per_clus;
	fs->total_clusters = count_of_clusters;
	
	DBG("count_of_clusters = %d\n", count_of_clusters);

	//FIXME. I should find another to determine the fat type.
	if (count_of_clusters < 4085) {
		fs->fat_type = FT_FAT12;
	}
	else if (count_of_clusters < 65525 && pbs->fatsz16 != 0){
		fs->fat_type = FT_FAT16;
	}
	else {
		fs->fat_type = FT_FAT32;
	}

	fs->sec_fat = pbs->resvd_sec_cnt;
	fs->sec_root_dir = fs->sec_fat + pbs->num_fats * fs->fatsz;
	fs->sec_first_data = fs->sec_root_dir + fs->root_dir_sectors;
}

