

#ifndef _NXP_DEVS_H_
#define _NXP_DEVS_H_


#include <stdint.h>
#include <stdbool.h>



typedef struct Ext_Addr{
	uint32_t l;
	uint32_t h;
}stExtAddr_t;


typedef  struct Base_Dev{
	bool valid;
	bool associated;
	bool removed;
	bool reported;
	uint8_t dev_type;
	uint16_t hw_version;
	uint16_t sw_version;

	uint32_t ext_addr_l;
	uint32_t ext_addr_h;
	uint16_t short_addr;
	uint32_t associated_time;

	bool  security_enabled;
	bool  paired;
	uint8_t sk[32];
	uint8_t pk[32];
	uint8_t dev_pk[32];
	uint8_t link_key[16];
	uint8_t capability;

	/* meter parameters */
	uint8_t meterIsRcd;
	float   meterVol;
	float   meterCur;
	float   meterPow;
	float   meterEng;
	float   meterMaxPow;
	uint16_t battery;
}stBaseDev_t;

typedef struct Plain_Dev{
	uint8_t valid;          // 该槽是否被占
	uint8_t associated;     // 是否关联，未使用
	uint8_t removed;        // 未知，
	uint8_t reported;       // 是否上报
	uint8_t devType;        // 设备类型
	uint16_t reserve1;      // 预留
	uint16_t reserve2;      // 预留
	uint8_t  mac[8];        // mac 8 bytes
	uint16_t shortaddr;     // short address 2 bytes

	uint8_t reserve[124];

	uint16_t model;         // 设备模型
	uint32_t swVer;         // 软件版本
	uint32_t hwVer;         // 硬件版本
	uint8_t state;
	uint8_t crypt_type;
	uint8_t batPercent;     // 电池电量
	uint32_t lastTime;      // 上线时间

}stPlainDev_t;


int nxpDevDbLoad(const char baseDbPathStr[], const char plainDbPathStr[]);

int oriDevsList(void);

int newDevsList(void);



#endif

