#include <stdio.h>
#include <stdlib.h>
#include "ttf.h"

int main(int argc,char **argv){
	if(argc != 2){
		printf("usage : test FONT\n");
		return 1;
	}

	ttf_file *font = ttf_open(argv[1]);
	if(!font){
		perror(argv[1]);
		return 1;
	}

	return 0;
}
