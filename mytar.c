#include "mytar.h"

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
	unsigned int result = 0; // �±��0��ʼ	
	int i = 0;
	while(i < read_size) {
		unsigned int byte_num = (unsigned int)src[i++] - '0';
		result = (result << 3) | byte_num;
	}
	return result;
}

int file_exist(const char* file_name) {
	int fd = -1;
#ifdef __linux__
	if((fd = open(file_name, O_RDWR)) < 0)
		return -1;
#elif WIN32
	if((fd = open(file_name, O_RDWR | _O_BINARY)) < 0)
		return -1;
#endif

	return fd;
}

struct _tar_head* create_tar_block(const int fd, int* offsize) {
	struct _tar_head* tar = (struct _tar_head*)calloc(1,sizeof(struct _tar_head));
	int bytes = read(fd, tar->block, 512); 
	if(bytes != 512) {
		free(tar);
		printf("not 512. is end.\n");
		return NULL;
	}

	*offsize += bytes;

	// simple check;
	if(!strncmp(tar->ustar ,"ustar", 5)) {
		tar->itype = HEAD; 
	}
	else {
		tar->itype = BODY;
	}

	lseek(fd, *offsize, SEEK_SET);
	
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
	lseek(fd, 0, SEEK_SET);
	return count;
}

int create_dir(const char* name, int mode) {
	int len = strlen(name);
	char* path = calloc(len + 1, sizeof(char));
	strncpy(path, name, len);

	while(--len && path[len] != '/')
		path[len] = '\0'; 		
	
	if((opendir(path))!=NULL) {
		printf("Error, path already exist.\n");
		return -1;
	} 

#ifdef __linux__
	int res = mkdir(path, mode);
#elif WIN32
	int res = mkdir(path);
#endif
	if(res) { 
		printf("mkdir failed.\n");
		return res;
	}	

	return 0;
}

int recusive_mkdir(const char* name) {
	const size_t len = strlen(name);
	if (!len)
		return -1;

	char* path = calloc(len + 1, sizeof(char));
	strncpy(path, name, len);

	if (path[len - 1] == '/')
		path[len - 1] = 0;

	for(char *p = path; *p; p++) {
		if (*p == '/') {
			*p = '\0';

#ifdef	__linux__
			mkdir(path, 0755);
#elif	WIN32
			mkdir(path);
#endif
			*p = '/';
		}
	}

	free(path);

	return 0;
}

int open_file(const struct _tar_head* tar, int* start_new) {
	int fd = -1;
#ifdef __linux
	fd = open(tar->name, O_WRONLY|O_CREAT|O_TRUNC, oct2uint(tar->mode, 7) & 0777);
	if(fd < 0) {
		printf("Error: %s\n", __func__);
		return -1;
	}
#elif WIN32
	fd = open(tar->name, O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, oct2uint(tar->mode, 7) & 0777);
	if(fd < 0) {
		printf("Error: %s, %d\n", __func__, errno);
		if (errno == EACCES) {
			printf("shit things happend.\n");
		}
		return -1;
	}
#endif

	if(!(*start_new) && 
		lseek(fd, 0, SEEK_SET) == -1) {
		printf("Error: seek file error.\n");
		return -1;
	}

	return fd;
}

int write_file(const int fd, const struct _tar_head* tar, const unsigned int write_size, int* start_new) {
	if (tar->itype == HEAD || fd < 0) {
		printf("can't convert file. file already exist.\n");
		return -1;
	}

	ssize_t size = write(fd, tar->block, write_size);
	if(size < 0) {
		printf("Error: write error\n");
		return -1; 
	}

	*start_new = 1;
	
	return 0;
}

static int extract_tar_block(const struct _tar_head* tar) {

	ssize_t body_write_size = 0;
	int start_new = 0;
	int fd = -1;
	while(tar) {
		if(tar->itype == HEAD) {
			body_write_size = oct2uint(tar->size, 11);
			if(body_write_size)
				start_new = 0;
		}

		if(tar->type == lf_dir && 
			tar->itype == HEAD) {
			if( create_dir(tar->name, oct2uint(tar->mode, 7) & 0777) < 0 ) {
				printf("extracting failed.\n");
			}

			if (fd > 0) {
				close(fd); // close front fd 
				fd = -1;
			}
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

int printf_tar_all_file(const int fd) {
	struct _tar_head* tar = NULL;
	parse_tar_block(fd, &tar);

	while(tar) {
		if (tar->itype == HEAD) {
			printf("%s\n", tar->name);
		}
		tar = tar->next;
	}

	return 0;
}

int extract_all_file(const int fd) {
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

static int extract_dir_tar_block(const struct _tar_head* tar, const char* name) {
	recusive_mkdir(name);

	while(tar) {
		if(tar->type == lf_dir && strstr(tar->name, name) != NULL) {
			break;
		}
		tar = tar->next;
	}

	ssize_t body_write_size = 0;
	int start_new = 0;
	int fd = -1;
	while(tar) {

		if (tar->itype == HEAD && strstr(tar->name, name) == NULL)
			break;

		if(tar->itype == HEAD) {
			body_write_size = oct2uint(tar->size, 11);
			if(body_write_size)
				start_new = 0;
		}

		if(tar->type == lf_dir && 
			tar->itype == HEAD) {
			if( create_dir(tar->name, oct2uint(tar->mode, 7) & 0777) < 0 ) {
				printf("extracting failed.\n");
			}

			if (fd > 0) {
				close(fd); // close front fd 
				fd = -1;
			}
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

static int extract_file_tar_block(const struct _tar_head* tar, const char* name) {

	ssize_t body_write_size = 0;
	int start_new = 0;
	int fd = -1;

	recusive_mkdir(name);

	while (tar) {
		if (strstr(tar->name, name) != NULL) {
			break;
		}
		tar = tar->next;
	}

	// parse a file
	int afile = 0;
	while(tar) {
		if (tar->itype == HEAD) {
			body_write_size = oct2uint(tar->size, 11);
			if (body_write_size)
				start_new = 0;
			afile++;
		}

		if (afile >= 2)
			break;

		if(tar->type == lf_normal &&
			tar->itype == HEAD) {
			fd = open_file(tar, &start_new);
		}
		else if(tar->itype == BODY) {
			unsigned int blockSize = sizeof(tar->block);
			if(body_write_size < 512)
				blockSize = body_write_size;
			write_file(fd, tar, blockSize, &start_new);
			body_write_size -= blockSize;
			if(!body_write_size)
				break;
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

int extract_file(const int fd, const char * filename) {
	if (strlen(filename) > 100) {
		printf("name invaild.\n");
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

	extract_file_tar_block(tar, filename);
	extract_dir_tar_block(tar, filename);
	return 0;
}