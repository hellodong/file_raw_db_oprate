

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "logx.h"
#include "nxp_devs.h"
#include "inter_cmd.h"


int main(int argc, char *argv[])
{
	char *inputStr = NULL;
	char *tokenStr = NULL;
	char *argvArray[32];
	size_t len;
	uint16_t readIdx;

	printf("input # ");
	while(getline(&inputStr, &len, stdin) != -1){
		tokenStr=strtok(inputStr, " ");
		for(readIdx = 0;tokenStr != NULL && readIdx < 32;readIdx++) {   
			argvArray[readIdx] = tokenStr;
			tokenStr=strtok(NULL, " ");
		}   
		cmd_handler(readIdx, argvArray);
		printf("input # ");
	}

	return 0;
}


