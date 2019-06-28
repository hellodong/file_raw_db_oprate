

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "logx.h"
#include "nxp_devs.h"
#include "inter_cmd.h"


typedef void (*func_t)(int argc, char *argv[]);

typedef struct
{
	char cmdStr[128];
	func_t func;
	char usage[1024];
}stCmdCtx_t;


static void add_cmd_process(int argc, char *argv[]);

static void list_cmd_process(int argc, char *argv[]);

static void help_cmd_process(int argc, char *argv[]);


static stCmdCtx_t gCmdArray[] = {
	{"add", add_cmd_process,"add <mac> <short addr> "},
	{"list",list_cmd_process , "list 0/1"},
	{"help",help_cmd_process ,"help"},
};


int cmd_handler(int argc, char *argv[])
{
	uint16_t idx;
	if (argc == 0){
		return 1;
	}
	printf("argv[0]:%s\r\n", argv[0]);
	for (idx = 0; idx < sizeof(gCmdArray) / sizeof(gCmdArray[0]);idx++)	{
		if (strcmp(gCmdArray[idx].cmdStr, argv[0]) == 0){
			func_t func = gCmdArray[idx].func;
			func(argc - 1, &argv[1]);
			return 0;
		}
	}
	help_cmd_process(0, NULL);
	return 1;
}

static void add_cmd_process(int argc, char *argv[])
{
	if (argc < 1){
		LOG("No Enough Args(%d).\r\n", argc);
		return ;
	}
	uint16_t shortAddr;
	char addrH[16];
	char addrL[16];
	uint32_t addrH_u32;
	uint32_t addrL_u32;
	memcpy(addrH, &argv[0][0], 8);
	memcpy(addrL, &argv[0][8], 8);
	addrH[8] = '\0';
	addrL[8] = '\0';
	addrH_u32= strtol(addrH, NULL, 16);
	addrL_u32 = strtol (addrL, NULL, 16);

	if (argc > 1){//modify device info
		shortAddr = atoi(argv[1]);

		LOG("Short Addr:%x, MAC:%08x%08x\r\n", shortAddr, addrH_u32, addrL_u32);
		newDbDevMod(shortAddr, addrH_u32, addrL_u32);
	}else{//add device info
		LOG("MAC:%08x%08x\r\n", addrH_u32, addrL_u32);
		newDbDevAdd(addrH_u32, addrL_u32);
	}
}

static void list_cmd_process(int argc, char *argv[])
{
	if (argc < 1){
		LOG("No Enough Args(%d).\r\n", argc);
		return ;
	}
	int listflag = 1;
	listflag = atoi(argv[0]);
	LOG("%d\r\n", listflag);
	switch (listflag){
		case 0:
			oriDevsList();
			break;
		case 1:
			newDevsList();
			break;
		default:
			LOG("Error Argument.\r\n");
			break;
	}
}

static void help_cmd_process(int argc, char *argv[])
{
	uint16_t idx;
	printf("help:\r\n");
	for (idx = 0; idx < sizeof(gCmdArray) / sizeof(gCmdArray[0]);idx++)	{
		printf("  %s--%s\r\n", gCmdArray[idx].cmdStr, gCmdArray[idx].usage);
	}
}

