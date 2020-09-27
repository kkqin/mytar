#include "mytar.h"
#include "Configure.h"

int main(int argc, char** argv) {
	printf("version %d.%d\n", MYTAR_VERSION_MAJOR, MYTAR_VERSION_MINOR);

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
	printf("destination file: %s\n", file);

	TAR_HEAD* tar = NULL;
	int part_time = parse_tar_block(fd, &tar);
	if(!part_time) {
		printf("Error: parsing failed.\n");
		return part_time;
	}
	else{
		printf("Read Part: %d\n", part_time);
	}

	print_tar_all_file(tar);
	//extract_file(tar, file);
	check_file_hash(tar, file);

	printf("DONE.\n");
	return 0;
}
