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
// 8 进制转 10进制
unsigned int oct2uint(const char* src, int read_size);
// 文件存在
int file_exist(const char* file_name);
// 创建tar块
struct _tar_head* create_tar_block(const int fd, int* offsize);
// 解析tar块
int parse_tar_block(const int fd, struct _tar_head** package);
// 创建单个目录
int create_dir(const char* tar, int mode);
// 创建目录
int recusive_mkdir(const char* name);
// 打开tar 块
int open_file(const struct _tar_head* tar, int* start_new);
// 写入
int write_file(const int fd, const struct _tar_head* tar, const unsigned int write_size, int* start_new);
// 打印所有tar包文件
void print_tar_all_file(struct _tar_head* tar);
// 解特定文件
int extract_file(struct _tar_head* tar, const char* filename);
#endif