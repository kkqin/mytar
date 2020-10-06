#include "mytar.h"
#include "picohash.h"

#define BLOCKSIZE 512

unsigned int oct2uint(const char* src, int read_size) {
	unsigned int result = 0; // 下标从0开始	
	int i = 0;
	while(i < read_size) {
		unsigned int byte_num = (unsigned int)src[i++] - '0';
		result = (result << 3) | byte_num;
	}
	return result;
}

int file_exist(const char* filename) {
	int fd = -1;
#ifdef __linux__
	if((fd = open(filename, O_RDWR)) < 0)
		return -1;
#elif WIN32
	if((fd = open(filename, O_RDWR | _O_BINARY)) < 0)
		return -1;
#endif

	return fd;
}

TAR_HEAD* create_tar_block(const int fd, int* offsize) {
	TAR_HEAD* tar = (TAR_HEAD*)calloc(1, sizeof(TAR_HEAD));
	int bytes = read(fd, tar->block, BLOCKSIZE); 
	if(bytes != BLOCKSIZE) {
		free(tar);
		//printf("not BLOCKSIZE. is end.\n");
		return NULL;
	}

	*offsize += bytes;

	lseek(fd, *offsize, SEEK_SET);
	
	return tar;
}

int judge_head(TAR_HEAD* tar) {
	TAR_HEAD* prev = tar->prev;
	if(prev == NULL) {
		tar->itype = HEAD;
		return -1;
	}

	ssize_t file_size = oct2uint(prev->size, 11);
	ssize_t block_size = strlen(tar->block) + 1;
	if(prev->type == lf_longname 
		&& file_size == block_size  
		&& !strncmp(prev->ustar, "ustar", 5)) {
		tar->itype = LONGNAME_HEAD;
	}
	else if(!strncmp(tar->ustar, "ustar", 5)){
		tar->itype = HEAD;
	}
	else {
		tar->itype = BODY;
	}
}

int parse_tar_block(const int fd, TAR_HEAD** package, skiplist* skp) {
	int count = 0;
	int offsize = 0;
	TAR_HEAD* head = NULL, *tail = NULL;
	int key = 1;

	while(1) {
		TAR_HEAD* tar = create_tar_block(fd, &offsize); 
		if(head == NULL && tail == NULL) {
			head = tail = tar;
		}

		if(tar == NULL)
			break;
		
		tar->prev = tail;
		tail->next = tar;
		tail = tar;

		judge_head(tar);

		// go index
		if(skp != NULL)
		if (tar->itype == LONGNAME_HEAD) {
			insert_node(skp, key++, tar); 
		}
		else if (tar->itype == HEAD && tar->type != lf_longname && tar->prev->itype != LONGNAME_HEAD) {
			insert_node(skp, key++, tar); 
		}

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

int recusive_mkdir(const char* dirname) {
	const size_t len = strlen(dirname);
	if (!len)
		return -1;

	char* path = calloc(len + 1, sizeof(char));
	strncpy(path, dirname, len);

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

int open_file(const TAR_HEAD* tar, int* start_new) {
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

int write_file(const int fd, const TAR_HEAD* tar, const unsigned int write_size, int* start_new) {
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

static int extract_tar_block(const TAR_HEAD* tar) {

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
				printf("create director failed.\n");
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
			if(body_write_size < BLOCKSIZE)
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

void print_tar_all_file(TAR_HEAD* tar) {
	int total_file = 1;
	while(tar) {
		if (tar->itype == LONGNAME_HEAD) {
			printf("%s\n", tar->block);
			total_file++;
		}
		else if (tar->itype == HEAD && tar->type != lf_longname && tar->prev->itype != LONGNAME_HEAD) {
			printf("%.*s\n", 100, tar->name);
			total_file++;
		}
		tar = tar->next;
	}

	printf("total_file: %d\n", total_file);
}

static int extract_dir_tar_block(const TAR_HEAD* tar, const char* name) {
	recusive_mkdir(name);

	TAR_HEAD* save = tar;
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
			if(body_write_size < BLOCKSIZE)
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

static void back_tracking(const TAR_HEAD* past, TAR_HEAD* now) {
	now = past;
}

unsigned char* check_file_hash(const TAR_HEAD* tar, const char* filename) {
	if(!filename) {
		printf("file hash:name empty");
		return NULL;
	}

	TAR_HEAD* save = tar;
	while (tar) {
		if (strstr(tar->name, filename) != NULL) {
			break;
		}
		tar = tar->next;
	}

	if(!tar || tar->itype != HEAD) {
		printf("file hash:can't find the file.");
		back_tracking(save, tar);
		return NULL;
	}

	ssize_t body_write_size = oct2uint(tar->size, 11);
	if(!body_write_size) {
		back_tracking(save, tar);
		return NULL;
	}

	picohash_ctx_t ctx;
	unsigned char* digest = (unsigned char*)malloc(sizeof(char)*PICOHASH_MD5_DIGEST_LENGTH);

	picohash_init_md5(&ctx);

	int tmp_size = body_write_size;
	for(;tmp_size; tar = tar->next) {
		if(tar->itype != BODY)
			continue;
		int blockSize = sizeof(tar->block);
		if(tmp_size < BLOCKSIZE)
		    blockSize = tmp_size;
		picohash_update(&ctx, tar->block, blockSize);
		tmp_size -= blockSize;
		if(tmp_size <= 0) {
			break;
		}
	}

	picohash_final(&ctx, digest);
	back_tracking(save, tar);
	return digest;
}

static int extract_file_tar_block(const TAR_HEAD* tar, const char* name) {

	ssize_t body_write_size = 0;
	int start_new = 0;
	int fd = -1;

	recusive_mkdir(name);

	TAR_HEAD* save = tar;

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

		if (afile >= 2) {
			free(tar);
			close(fd);
			break;
		}

		if(tar->type == lf_normal &&
			tar->itype == HEAD) {
			fd = open_file(tar, &start_new);
		}
		else if(tar->itype == BODY) {
			unsigned int blockSize = sizeof(tar->block);
			if(body_write_size < BLOCKSIZE)
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

	tar = save;
	return 0;
}

int extract_file(TAR_HEAD* tar, const char* filename) {
	if (filename && strlen(filename) > 100) {
		printf("name invaild.\n");
		return -1;
	}

	if (filename) {
		extract_file_tar_block(tar, filename);
		extract_dir_tar_block(tar, filename);
	}
	else {
		extract_tar_block(tar);
	}
	return 0;
}

int free_tar_head(TAR_HEAD* tar) {
	free(tar);
	return 0;
}
