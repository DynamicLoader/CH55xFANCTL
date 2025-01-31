#ifndef __DATAFLASH_H__
#define __DATAFLASH_H__

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;

typedef struct {
    uint8_t bound;
    uint8_t duty;
} fanlevel_t; // 2B

#define FANLEVEL_COUNT 16

typedef struct {
    uint8_t ts : 1; // require to send tsid
    uint8_t mode : 2;
    uint8_t polar : 1;
    uint8_t _rsv: 4;
    uint8_t m1data;
    fanlevel_t levels[FANLEVEL_COUNT]; // 32B
    uint8_t tsid[8]; // Temperature Sensor ID
} fanctrl_t; // 42B

typedef struct {
    uint16_t vid; // USB Vendor ID
    uint16_t pid; // USB Product ID
    uint8_t console : 1; // USB Console
    uint8_t manExt : 1; // USB Manufacturer String Extended Flag (If set, the string will be extended with \0 to form a unicode string)
    uint8_t prdExt : 1; // USB Product String Extended Flag (If set, the string will be extended with \0 to form a unicode string)
    uint8_t _rsv0:1;
    uint8_t manLen : 4; // USB Manufacturer String Length
    uint8_t prdLen : 5; // USB Product String Length
    uint8_t _rsv1:3;
    uint8_t man[10]; // USB Manufacturer String
    uint8_t prd[20]; // USB Product String
} usb_t;

typedef struct {
    uint16_t magic;
    uint8_t _rsv;
    uint8_t pwmdiv;
    usb_t usb;
    uint16_t serial; // SLL for 4 bitss
    fanctrl_t fan[2];
    uint16_t _rsv2;
} sysctrl_t;

#ifndef SYSCFG_MAGIC
#define SYSCFG_MAGIC 0xdebf
#endif


extern xdata sysctrl_t sysctrl;

/*******************************************************************************
 * Function Name  : WriteDataFlash(UINT16 Addr,PUINT8 buf,UINT8 len)
 * Description    : DataFlash–¥
 * Input          : UINT16 Addr£¨PUINT16 buf,UINT8 len
 * Output         : None
 * Return         :
 *******************************************************************************/
UINT8 WriteDataFlash(UINT8 Addr, PUINT8 buf, UINT8 len);

/*******************************************************************************
 * Function Name  : ReadDataFlash(UINT8 Addr,UINT8 len,PUINT8 buf)
 * Description    : ∂¡DataFlash
 * Input          : UINT16 Addr PUINT8 buf
 * Output         : None
 * Return         : UINT8 l
 *******************************************************************************/
UINT8 ReadDataFlash(UINT8 Addr, UINT8 len, PUINT8 buf);

BOOL initCfg();
BOOL saveCfg();

#endif