#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
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

enum {
	 HEAD = 1,
	 BODY 
};

#pragma pack(1) 
struct _tar_head{
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
};

unsigned int oct2uint(const char* src, int read_size) {
	unsigned int result = 0; // 下标从0开始	
	int i = 0;
	while(i < read_size) {
		unsigned int byte_num = (unsigned int)src[i++] - '0';
		result = (result << 3) | byte_num;
	}
	return result;
}

int file_exist(const char* file_name) {
	int fd = -1;
	if((fd = open(file_name, O_RDWR)) < 0)
		return -1;

	return fd;
}

struct _tar_head* create_tar_block(const int fd, int* offsize) {
	struct _tar_head* tar = (struct _tar_head*)malloc(sizeof(struct _tar_head));
	tar->next = NULL;
	tar->end = 0; 
	int bytes = read(fd, tar->block, 512); 
	if(bytes != 512) {
		free(tar);
		printf("not 512. is end.\n");
		return NULL;
	}

	// simple check;
	if( !strcmp(tar->ustar ,"ustar") || !strcmp(tar->ustar, "ustar  ") ) {
		tar->itype = HEAD; 
	}
	else {
		tar->itype = BODY;
	}
	
	return tar;
}

int parse_tar_block(const int fd, struct _tar_head** package) {
	int count = 0;
	int offsize = 0;
	struct _tar_head* head = NULL, *tail = NULL;
	while(1) {
		struct _tar_head* tar = create_tar_block(fd, &offsize); 
		if(head == NULL && tail == NULL) {
			head = tail = tar;
		}

		if(tar == NULL)
			break;

		tail->next = tar;
		tail = tar;
		count++;
	}

	*package = head;
	return count;
}

int create_dir(struct _tar_head* tar) {
	int len = strlen(tar->name);
	char* path = calloc(len + 1, sizeof(char));
	strncpy(path, tar->name, len);

	while(--len && path[len] != '/')
		path[len] = '\0'; 		
	
	if((opendir(path))!=NULL) {
		printf("Error, path already exist.\n");
		return -1;
	} 

	int res = mkdir(path, oct2uint(tar->mode, 7) & 0777);
	if(res) { 
		printf("mkdir failed.\n");
		return res;
	}	
}

int open_file(const struct _tar_head* tar, int* start_new) {
	int fd = open(tar->name, O_WRONLY|O_CREAT|O_TRUNC, oct2uint(tar->mode, 7) & 0777);
	if(fd < 0) {
		printf("Error: %s\n", __func__);
		return -1;
	}

	if(!(*start_new) && 
		lseek(fd, 0, SEEK_SET) == -1) {
		printf("Error: seek file error.\n");
		return -1;
	}

	return fd;
}

int write_file(const int fd, const struct _tar_head* tar, const unsigned int write_size, int* start_new) {
	if(tar->itype == HEAD)
		return -1;

	ssize_t size = write(fd, tar->block, write_size);
	if(size < 0) {
		printf("Error: write error\n");
		return -1; 
	}

	*start_new = 1;
	
	return 0;
}

int extract_tar_block(const struct _tar_head* tar) {

	ssize_t body_write_size = 0;
	int start_new = 0;
	int fd = -1;
	int mode = -1;
	while(tar) {
		if(tar->itype == HEAD) {
			body_write_size = oct2uint(tar->size, 11);
			if(body_write_size)
				start_new = 0;
		}

		if(tar->type == lf_dir && 
			tar->itype == HEAD) {
			if( create_dir(tar) < 0 ) {
				printf("extracting failed.\n");
			}

			close(fd); // close front fd 
			fd = -1;
		}
		else if(tar->type == lf_normal &&
			tar->itype == HEAD) {
			fd = open_file(tar, &start_new);
		}
		else if(tar->itype == BODY) {
			unsigned int blockSize = sizeof(tar->block);
			if(body_write_size < 512)
				blockSize = body_write_size;
			write_file(fd, tar, blockSize, &start_new);
			body_write_size -= blockSize;
		}
		else {
			printf("Error: mode %c not support now.\n", tar->type);
			return -1;
		}
		// go next block 
		tar = tar->next;
	}

	return 0;
}

int main(int argc, char** argv) {

	if(argc < 3) {
		printf("Error: arguments less.\n");
		return -1;
	}

	if(argv[1][0] != 'x') {
		printf("Error: only support 'x'.\n");
		return -1;
	}
	
	char *target_file = argv[2];
	printf("target file: %s\n", target_file);

	int fd = file_exist(target_file); 
	if(fd < 0) {
		printf("Error: file doesn't exist.\n");
		return -1;
	}

	struct _tar_head* tar = NULL;
	int part_time = parse_tar_block(fd, &tar);
	if(!part_time) {
		printf("Error: parsing failed.\n");
		return part_time;
	}
	else{
		printf("Read Part: %d\n", part_time);
	}

	int res = extract_tar_block(tar); 
	if(res) {
		printf("Error: extract tar failed.\n");
		return res;
	}
	return 0;
}

