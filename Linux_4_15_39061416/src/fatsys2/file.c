#include "file.h"
#include "fatfilesys.h"
#include "common.h"
#include "debug.h"
#include "dir.h"
#include "dirent.h"
#include "fat.h"

#ifndef DBG_FILE
#undef DBG
#define DBG nulldbg
#endif

/* private function declaration. */

static BOOL
_parse_open_mode(
	IN	byte * open_mode,
	OUT	uint32 * popen_mode
);

static void
_file_seek(
	IN	tfile_t * pfile,
	IN	uint32 offset
);

static int32
_initialize_file(
	IN	fs_t * pfs,
	IN	tdir_t * pdir,
	IN	tdir_entry_t * pdir_entry,
	OUT	tfile_t * pfile
);

static int32
_get_next_sec(
	IN	tfile_t * pfile
);

/*----------------------------------------------------------------------------------------------------*/

int32 
FS_fopen(
	IN	fs_handle_t hfs,
	IN	byte * file_path,
	IN	byte * open_mode,
	OUT	tfile_handle_t * phfile)
{
	int32 ret;
	fs_t * pfs = (fs_t *)hfs;
	byte * fname, * path;
	byte * dup_file_path;
	tfile_t * pfile;
	tdir_t * pdir;
	tdir_entry_t * pdir_entry;

	if (!hfs || !file_path || !open_mode || !phfile)
		return ERR_FS_INVALID_PARAM;

	ret = FS_OK;
	pfile = (tfile_t *)Malloc(sizeof(tfile_t));
	dup_file_path = dup_string(file_path);
	pdir_entry = dirent_malloc();
	fname = (byte *)Malloc(DNAME_MAX);
	pfile->secbuf = (ubyte *)Malloc(pfs->pbs->byts_per_sec);
	Memset(pfile->secbuf, 0, pfs->pbs->byts_per_sec);

	path = dup_file_path;
	if (!divide_path(dup_file_path, fname)) {
		ret = ERR_FS_INVALID_PATH;
		goto _release;
	}
	
	if (!_parse_open_mode(open_mode, &pfile->open_mode)) {
		ret = ERR_FS_INVALID_OPENMODE;
		goto _release;
	}

	if ((dir_init(pfs, path, &pdir)) != DIR_OK) {
		ret = ERR_FS_INVALID_PATH;
		goto _release;
	}

	if (dirent_find(pdir, fname, pdir_entry) != DIRENTRY_OK) {

		DBG("%s(): can't find file [%s] at [%s]\n", __FUNCTION__, fname, path);
		if (pfile->open_mode == OPENMODE_READONLY) {
			ret = ERR_FS_FILE_NOT_EXIST;
			goto _release;
		}
		
		if (!dirent_init(fname, 0, TRUE, pdir_entry)) {
			ret = ERR_FS_INVALID_PATH;
			goto _release;
		}

		if ((ret = dir_append_direntry(pdir, pdir_entry)) != DIR_OK) {
			if (ret == ERR_DIR_NO_FREE_SPACE) {
				ret = ERR_FS_NO_FREE_SPACE;
			}
			else {
				ret = ERR_FS_DEVICE_FAIL;
			}
			goto _release;
		}
	}
	
	ret = _initialize_file(pfs, pdir, pdir_entry, pfile);
	if (ret == FILE_OK) {
		*phfile = (tfile_handle_t)pfile;
	}

_release:
	Free(fname);
	Free(dup_file_path);
	return ret;
}

int32
FS_rmfile(
	IN	fs_handle_t hfs,
	IN	byte * file_path)
{
	int32 ret;
    byte * fname, * path;
    byte * dup_file_path;
    tdir_t * pdir;
    fs_t * pfs;
	tdir_entry_t * pdir_entry;

	ret = FS_OK;

    if (!hfs || !file_path)
        return ERR_FS_INVALID_PARAM;
	
    pfs = (fs_t *)hfs;
    dup_file_path = dup_string(file_path);
    fname = (byte *)Malloc(DNAME_MAX);
	pdir_entry = dirent_malloc();

    path = dup_file_path;
    if (!divide_path(dup_file_path, fname)) {
        ret = ERR_FS_INVALID_PATH;
        goto _release;
    }

    if ((dir_init(pfs, path, &pdir)) != DIR_OK) {
        ret = ERR_FS_INVALID_PATH;
        goto _release;
    }

	if ((ret = dirent_find(pdir, fname, pdir_entry)) == DIRENTRY_OK) {
		if (dirent_get_dir_attr(pdir_entry) & ATTR_DIRECTORY) {
			ret = ERR_FS_IS_NOT_A_FILE;
			goto _release;
		}

		if (dir_del_direntry(pdir, fname) != DIR_OK) {
			ret = ERR_FS_REMOVE_FILE_FAIL;
			goto _release;
		}

		if (fat_free_clus(pdir->pfs->pfat, dirent_get_clus(pdir_entry)) != FAT_OK) {
			ret = ERR_FS_REMOVE_FILE_FAIL;
			goto _release;
		}
		
		ret = FS_OK;
	}
	else {
		ret = ERR_FS_NO_SUCH_FILE;
	    goto _release;
    }

_release:
	Free(fname);
	Free(dup_file_path);
	dirent_release(pdir_entry);
	dir_destroy(pdir);
	return ret;
}

int32
FS_fwrite(
	IN	tfile_handle_t hfile,
	IN	uint32 buflen,
	IN	ubyte * ptr)
{
	tfile_t * pfile = (tfile_t *)hfile;
	tdir_entry_t * pdir_entry;
	tdir_t * pdir;
	uint32 write_size;
	uint32 written_size;
	uint32 file_size;
	int32 ret;

	if (!hfile || !ptr)
		return ERR_FS_INVALID_PARAM;

	if (pfile->open_mode == OPENMODE_READONLY)
		return ERR_FS_READONLY;

	pdir = pfile->pdir;
	pdir_entry = pfile->pdir_entry;
	file_size = dirent_get_file_size(pdir_entry);
	write_size = buflen;
	written_size = 0;

	while (written_size < buflen) {
		uint32 write_once_size;

		write_once_size = min(pfile->pfs->pbs->byts_per_sec - pfile->cur_sec_offset, 
			buflen - written_size);

		Memcpy(pfile->secbuf + pfile->cur_sec_offset, ptr + written_size,
			write_once_size);
		written_size += write_once_size;
		pfile->cur_sec_offset += write_once_size;

		if (pfile->cur_sec_offset == pfile->pfs->pbs->byts_per_sec) {
			ret = file_write_sector(pfile);
			if (ret == FILE_OK) {
				if (buflen - written_size > 0) {
					if ((ret = _get_next_sec(pfile)) != FILE_OK) {
						ERR("%s(): get next sector failed at %d with ret = %d\n", __FUNCTION__, __LINE__, ret);
						break;
					}
				}
				pfile->cur_sec_offset = 0;
			}
			else if (ret == ERR_FILE_NO_FREE_CLUSTER) {
				ret = ERR_FS_NO_FREE_SPACE;
				break;
			}
			else {
				ret = ERR_FS_DEVICE_FAIL;
				break;
			}
		}
	}

	if (written_size > 0) {
		dirent_update_wrt_time(pdir_entry);
		dirent_update_wrt_date(pdir_entry);
		pfile->file_size += written_size;
	}

	return written_size;
}

int32
FS_fread(
	IN	tfile_handle_t hfile,
	IN	uint32 buflen,
	OUT	ubyte * ptr)
{
	tfile_t * pfile = (tfile_t *)hfile;
	tdir_entry_t * pdir_entry;
	tdir_t * pdir;
	uint32 read_size;
	uint32 readin_size;
	uint32 file_size;
	int32 ret;

	if (!hfile || !ptr)
		return ERR_FS_INVALID_PARAM;

	pdir = pfile->pdir;
	pdir_entry = pfile->pdir_entry;
	file_size = dirent_get_file_size(pdir_entry);
	read_size = min(buflen, file_size - pfile->cur_fp_offset);
	readin_size = 0;

	while (readin_size < read_size) {

		if (pfile->cur_sec_offset + (read_size - readin_size) >= pfile->pfs->pbs->byts_per_sec) {

			Memcpy(ptr + readin_size, pfile->secbuf + pfile->cur_sec_offset, 
				pfile->pfs->pbs->byts_per_sec - pfile->cur_sec_offset);
			readin_size += pfile->pfs->pbs->byts_per_sec - pfile->cur_sec_offset;

			ret = file_read_sector(pfile); 
			if (ret == FILE_OK) {
				pfile->cur_sec_offset = 0;
				continue;
			}
			else if (ret == ERR_FILE_EOF) {
				//WARN("%s(): unexpect file end at %d\n", __FUNCTION__, __LINE__);
				break;
			}
			else {
				ERR("%s(): read file data sector failed at %d\n", __FUNCTION__, __LINE__);
				ret = ERR_FS_DEVICE_FAIL;
				goto _end;
			}
		}
		else {
			Memcpy(ptr + readin_size, pfile->secbuf + pfile->cur_sec_offset,
				read_size - readin_size);
			pfile->cur_sec_offset += (read_size - readin_size);
			readin_size += (read_size - readin_size);
		}
	}

	if (readin_size > 0) {
		dirent_update_lst_acc_date(pdir_entry);
		pfile->cur_fp_offset += readin_size;
		ret = readin_size;
	}
	else {
		ret = ERR_FS_FILE_EOF;
	}

_end:
	return ret;
}

int32
FS_fclose(
	IN	tfile_handle_t hfile)
{
	int32 ret;
	tfile_t * pfile = (tfile_t *)hfile;

	if (!hfile)
		return ERR_FS_INVALID_PARAM;

	ret = FS_OK;
	if (file_write_sector(pfile) != FILE_OK) {
		ret = ERR_FS_DEVICE_FAIL;
	}

	dirent_set_file_size(pfile->pdir_entry, pfile->file_size);
	if (dir_update_direntry(pfile->pdir, pfile->pdir_entry) != DIR_OK) {
		ret = ERR_FS_DEVICE_FAIL;
	}

	dir_destroy(pfile->pdir);
	dirent_release(pfile->pdir_entry);
	Free(pfile->secbuf);
	Free(pfile);

	return ret;
}

/*----------------------------------------------------------------------------------------------------*/

int32
file_read_sector(
	IN	tfile_t * pfile)
{
	int32 ret;
	fs_t * pfs = pfile->pfs;
	
	if (pfile->cur_clus == 0) {
		ret = ERR_FILE_EOF;
	}
	else {
		if (fat_get_next_sec(pfs->pfat, &pfile->cur_clus, &pfile->cur_sec)) {
			if (cache_readsector(pfs->pcache, clus2sec(pfs, pfile->cur_clus) + pfile->cur_sec, 
					pfile->secbuf) == CACHE_OK) {
				ret = FILE_OK;
			}
			else {
				ret = ERR_FILE_DEVICE_FAIL;
			}
		}
		else {
			ret = ERR_FILE_EOF;
		}
	}

	return ret;
}

int32
file_write_sector(
	IN	tfile_t * pfile)
{
	int32 ret;
	fs_t * pfs = pfile->pfs;
	tdir_entry_t * pdir_entry = pfile->pdir_entry;
	uint32 new_clus;
	
	ret = FILE_OK;

	if (pfile->cur_clus == 0) {
		int32 fatret;

		if ((fatret = fat_malloc_clus(pfs->pfat, FAT_INVALID_CLUS, &new_clus)) == FAT_OK) {
			DBG("%s: new_clus = %d\n", __FUNCTION__, new_clus);
			dirent_set_clus(pdir_entry, new_clus);
			pfile->cur_clus = new_clus;
			pfile->cur_sec = 0;
		}
		else if (fatret == ERR_FAT_NO_FREE_CLUSTER) {
			ret = ERR_FILE_NO_FREE_CLUSTER;
		}
		else {
			ret = ERR_FILE_DEVICE_FAIL;
		}
	}

	if (ret == FILE_OK) {
		if (cache_writesector(pfs->pcache, clus2sec(pfs, pfile->cur_clus) + pfile->cur_sec,
				pfile->secbuf) != CACHE_OK) {
			ret = ERR_FILE_DEVICE_FAIL;
		}
	}

	return ret;
}

/*----------------------------------------------------------------------------------------------------*/

static int32
_initialize_file(
	IN	fs_t * pfs,
	IN	tdir_t * pdir,
	IN	tdir_entry_t * pdir_entry,
	OUT	tfile_t * pfile)
{
	int32 ret;

	pfile->pfs = pfs;
	pfile->pdir = pdir;
	pfile->pdir_entry = pdir_entry;
	ret = FILE_OK;

	pfile->start_clus = dirent_get_clus(pdir_entry);
	pfile->file_size = dirent_get_file_size(pdir_entry);
	pfile->cur_clus = pfile->start_clus;
	pfile->cur_sec = 0;
	pfile->cur_sec_offset = 0;
	pfile->cur_fp_offset = 0;

	if (pfile->open_mode == OPENMODE_APPEND) {
		pfile->cur_fp_offset = dirent_get_file_size(pdir_entry);

		_file_seek(pfile, pfile->cur_fp_offset);
		if (pfile->cur_clus != 0) {
			if (cache_readsector(pfs->pcache, clus2sec(pfs, pfile->cur_clus) + pfile->cur_sec, 
					pfile->secbuf) != CACHE_OK) {
				ret = ERR_FS_DEVICE_FAIL;
			}
		}
	}
	else if (pfile->open_mode == OPENMODE_WRITE) {
		if (pfile->start_clus != 0) {
			if (fat_free_clus(pfs->pfat, pfile->start_clus) != FAT_OK) {
				ERR("%s(): %d fat_free_clus failed.\n", __FUNCTION__, __LINE__);
				ret = ERR_FS_FAT;
			}
		}

		dirent_set_file_size(pdir_entry , 0);
		dirent_set_clus(pdir_entry, 0);

		pfile->file_size = 0;
		pfile->cur_clus = 0;
		pfile->cur_sec = 0;
		pfile->cur_sec_offset = 0;
		pfile->cur_fp_offset = 0;
	}
	else {
		if (pfile->cur_clus != 0) {
			if (cache_readsector(pfs->pcache, clus2sec(pfs, pfile->cur_clus) + pfile->cur_sec, 
					pfile->secbuf) != CACHE_OK) {
				ret = ERR_FS_DEVICE_FAIL;
			}
		}
	}

	return ret;
}

static void
_file_seek(
	IN	tfile_t * pfile,
	IN	uint32 offset)
{
	int32 cur_offset;

	cur_offset = offset;
	while (cur_offset - pfile->pfs->pbs->byts_per_sec > 0 &&
		fat_get_next_sec(pfile->pfs->pfat, &pfile->cur_clus, &pfile->cur_sec)) {
		cur_offset -= pfile->pfs->pbs->byts_per_sec;
	}
	pfile->cur_sec_offset = cur_offset;
}

static int32
_get_next_sec(
	IN	tfile_t * pfile)
{
	int32 ret;
	fs_t * pfs = pfile->pfs;
	uint32 new_clus;

	ret = FILE_OK;

	if (pfile->cur_sec + 1 < pfs->pbs->sec_per_clus) {
		pfile->cur_sec++;
	}
	else {
		int32 fatret;

		if ((fatret = fat_malloc_clus(pfs->pfat, pfile->cur_clus, &new_clus)) == FAT_OK) {
			pfile->cur_clus = new_clus;
			pfile->cur_sec = 0;
		}
		else if (fatret == ERR_FAT_NO_FREE_CLUSTER) {
			ret = ERR_FILE_NO_FREE_CLUSTER;
		}
		else {
			ret = ERR_FILE_DEVICE_FAIL;
		}
	}

	return ret;
}

static BOOL
_parse_open_mode(
	IN	byte * open_mode,
	OUT	uint32 * popen_mode)
{
	if (!Strcmp(open_mode, "r")) {
		*popen_mode = OPENMODE_READONLY;
	}
	else if (!Strcmp(open_mode, "w")) {
		*popen_mode = OPENMODE_WRITE;
	}
	else if (!Strcmp(open_mode, "a")) {
		*popen_mode = OPENMODE_APPEND;
	}
	else {
		return FALSE;
	}

	return TRUE;
}

