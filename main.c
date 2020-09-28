#include "mytar.h"
#include "Configure.h"

void print_hash(const unsigned char* digest, const char* filename); 
int main(int argc, char** argv) {
	printf("version %d.%d\n", MYTAR_VERSION_MAJOR, MYTAR_VERSION_MINOR);

	if(argc < 3) {
		fprintf(stderr, "arguments less.\n");
		return -1;
	}

	if(argv[1][0] != 'x' && argv[1][0] != 'p') {
		fprintf(stderr, "Error: only support 'x' 'p' 'e' 'h'.\n");
		fprintf(stderr, "Usage: %s 'p' tarfile.\n", argv[0]);
		fprintf(stderr, "Usage: %s 'x' tarfile 'e' tarfile/file.\n", argv[0]);
		fprintf(stderr, "Usage: %s 'x' tarfile 'h' tarfile/file.\n", argv[0]);
		return -1;
	}

	char *target_file = argv[2];
	fprintf(stderr, "target file: %s\n", target_file);

	int fd = file_exist(target_file); 
	if(fd < 0) {
		fprintf(stderr, "Error: file doesn't exist.\n");
		return -1;
	}

	TAR_HEAD* tar = NULL;
	int part_time = parse_tar_block(fd, &tar);
	if(!part_time) {
		fprintf(stderr, "Error: parsing failed.\n");
		return part_time;
	}
	else {
		//fprintf(stderr, "Read Part: %d\n", part_time);
	}

	char *file = argv[3];

	if(argv[1][0] == 'p') {
		printf("file are: \n");
		// 打印所有
		print_tar_all_file(tar);
		printf("\n");
	}

	char option = 0;
	if(argc > 3) {
		file = argv[4];
		option = argv[3][0];
	
		if(file == NULL) {
			printf("file error.\n");
			return -1;
		}
	}

	if(option == 'h') {
		// 查看文件hash
		unsigned char* digest = check_file_hash(tar, file);
		print_hash(digest, file);
	}
	else if(option == 'e') {
		// 解文件/多个/单个
		extract_file(tar, file);
	}

	//printf("DONE.\n");
	free_tar_head(tar);
	return 0;
}

void print_hash(const unsigned char* digest, const char* filename) {
	for(int i = 0; i < 16; i++) 
		printf("%02x", digest[i]);
	printf("%s\n", filename);
}
