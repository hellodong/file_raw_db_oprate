

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


#define U16SWAP(val)    ((((val)>>8)&0xff) | (((val) << 8) & 0xff00))
#define U32SWAP(val)    (((val) >> 24) | (((val) >> 8) & 0xff00) | ((((val) & 0xff00) << 8) & 0xff0000) | (((val) & 0xff) << 24 ))

#define BASE_DEV_PATH	"local_data_new.db"
#define PLAIN_DEV_PATH	"nxpver2data_new.db"


static void view_data(const char str[], uint8_t data[], uint16_t dataLen);
static void base_devinfo_list(int fd);
static void devinfo_list(int basefd, int plainfd);
static void write_addrInfo(uint32_t addrH, uint32_t addrL);


typedef struct
{
	int baseDevWritefd;
	int plainDevWritefd; int index; uint8_t baseDevBin[DEV_DATA_SIZE];
	uint8_t plainDevBin[DEV_DATA_SIZE];
}stCtx_t;

static stCtx_t gCtx;

int main(int argc, char *argv[])
{
	if (argc < 3){
		LOG("No Enough Argsments.\r\n");
		return 1;
	}	

	int basefd, fd;
	char cmd[1024];
	char addrH[16];
	char addrL[16];
	uint32_t addrH_u32;
	uint32_t addrL_u32;
	int inputLen;

	basefd = open(argv[1],O_RDWR);
	fd = open(argv[2], O_RDWR);

	devinfo_list(basefd, fd);

	printf("input # ");
	while(scanf("%s",cmd)){
		memcpy(addrH, cmd, 8);
		memcpy(addrL, cmd + 8, 8);
		addrH[8] = '\0';
		addrL[8] = '\0';
		addrH_u32= strtol(addrH, NULL, 16);
		addrL_u32 = strtol (addrL, NULL, 16);
		printf("%08x%08x\r\n", addrH_u32, addrL_u32);
		write_addrInfo(addrH_u32, addrL_u32);

		printf("input # ");
	}
	
	return 0;
}

static int baseInfoPrint(stBaseDev_t *dev)
{
	if (!dev->ext_addr_h) {
	    return 1;
	} 

	view_data("hex", (uint8_t *)dev, sizeof(*dev));
	LOG("ShortAddr:%04x, IeeeAddr:%08x%08x\r\n", U16SWAP(dev->short_addr), U32SWAP(dev->ext_addr_h), U32SWAP(dev->ext_addr_l));
	LOG("                                valid:%d, associated:%d, associated time:%d\r\n", dev->valid, dev->associated, dev->associated_time);
	LOG("                                paired:%d, securitied:%d\r\n", dev->paired, dev->security_enabled);
	LOG("                                reported:%d\r\n", dev->reported);
	view_data("                             sk", dev->sk, sizeof(dev->sk));
	view_data("                             pk", dev->pk, sizeof(dev->pk));
	view_data("                             dev_pk", dev->dev_pk, sizeof(dev->dev_pk));
	view_data("                             link_key", dev->link_key, sizeof(dev->link_key));
	LOG("    \r\n");
	
	return 0;
}

static void plainInfoPrint(stPlainDev_t *dev)
{
	LOG("Plain Device.\r\n");

	LOG("ShortAddr:%04x, IeeeAddr:%02x%02x%02x%02x%02x%02x%02x%02x\r\n", U16SWAP(dev->shortaddr),dev->mac[0], dev->mac[1], dev->mac[2], dev->mac[3], dev->mac[4], dev->mac[5], dev->mac[6], dev->mac[7]);
	LOG("                                valid:%d, associated:%d\r\n", dev->valid, dev->associated);
	LOG("                                reported:%d, devType:%d\r\n", dev->reported, dev->devType);
	LOG("								 model:0x%x sw:0x%x, hw:0x%x\r\n", dev->model, dev->swVer, dev->hwVer);
	LOG("                                lastime:%d\r\n", dev->lastTime);
	LOG("    \r\n");
}


static void view_data(const char str[], uint8_t data[], uint16_t dataLen)
{
	printf("%s(%04d):", str, dataLen);
	uint16_t idx;
	for(idx = 0;idx < dataLen;idx++){
		if (idx % 16 == 0 && idx) {
			printf("\r\n%s(    ):", str);
		}   
		printf("[%02x]", data[idx]); 
	}   
	printf("\r\n");
}


static void base_devinfo_list(int fd)
{
	stBaseDev_t *dev=NULL;
	uint8_t buf[DEV_DATA_SIZE];
	int i;
	for (i = 0; i < 20; i++) {
		lseek(fd, i * DEV_DATA_SIZE, SEEK_SET);
		memset(buf, 0,  DEV_DATA_SIZE);
		if (read(fd, buf, DEV_DATA_SIZE) != DEV_DATA_SIZE) {
			break;
		}   
		dev = (stBaseDev_t *)buf;
		baseInfoPrint(dev);
	}    
	LOG("read index:%d\r\n", i);
}

static void devinfo_list(int basefd, int plainfd)
{
	stBaseDev_t *dev=NULL;
	stPlainDev_t *plaindev = NULL;
	uint8_t buf[DEV_DATA_SIZE];
	int i, ret;
	for (i = 0; i < 20; i++) {
		lseek(basefd, i * DEV_DATA_SIZE, SEEK_SET);
		memset(buf, 0,  DEV_DATA_SIZE);
		if (read(basefd, buf, DEV_DATA_SIZE) != DEV_DATA_SIZE) {
			break;
		}   
		dev = (stBaseDev_t *)buf;
		ret = baseInfoPrint(dev);
		if (0 == ret){
			if (0 == dev->reported) {
				memcpy(gCtx.baseDevBin,buf, DEV_DATA_SIZE);
			}
			lseek(plainfd, i * DEV_DATA_SIZE, SEEK_SET);
			memset(buf, 0, DEV_DATA_SIZE);
			read(plainfd, buf, DEV_DATA_SIZE);
			if (0 == dev->reported){
				memcpy(gCtx.plainDevBin, buf, DEV_DATA_SIZE);
			}
			plaindev = (stPlainDev_t *)buf;
			plainInfoPrint(plaindev);
		}

	}    
	LOG("read index:%d\r\n", i);
}

static void basedev_write(int writefd, uint32_t addrH, uint32_t addL, uint16_t idx)
{
	uint32_t buf[DEV_DATA_SIZE];
	stBaseDev_t *devPtr = (stBaseDev_t *)buf;
	memcpy(buf, gCtx.baseDevBin, DEV_DATA_SIZE);
	devPtr->ext_addr_l = U32SWAP(addL);
	devPtr->ext_addr_h = U32SWAP(addrH);
	devPtr->short_addr = U16SWAP(idx);
	
	lseek(writefd, idx * DEV_DATA_SIZE, SEEK_SET);
	write(writefd, buf, sizeof(buf));
	sync();
	close(writefd);
}

static void plaindev_write(int writefd, uint32_t addrH, uint32_t addL, uint16_t idx)
{
	uint32_t buf[DEV_DATA_SIZE];
	stPlainDev_t*devPtr = (stPlainDev_t *)buf;
	memcpy(buf, gCtx.plainDevBin, DEV_DATA_SIZE);
	devPtr->shortaddr = U16SWAP(idx);
	devPtr->mac[0] = (addrH >> 24) & 0xff;
	devPtr->mac[1] = (addrH >> 16) & 0xff;
	devPtr->mac[2] = (addrH >> 8) & 0xff;
	devPtr->mac[3] = (addrH ) & 0xff;

	devPtr->mac[4] = (addL >> 24) & 0xff;
	devPtr->mac[5] = (addL >> 16) & 0xff;
	devPtr->mac[6] = (addL >> 8) & 0xff;
	devPtr->mac[7] = (addL) & 0xff;

	devPtr->crypt_type = 0;
	devPtr->reported = 1;
	
	lseek(writefd, idx * DEV_DATA_SIZE, SEEK_SET);
	write(writefd, buf, sizeof(buf));
	sync();
	close(writefd);
}

static void write_addrInfo(uint32_t addrH, uint32_t addrL)
{
	uint8_t tmp = 0;
	int basefd = open(BASE_DEV_PATH, O_RDWR);
	if (basefd < 0){
		LOG("Create %s\r\n", BASE_DEV_PATH);
		basefd = open(BASE_DEV_PATH, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		//lseek(basefd, 20*DEV_DATA_SIZE - 1, SEEK_SET);
		write(basefd, &tmp, 1);
		close(basefd);
		basefd = open(BASE_DEV_PATH, O_RDWR);
	}

	int plainfd = open(PLAIN_DEV_PATH, O_RDWR);
	if (plainfd < 0) {
		uint8_t tmp = 0;
		LOG("Create %s\r\n",PLAIN_DEV_PATH );
		plainfd = open(PLAIN_DEV_PATH , O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		//lseek(plainfd, 20*DEV_DATA_SIZE - 1, SEEK_SET);
		write(plainfd, &tmp, 1);
		close(plainfd);
		plainfd = open(PLAIN_DEV_PATH, O_RDWR);
	}
	basedev_write(basefd, addrH, addrL, gCtx.index);
	plaindev_write(plainfd, addrH, addrL, gCtx.index);
	LOG("Done:%d\r\n", gCtx.index);
	gCtx.index++;
}



