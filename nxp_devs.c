

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


#define U16SWAP(val)    ((((val)>>8)&0xff) | (((val) << 8) & 0xff00))
#define U32SWAP(val)    (((val) >> 24) | (((val) >> 8) & 0xff00) | ((((val) & 0xff00) << 8) & 0xff0000) | (((val) & 0xff) << 24 ))

#define BASE_DEV_PATH	"local_data_new.db"
#define PLAIN_DEV_PATH	"nxpver2data_new.db"

#define DEV_DATA_SIZE		(256)
#define DB_MAX_DEVS			(32)


typedef struct
{
	int oriBaseDbfd;
	int oriPlainDbfd;
	uint16_t oriDbSize;
	int newBaseDbfd;
	int newPlainDbfd;
	uint16_t newDbSize;
}stDbCtx_t;


static stDbCtx_t gCtx;

const static stBaseDev_t gDefaultBaseDevInfo={
	.valid = true,
	.associated = true,
	.reported = false,
};

const static stPlainDev_t gDefaultPlainDevInfo={
	.valid = 1,
	.reported = 1,
	.crypt_type = 0,
	.model = 0x0013,
};


#if 0
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
#endif

static int baseInfoPrint(stBaseDev_t *dev)
{
	if (!dev->ext_addr_h) {
		return 1;
	} 

	//view_data("hex", (uint8_t *)dev, sizeof(*dev));
	LOG("ShortAddr:%04x, IeeeAddr:%08x%08x\r\n", U16SWAP(dev->short_addr), U32SWAP(dev->ext_addr_h), U32SWAP(dev->ext_addr_l));
	LOG("    valid:%d, associated:%d, reported:%d\r\n", dev->valid, dev->associated, dev->reported);
	if (dev->reported == 1){
		LOG("    associated time:%d paired:%d, securitied:%d\r\n",dev->associated_time ,dev->paired, dev->security_enabled);
	}

	return 0;
}

static void plainInfoPrint(stPlainDev_t *dev)
{
	LOG("Plain Device.\r\n");

	LOG("ShortAddr:%04x, IeeeAddr:%02x%02x%02x%02x%02x%02x%02x%02x\r\n", U16SWAP(dev->shortaddr),dev->mac[0], dev->mac[1], dev->mac[2], dev->mac[3], dev->mac[4], dev->mac[5], dev->mac[6], dev->mac[7]);
	LOG("    valid:%d, reported:%d, \r\n", dev->valid, dev->reported);
	if (1 == dev->reported){
		LOG("    model:0x%x sw:0x%x, hw:0x%x\r\n", dev->model, dev->swVer, dev->hwVer);
		LOG("    associated:%d, devType:%d\r\n", dev->associated, dev->devType);
	}
}

static int  devInfo_write(int dbfd, uint16_t idx, uint8_t buf[], uint16_t bufSize)
{
	size_t writeLen;
	lseek(dbfd, idx * bufSize, SEEK_SET);
	writeLen = write(dbfd, buf, bufSize);
	sync();
	return writeLen;
}

static int devInfo_read(int dbfd, uint16_t idx, uint8_t buf[], uint16_t bufSize)
{
	size_t readLen;

	lseek(dbfd, idx * bufSize, SEEK_SET);
	readLen = read(dbfd, buf, bufSize);
	return readLen;
}


static int devsInfo_list(int basefd, int plainfd)
{
	stBaseDev_t *dev=NULL;
	stPlainDev_t *plaindev = NULL;
	uint8_t buf[DEV_DATA_SIZE];
	int i, ret;

	for (i = 0; i < DB_MAX_DEVS; i++) {
		lseek(basefd, i * DEV_DATA_SIZE, SEEK_SET);
		memset(buf, 0,  DEV_DATA_SIZE);
		if (read(basefd, buf, DEV_DATA_SIZE) != DEV_DATA_SIZE) {
			break;
		}   
		dev = (stBaseDev_t *)buf;
		ret = baseInfoPrint(dev);
		if (0 == ret){
			lseek(plainfd, i * DEV_DATA_SIZE, SEEK_SET);
			memset(buf, 0, DEV_DATA_SIZE);
			read(plainfd, buf, DEV_DATA_SIZE);
			plaindev = (stPlainDev_t *)buf;
			plainInfoPrint(plaindev);
			printf("\r\n");
		}
	}    
	LOG("read index:%d\r\n", i);
}

static void basedevInf_cfg(uint8_t buf[], uint16_t idx, uint32_t addrH, uint32_t addL)
{
	stBaseDev_t *devPtr = (stBaseDev_t *)buf;

	devPtr->short_addr = U16SWAP(idx);
	devPtr->ext_addr_l = U32SWAP(addL);
	devPtr->ext_addr_h = U32SWAP(addrH);
}

static void plaindevInf_cfg(uint16_t idx, uint8_t buf[], uint32_t addrH, uint32_t addL)
{
	stPlainDev_t*devPtr = (stPlainDev_t *)buf;

	devPtr->shortaddr = U16SWAP(idx);

	devPtr->mac[0] = (addrH >> 24) & 0xff;
	devPtr->mac[1] = (addrH >> 16) & 0xff;
	devPtr->mac[2] = (addrH >> 8) & 0xff;
	devPtr->mac[3] = (addrH ) & 0xff;

	devPtr->mac[4] = (addL >> 24) & 0xff;
	devPtr->mac[5] = (addL >> 16) & 0xff;
	devPtr->mac[6] = (addL >> 8) & 0xff;
	devPtr->mac[7] = (addL) & 0xff;

	//devPtr->valid = true;
	//devPtr->reported = true;
	//devPtr->crypt_type = 0;
}

int nxpDevsCopy(void)
{
	uint8_t buf[DEV_DATA_SIZE];
	int idx, ret;
	uint16_t baseDevSize, plainDevSize;

	/* Base Devs DB Copy */
	for (idx = 0;idx < DB_MAX_DEVS;idx++){
		if (devInfo_read(gCtx.oriBaseDbfd, idx, buf, DEV_DATA_SIZE) != DEV_DATA_SIZE){
			LOG("Read Index:%d\r\n", idx);
			break;
		}
		devInfo_write(gCtx.newBaseDbfd, idx, buf, DEV_DATA_SIZE);
	}
	baseDevSize = idx;

	/* Plain Devs DB Copy */
	for (idx = 0;idx < DB_MAX_DEVS;idx++){
		if (devInfo_read(gCtx.oriPlainDbfd, idx, buf, DEV_DATA_SIZE) != DEV_DATA_SIZE){
			LOG("Read Index:%d\r\n", idx);
			break;
		}
		devInfo_write(gCtx.newPlainDbfd, idx, buf, DEV_DATA_SIZE);
	}
	plainDevSize = idx;
	LOG("Origin Base Dev Size:%d, Origin Plain Dev Size:%d\r\n", baseDevSize, plainDevSize);
	gCtx.oriDbSize = baseDevSize;
	gCtx.newDbSize = baseDevSize;
	return 0;
}

int nxpDevDbLoad(const char baseDbPathStr[], const char plainDbPathStr[])
{
	int oriBaseDbfd, oriPlainDbfd, newBaseDbfd, newPlainDbfd;

	if (NULL != baseDbPathStr){
		oriBaseDbfd = open(baseDbPathStr, O_RDONLY);
		if (oriBaseDbfd < 0){
			LOG("Open Origin Base DB(%s) Error.\r\n", baseDbPathStr);
			return 1;
		}
		LOG("Opened Origin Base DB.\r\n");
	}

	if (NULL != plainDbPathStr){
		oriPlainDbfd = open(plainDbPathStr, O_RDONLY);
		if (oriPlainDbfd < 0){
			LOG("Open Origin Plain DB(%s) Error.\r\n", plainDbPathStr);
			if (oriBaseDbfd > 0){
				close(oriBaseDbfd);
			}
			return 1;
		}
		LOG("Opened Origin Plain DB.\r\n");
	}

	newBaseDbfd = open(BASE_DEV_PATH, O_RDWR);
	if (newBaseDbfd < 0){
		LOG("Create New Base DB(%s)\r\n", BASE_DEV_PATH);
		newBaseDbfd = open(BASE_DEV_PATH, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		if (newBaseDbfd < 0){
			LOG("Create New Base Error\r\n");
			if (oriBaseDbfd > 0){
				close(oriBaseDbfd);
			}
			if (oriPlainDbfd > 0){
				close(oriPlainDbfd);
			}
			return 1;
		}
	}
	newPlainDbfd = open(PLAIN_DEV_PATH, O_RDWR);
	if (newPlainDbfd < 0) {
		LOG("Create New Plain DB(%s)\r\n", PLAIN_DEV_PATH);
		newPlainDbfd = open(PLAIN_DEV_PATH, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		if (newPlainDbfd < 0) {
			LOG("Create New Plain DB Error\r\n");
			if (oriBaseDbfd > 0){
				close(oriBaseDbfd);
			}
			if (oriPlainDbfd > 0){
				close(oriPlainDbfd);
			}
			close(newBaseDbfd);
			return 1;
		}
	}
	if (NULL != baseDbPathStr){
		gCtx.oriBaseDbfd = oriBaseDbfd;
	}
	if (NULL != plainDbPathStr){
		gCtx.oriPlainDbfd = oriPlainDbfd;
	}
	gCtx.newBaseDbfd = newBaseDbfd;
	gCtx.newPlainDbfd = newPlainDbfd;
	return 0;
}

int oriDevsList(void)
{
	if (0 == gCtx.oriBaseDbfd || 0 == gCtx.oriPlainDbfd){
		LOG_ERR("No Origin Base DB or Origin Plain DB\r\n");
		return 1;
	}
	devsInfo_list(gCtx.oriBaseDbfd, gCtx.oriPlainDbfd);
	return 0;
}

int newDevsList(void)
{
	if (0 == gCtx.newBaseDbfd || 0 == gCtx.newPlainDbfd){
		LOG_ERR("No Create Base DB or Create PlainDB\r\n");
		return 1;
	}
	devsInfo_list(gCtx.newBaseDbfd, gCtx.newPlainDbfd);
	return 0;
}

int newDbDevAdd(uint32_t addrH_u32, uint32_t addrL_u32)
{
	uint16_t shortAddr;
	uint8_t buf[DEV_DATA_SIZE];

	shortAddr = gCtx.newDbSize;

	memset(buf, 0x00, sizeof(buf));
	memcpy(buf, &gDefaultBaseDevInfo, sizeof(gDefaultBaseDevInfo));
	basedevInf_cfg(buf, shortAddr, addrH_u32, addrL_u32);
	devInfo_write(gCtx.newBaseDbfd, shortAddr, buf, sizeof(buf));

	memset(buf, 0x00, sizeof(buf));
	memcpy(buf, &gDefaultPlainDevInfo, sizeof(gDefaultPlainDevInfo));
	plaindevInf_cfg(shortAddr, buf, addrH_u32, addrL_u32);
	devInfo_write(gCtx.newPlainDbfd, shortAddr, buf, sizeof(buf));

	gCtx.newDbSize++;

	LOG("Add New Device:%08x%08x\r\n", addrH_u32, addrL_u32);
	LOG("DB Size:%d\r\n", gCtx.newDbSize);

	return 0;
}

int newDbDevMod(uint16_t shortAddr, uint32_t addrH_u32, uint32_t addrL_u32)
{
	uint8_t buf[DEV_DATA_SIZE];

	memset(buf, 0x00, sizeof(buf));
	memcpy(buf, &gDefaultBaseDevInfo, sizeof(gDefaultBaseDevInfo));
	basedevInf_cfg(buf, shortAddr, addrH_u32, addrL_u32);
	devInfo_write(gCtx.newBaseDbfd, shortAddr, buf, sizeof(buf));

	memset(buf, 0x00, sizeof(buf));
	memcpy(buf, &gDefaultPlainDevInfo, sizeof(gDefaultPlainDevInfo));
	plaindevInf_cfg(shortAddr, buf, addrH_u32, addrL_u32);
	devInfo_write(gCtx.newPlainDbfd, shortAddr, buf, sizeof(buf));

	LOG("Modify Device:%08x%08x\r\n", addrH_u32, addrL_u32);
	LOG("DB Size:%d\r\n", gCtx.newDbSize);

	return 0;
}



