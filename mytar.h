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

struct _tar_head;
// 8 ����ת 10����
unsigned int oct2uint(const char* src, int read_size);
// �ļ�����
int file_exist(const char* file_name);
// ����tar��
struct _tar_head* create_tar_block(const int fd, int* offsize);
// ����tar��
int parse_tar_block(const int fd, struct _tar_head** package);
// ��������Ŀ¼
int create_dir(const char* tar, int mode);
// ����Ŀ¼
int recusive_mkdir(const char* name);
// ��tar ��
int open_file(const struct _tar_head* tar, int* start_new);
// д��
int write_file(const int fd, const struct _tar_head* tar, const unsigned int write_size, int* start_new);
// ��ӡ����tar���ļ�
void print_tar_all_file(struct _tar_head* tar);
// ���ض��ļ�
int extract_file(struct _tar_head* tar, const char* filename);
#endif