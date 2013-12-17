#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <curses.h>
#include "fatfilesys.h"

void do_cmd(fs_handle_t hfs);
#define min(a, b) (a)<(b)?(a):(b);
#define MAX_PATH		64



void welcome()
{
	printf("\n//////////////////////////////////////////////\n");
	printf("\tWelcome to File System version 1.0\n");
	printf("\tAuthor:39061416\tHuang Jianyu\n");
	printf("\tWebsite:www.huangjy.info\n");
	printf("\tMobile Phone:15210965935\n");
	printf("\tEmail:hjyahead@gmail.com\n");
	printf("///////////////////////////////////////////////\n");
	printf("\n\n");
}


char * errmsg[] = {
	"Invalid parameters.",					//ERR_FS_INVALID_PARAM
	"Device access failed.",				//ERR_FS_DEVICE_FAIL
	"Bad boot sector.",						//ERR_FS_BAD_BOOTSECTOR
	"Bad fat table.",						//ERR_FS_BAD_FAT
	"Invalid path.",						//ERR_FS_INVALID_PATH
	"Got the last directory entry.",		//ERR_FS_LAST_DIRENTRY
	"Invalid open mode.",					//ERR_FS_INVALID_OPENMODE
	"File is not exist.",					//ERR_FS_FILE_NOT_EXIST
	"Open file failed.",					//ERR_FS_FILE_OPEN_FAIL
	"No free disk space.",					//ERR_FS_NO_FREE_SPACE 
	"Try to write a readonly file.",		//ERR_FS_READONLY
	"Reached the end of the file.", 		//ERR_FS_FILE_EOF
	"Access fat table error.",				//ERR_FS_FAT
	"Directory is already exist.",			//ERR_FS_DIR_ALREADY_EXIST
	"Initialize directory failed.",			//ERR_FS_INITIAL_DIR_FAIL
	"No such a file or directory.",			//ERR_FS_NO_SUCH_FILE
	"This is not a file.",					//ERR_FS_IS_NOT_A_FILE
	"Remove file failed.",					//ERR_FS_REMOVE_FILE_FAIL
	"This is not a directory.",				//ERR_FS_IS_NOT_A_DIRECTORY
	"This is not a empty directory.",		//ERR_FS_NOT_EMPTY_DIR
	"Remove directory failed."				//ERR_FS_REMOVE_DIR_FAIL
};

void
print_error(
	char * fs_func,
	int32 ret)
{
	printf("FS: <%s> failed for error message [%s]\n", fs_func, errmsg[-(ret + 1)]);
}

int
main(
	int argc,
	char *argv[])
{
	fs_handle_t hfs;
	int32 ret;
	
	welcome();

	if (argc < 2) {
		printf("Usage: ./filesys <image name>(e.g: ./filesys /dev/ram1)\n");
		return -1;
	}

	if ((ret = FS_mount(argv[1], &hfs)) != FS_OK) {
		print_error("FS_mount", ret);
		return -1;
	}

	do_cmd(hfs);

	if ((ret = FS_umount(hfs)) != FS_OK) {
		print_error("FS_umount", ret);
		return -1;
	}
	return 0;
}

#define MAX_CMD_LEN		64
#define MAX_ARG_NUM		3

typedef struct _tsh_session{
	char cur_dir[DNAME_MAX];
	fs_handle_t hfs;
	char * argv[MAX_ARG_NUM];
	char argc;
}tsh_session_t;

typedef void (*do_cmd_func)(tsh_session_t *);
typedef struct _cmd_handler{
	const char * cmd;
	const char * usage;
	const char * description;
	do_cmd_func handler;
}cmd_handler_t;

void do_ls(tsh_session_t * psession);
void do_pwd(tsh_session_t * psession);
void do_cd(tsh_session_t * psession);
void do_mkdir(tsh_session_t * psession);
void do_rm(tsh_session_t * psession);
void do_cat(tsh_session_t * psession);
void do_write(tsh_session_t * psession);
void do_help(tsh_session_t * psession);

void parse_cmd(char * cmd, tsh_session_t * psession);
void get_cmd(char * cmd);

cmd_handler_t chs[] = {
	{"ls", "ls <directory name>", "\tLists the contents of a directory.", do_ls},
	{"cd", "cd <directory name>", "\tChange the current directory to dir.", do_cd},
	{"mkdir", "mkdir <directory name>", "Make directories.", do_mkdir},
	{"rm", "rm [-r] <directory/file name>", "\tRemove directories or files.", do_rm},
	{"cat", "cat <file name>", "\tShow the file contents.", do_cat},
	{"cf", "cf <file name>", "\tWrite contents to the file.", do_write},
	{"help", "help", "\tShow help.", do_help},
	{"exit", "exit", "\tExit file system.", NULL}
};
int chs_num = sizeof(chs)/sizeof(chs[0]);

void
do_cmd(
	fs_handle_t hfs)
{
	tsh_session_t * psession;
	char cmd[MAX_CMD_LEN];

	psession = (tsh_session_t *)malloc(sizeof(tsh_session_t));
	strcpy(psession->cur_dir, "/");
	psession->hfs = hfs;

	while (1) {
		int ci;

		printf("%s:>", psession->cur_dir);
		memset(cmd, 0, MAX_CMD_LEN);
		get_cmd(cmd);
		parse_cmd(cmd, psession);

		if (psession->argc == 0)
			continue;

		for (ci = 0; ci < chs_num; ci++) {
			if (!strcmp(chs[ci].cmd, psession->argv[0])) {
				if (chs[ci].handler == NULL) {
					goto _end;
				}
				else {
					chs[ci].handler(psession);
					break;
				}
			}
		}

		if (ci == chs_num) {
			printf("Unrecognized command %s\n", cmd);
			do_help(psession);
		}
	}

_end:
	free(psession);
}

void
parse_cmd(
	char * cmd,
	tsh_session_t * psession)
{
	char * pcur;
	int argc;

	argc = 0;

	pcur = cmd;
	while (*pcur && argc < MAX_ARG_NUM) {
		psession->argv[argc] = pcur;
		while (*pcur && *pcur != ' ')
			pcur++;

		if (*pcur == ' ') {
			*pcur = '\0';
			pcur++;
		}

		while (*pcur && *pcur == ' ')
			pcur++;

		argc++;
	}

	psession->argc = min(argc, MAX_ARG_NUM);
}

void
get_cmd(
	char *cmd)
{
	int ci;
	
	ci = 0;
	while (ci < MAX_CMD_LEN - 1) {
		char c;

		c = getc(stdin);
		if (c == '\n') {
			break;
		}
		else {
			cmd[ci++] = c;
		}
	}
	
	cmd[ci] = '\0';
}

void
do_help(
	tsh_session_t * psession)
{
	int ci;

	printf("file system commands:\n");
	for (ci = 0; ci < chs_num; ci++) {
		printf("  %s:\t%s\n", chs[ci].cmd, chs[ci].description);
	}
}


void
_show_dirent(
	dirent_t * pdirent)
{
	printf("%8d byte", pdirent->dir_file_size);
	printf("\t%2d/%02d/%02d - %02d:%02d  ", pdirent->crttime.year,
		pdirent->crttime.month,
		pdirent->crttime.day,
		pdirent->crttime.hour,
		pdirent->crttime.min);

	if (pdirent->dir_attr & DIR_ATTR_DIRECTORY) {
		printf("\033[32m%s\033[0m", pdirent->d_name);
	}
	else {
		printf("%s", pdirent->d_name);
	}
	printf("\n");
}

void
do_ls(
	tsh_session_t * psession)
{
	int32 ret;
	fs_handle_t hfs;
	tdir_handle_t hdir;
	char dir[MAX_PATH];
	int file_num;

	if (psession->argc > 2 || psession->argc <= 0) {
		printf("Usage: ls <directory name>\n");
		return;
	}

	strcpy(dir, psession->cur_dir);
	if (psession->argc == 2) {
		strcat(dir, psession->argv[1]);
	}

	hfs = psession->hfs;
	if ((ret = FS_opendir(hfs, dir, &hdir)) != FS_OK) {
		print_error("FS_opendir", ret);
		return;
	}

	file_num = 0;
	while (1) {
		dirent_t dirent;

		if ((ret = FS_readdir(hdir, &dirent)) == FS_OK) {
			_show_dirent(&dirent);
		}
		else if (ret == ERR_FS_LAST_DIRENTRY) {
			break;
		}
		else {
			print_error("FS_readdir", ret);
			break;
		}
		file_num++;
	}

	printf("\nTotal %d files.\n", file_num);
	if ((ret = FS_closedir(hdir)) != FS_OK) {
		print_error("FS_closedir", ret);
		return;
	}

	return;
}

void
_del_token(
	char * dir)
{
	char * pcur;

	if (!strlen(dir) || !strcmp(dir, "/"))
		return;

	pcur = dir + strlen(dir) - 1;

	if (*pcur == '/')
		pcur--;

	while (*pcur && *pcur != '/') {
		pcur--;
	}

	*pcur = '\0';
}

void
do_cd(
	tsh_session_t * psession)
{
	int32 ret;
	fs_handle_t hfs;
	char dir[MAX_PATH];

	if (psession->argc > 2 || psession->argc <= 0) {
		printf("Usage: cd <directory name>\n");
		return;
	}

	if (psession->argc == 2) {
		strcpy(dir, psession->cur_dir);

		if (!strcmp(psession->argv[1], "..")) {
			_del_token(dir);
		}
		else if (strcmp(psession->argv[1], ".")) {
			strcat(dir, psession->argv[1]);
		}

		if (*(dir + strlen(dir) - 1) != '/') {
			strcat(dir, "/");
		}
	}
	else {
		strcpy(dir, "/");
	}

	hfs = psession->hfs;
	if ((ret = FS_chdir(hfs, dir)) != FS_OK) {
		print_error("FS_chdir", ret);
		return;
	}

	strcpy(psession->cur_dir, dir);
}

void
do_mkdir(
	tsh_session_t * psession)
{
	int32 ret;

	if (psession->argc != 2) {
		printf("Usage: mkdir <directory name>\n");
		return;
	}

	if ((ret = FS_mkdir(psession->hfs, psession->argv[1])) != FS_OK) {
		print_error("FS_mkdir", ret);
		return;
	}
}

void
do_rm(
	tsh_session_t * psession)
{
	int32 ret;
	char dir[MAX_PATH];

	if (psession->argc == 3 && !strcmp(psession->argv[1], "-r")) {
		strcpy(dir, psession->argv[2]);
		if ((ret = FS_rmdir(psession->hfs, dir)) != FS_OK) {
			print_error("FS_rmdir", ret);
			return;
		}
	}
	else if (psession->argc == 2){
		strcpy(dir, psession->argv[1]);
		if ((ret = FS_rmfile(psession->hfs, dir)) != FS_OK) {
			print_error("FS_rmfile", ret);
			return;
		}
	}
	else {
		printf("Usage: rm [-r] <directory/file name>\n");
		return;
	}
}

void
do_cat(
	tsh_session_t * psession)
{
#define BUF_SIZE	1024
	int32 ret;
	tfile_handle_t hfile;
	char dir[MAX_PATH];
	char *buf;
	int32 all_size;
	
	if (psession->argc != 2) {
		printf("Usage: cat <file name>\n");
		return;
	}

	strcpy(dir, psession->argv[1]);
	buf = (char *)malloc(BUF_SIZE);
	all_size = 0;

	if ((ret = FS_fopen(psession->hfs, dir, "r", &hfile)) != FS_OK) {
		print_error("FS_fopen", ret);
		return;
	}

	while (1) {
		memset(buf, 0, BUF_SIZE);

		if ((ret = FS_fread(hfile, BUF_SIZE, (unsigned char *)buf)) < 0) {
			if (ret == ERR_FS_FILE_EOF) {
				break;
			}
			else {
				print_error("FS_fread", ret);
				break;
			}
		}

		printf("%s", buf);
		all_size += ret;
	}
	printf("\n============================================================\n");
	printf("[%s] total size %d bytes\n", dir, all_size);

	if ((ret = FS_fclose(hfile)) != FS_OK) {
		print_error("FS_fclose", ret);
		return;
	}

	free(buf);

#undef BUF_SIZE
}

char * 
_get_input()
{
#define BUF_SIZE	4096
	char * buf;
	int ci;

	buf = (char *)malloc(BUF_SIZE);
	ci = 0;

    while (ci < BUF_SIZE - 1) {
        char c;

        c = getc(stdin);
        if (c == -1) {
            break;
        }
        else {
            buf[ci++] = c;
        }
    }

	return buf;
#undef BUF_SIZE
}

void
do_write(
	tsh_session_t * psession)
{
	int32 ret;
	char dir[MAX_PATH];
	tfile_handle_t hfile;
	char openmode[2];
	char * buf;

	if (psession->argc == 3 && !strcmp(psession->argv[1], "-a")) {
		strcpy(openmode, "a");
		strcpy(dir, psession->argv[2]);
	}
	else if (psession->argc == 2) {
		strcpy(openmode, "w");
		strcpy(dir, psession->argv[1]);
	}
	else {
		printf("Usage: write [-a] <file name>\n");
		return;
	}

	buf = _get_input();

	if ((ret = FS_fopen(psession->hfs, dir, openmode, &hfile)) != FS_OK) {
		print_error("FS_fopen", ret);
		return;
	}

	ret = FS_fwrite(hfile, strlen(buf), (unsigned char *)buf);
	if (ret < 0 || ret != strlen(buf)) {
		print_error("FS_fwrite", ret);
		return;
	}

	if ((ret = FS_fclose(hfile)) != FS_OK) {
		print_error("FS_fclose", ret);
		return;
	}

	printf("\n============================================================\n");
	printf("wrote %d bytes to file %s ok.\n", strlen(buf), dir);
	free(buf);
}

