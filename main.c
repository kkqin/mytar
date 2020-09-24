#include "mytar.h"

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

	const char* file = argv[3];
	//printf("destination file: %s\n", file);
	printf_tar_all_file(fd);
	//extract_all_file(fd);
	//extract_file(fd, "mytar/main.c");
	//extract_file(fd, "mytar/.git/config");
	//extract_file(fd, "mytar");
	extract_file(fd, file);

	printf("DONE.\n");
	return 0;
}
