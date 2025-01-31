
/********************************** (C) COPYRIGHT *******************************
 * File Name          : DataFlash.C
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2017/01/20
 * Description        : CH554 DataFlash字节读写函数定义
 *******************************************************************************/

#include "..\Public\CH554.H"
#include "..\Public\Debug.H"
#include "DataFlash.H"
#include <string.h>

/*******************************************************************************
 * Function Name  : WriteDataFlash(UINT8 Addr,PUINT8 buf,UINT8 len)
 * Description    : DataFlash写
 * Input          : UINT8 Addr，PUINT16 buf,UINT8 len
 * Output         : None
 * Return         : UINT8 i 返回写入长度
 *******************************************************************************/
UINT8 WriteDataFlash(UINT8 Addr, PUINT8 buf, UINT8 len)
{
    UINT8 i;
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA; // 进入安全模式
    GLOBAL_CFG |= bDATA_WE; // 使能DataFlash写
    SAFE_MOD = 0; // 退出安全模式
    ROM_ADDR_H = DATA_FLASH_ADDR >> 8;
    Addr <<= 1;
    for (i = 0; i < len; i++) {
        ROM_ADDR_L = Addr + i * 2;
        ROM_DATA_L = *(buf + i);
        if (ROM_STATUS & bROM_ADDR_OK) { // 操作地址有效
            ROM_CTRL = ROM_CMD_WRITE; // 写入
        }
        if ((ROM_STATUS ^ bROM_ADDR_OK) > 0)
            return i; // 返回状态,0x00=success,  0x02=unknown command(bROM_CMD_ERR)
    }
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA; // 进入安全模式
    GLOBAL_CFG &= ~bDATA_WE; // 开启DataFlash写保护
    SAFE_MOD = 0; // 退出安全模式
    return i;
}

/*******************************************************************************
 * Function Name  : ReadDataFlash(UINT8 Addr,UINT8 len,PUINT8 buf)
 * Description    : 读DataFlash
 * Input          : UINT8 Addr UINT8 len PUINT8 buf
 * Output         : None
 * Return         : UINT8 i 返回写入长度
 *******************************************************************************/
UINT8 ReadDataFlash(UINT8 Addr, UINT8 len, PUINT8 buf)
{
    UINT8 i;
    ROM_ADDR_H = DATA_FLASH_ADDR >> 8;
    Addr <<= 1;
    for (i = 0; i < len; i++) {
        ROM_ADDR_L = Addr + i * 2; // Addr必须为偶地址
        ROM_CTRL = ROM_CMD_READ;
        //     if ( ROM_STATUS & bROM_CMD_ERR ) return( 0xFF );                        // unknown command
        *(buf + i) = ROM_DATA_L;
    }
    return i;
}

BOOL initCfg()
{
    BOOL x = (ReadDataFlash(0, sizeof(sysctrl_t), (PUINT8)&sysctrl) == sizeof(sysctrl_t));
    if (!x || sysctrl.magic != SYSCFG_MAGIC) {
        sysctrl.magic = SYSCFG_MAGIC;
        // Global Settings
        sysctrl.pwmdiv = 8;

        // USB Settings
        sysctrl.usb.vid = 0x8680;
        sysctrl.usb.pid = 0x2107;
        memcpy(sysctrl.usb.man, "INTEL", 5);
        memcpy(sysctrl.usb.prd, "Adv. Fan Controller", 19);
        sysctrl.usb.manLen = 5;
        sysctrl.usb.manExt = 1;
        sysctrl.usb.prdLen = 19;
        sysctrl.usb.prdExt = 1;
        sysctrl.usb.console = 1;
        sysctrl.serial = UART0_BUAD / 16;
        memset(&sysctrl.fan, 0, sizeof(sysctrl.fan));
        return FALSE;
    }
    return TRUE;
}

BOOL saveCfg()
{
    return WriteDataFlash(0, (PUINT8)&sysctrl, sizeof(sysctrl)) == sizeof(sysctrl);
}