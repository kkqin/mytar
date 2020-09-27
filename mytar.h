#ifndef _MYTAR_H_
#define _MYTAR_H_

#define _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE 
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#ifdef __linux__
	#include <unistd.h>
#elif WIN32
	#include <direct.h>
	#include <dirent.h>
	#include <io.h>
	#include <process.h>
	#define ssize_t unsigned int 
#endif

#include <string.h>

#define  lf_oldnormal '\0'       /* normal disk file, unix compatible */
#define  lf_normal    '0'        /* normal disk file */
#define  lf_link      '1'        /* link to previously dumped file */
#define  lf_symlink   '2'        /* symbolic link */
#define  lf_chr       '3'        /* character special file */
#define  lf_blk       '4'        /* block special file */
#define  lf_dir       '5'        /* directory */
#define  lf_fifo      '6'        /* fifo special file */
#define  lf_contig    '7'        /* contiguous file */

#include "picohash.h"

enum {
	 HEAD = 1,
	 BODY 
};

#pragma pack(1) 
typedef struct _tar_head {
	union {
		struct {
			char name[100];	
			char mode[8];
			char uid[8];
			char gid[8];
			char size[12];
			char mtime[12];
			char chksum[8];
			char type;
			char link_name[100];
			char ustar[8];
			char owner[32];
			char group[32];
			char major[32];
			char minor[32];
		};
		char block[512];
	};

	int itype;
	int end;
	struct _tar_head* next;
} TAR_HEAD;

// 8 ����ת 10����
unsigned int oct2uint(const char* src, int read_size);
// �ļ�����
int file_exist(const char* filename);
// ����tar��
TAR_HEAD* create_tar_block(const int fd, int* offsize);
// ����tar��
int parse_tar_block(const int fd, TAR_HEAD** package);
// ��������Ŀ¼
int create_dir(const char* tar, int mode);
// ����Ŀ¼
int recusive_mkdir(const char* dirname);
// ��tar ��
int open_file(const TAR_HEAD* tar, int* start_new);
// д��
int write_file(const int fd, const TAR_HEAD* tar, const unsigned int write_size, int* start_new);
// ��ӡ����tar���ļ�
void print_tar_all_file(TAR_HEAD* tar);
// ���ض��ļ�
int extract_file(TAR_HEAD* tar, const char* filename);
unsigned char* check_file_hash(const TAR_HEAD* tar, const char* filename);
#endif
