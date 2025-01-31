#include "CH554.H"
#include "DEBUG.H"
#include "DataFlash.H"
#include "CDC.H"
#include "Timer.H"
#include "PWM.H"
#include "DS18B20.H"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef COMPACT
#define DETAIL_HELP
#endif

xdata sysctrl_t sysctrl;
extern volatile UINT16I FANFreq[2];
extern volatile UINT8 FANSet[2];
BOOL UsbConsole;
extern volatile UINT16I temperature[2];
volatile BOOL tsNeedRom;

volatile BOOL mutexFan = 0;
BOOL currentCON = 1; // 1 for con, 0 for lcd
BOOL PROXYMODE = 0;
UINT16I LCDCMDCount = 0;

void consoleSend(uint8_t len)
{
    UINT8 i;
    if (currentCON)
        if (UsbConsole) {
            CONBUSY = 1;
            UEP2_T_LEN = len;
            UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK;
        } else {
            // ES = 0;
            for (i = 0; i < len; i++) {
                // SBUF = CMDOUT[i];
                // while (!TI)
                //     ;
                // TI = 0;
                CH554UART0SendByte(CMDOUT[i]);
            }
            // ES = 1;
        }
    else { // LCD
        // IE_UART1 = 0;
        for (i = 0; i < len; ++i) {
            // SBUF1 = CMDOUT[i];
            // while (!U1TI)
            //     ;
            // U1TI = 0;
            CH554UART1SendByte(CMDOUT[i]);
        }
        // IE_UART1 = 1;
    }
}

void consoleDone()
{
    if (currentCON) {
        CONLEN = 0;
        if (UsbConsole) {
            UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_R_RES | UEP_R_RES_ACK;
        } else {
            CONOK = 0;
            REN = 1;
        }
    } else {
#ifndef COMPACT
        LCDLEN = 0;
        LCDOK = 0;
        U1REN = 1;
#endif
    }
}

void waitConsole()
{
    if (currentCON) {
        while (CONBUSY)
            ;
    }
}

void consoleSendStr(const char* str)
{
    uint16_t len = strlen(str), i;
    waitConsole();
    for (i = 0; i < len; i += 64) {
        memcpy(CMDOUT, str + i, MIN(64, len - i));
        consoleSend(MIN(64, len - i));
        waitConsole();
    }
}

void proxySerial()
{
#ifndef COMPACT
    UINT8 lenth;
    UINT8 Uart_Timeout = 0;
    while (1) {
        if (UsbConfig) {
            F0 = 0;
            if (USBByteCount) // USB接收端点有数据
            {
                CH554UART1SendByte(Ep2Buffer[USBBufOutPoint++]);
                USBByteCount--;
                if (USBByteCount == 0)
                    UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_R_RES | UEP_R_RES_ACK;
            }
            if (UartByteCount)
                Uart_Timeout++;
            if (!UpPoint2_Busy) // 端点不繁忙（空闲后的第一包数据，只用作触发上传）
            {
                lenth = UartByteCount;
                if (lenth > 0) {
                    if (lenth > 29 || Uart_Timeout > 50) {
                        Uart_Timeout = 0;
                        if (Uart_Output_Point + lenth > 64)
                            lenth = 64 - Uart_Output_Point;
                        IE_UART1 = 0;
                        UartByteCount -= lenth;
                        IE_UART1 = 1;
                        // 写上传端点
                        memcpy(Ep2Buffer + MAX_PACKET_SIZE, &LCDIN[Uart_Output_Point], lenth);
                        UEP2_T_LEN = lenth; // 预使用发送长度一定要清空87
                        UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK; // 应答ACK
                        UpPoint2_Busy = 1;
                        Uart_Output_Point += lenth;
                        if (Uart_Output_Point >= 64)
                            Uart_Output_Point = 0;
                    }
                }
            }
        }
    }
#endif
}

void init()
{
    if (RESET_KEEP & RSTFLAG_UPDATE) {
        RESET_KEEP = 0;
        (*((void (*)())((PUINT8C)0x3800)))();
        while (1)
            ;
    }

    if (RESET_KEEP & RSTFLAG_PROXYSERIAL) {
        RESET_KEEP = 0;
        PROXYMODE = 1;
    }

    CfgFsys(); // CH559时钟选择配置
    mDelaymS(5); // 修改主频等待内部晶振稳定,必加

    if (RESET_KEEP & RSTFLAG_FACTORY) {
        memset(&sysctrl, 0xff, sizeof(sysctrl_t));
        saveCfg();
        RESET_KEEP &= ~RSTFLAG_FACTORY;
    }

    initCfg();
    UsbConsole = sysctrl.usb.console;

    mInitSTDIO(((UINT32)sysctrl.serial) << 4); // 串口0 使用 Timer1,可以用于调试

#ifndef COMPACT
    UART1Setup(); // 串口1 初始化
#endif

    if (!PROXYMODE) {
        // Timers init
        mTimer0Clk12DivFsys(); // 定时器0时钟12分频，1MHz
        mTimer_x_ModInit(0, 1); // 定时器0,模式31，16位
        mTimer_x_SetData(0, 10000); // 定时器0赋初值

        mTimer2Clk12DivFsys(); // 定时器2时钟12分频
        CAP1Alter(); // CAP1由P10 映射到P14
        CAP2Alter(); // CAP2由P11 映射RST
        CAP1Init(3);
        CAP2Init(3);

        ET0 = 1;
        ET2 = 1;
        mTimer0RunCTL(1);

        PX0 = 1;
        PX1 = 1;
        PT0 = 1;
        PT2 = 1;

        // PWM
        P1_MOD_OC &= ~(bPWM1 | bPWM2); // 设置PWM引脚为推挽输出
        P1_DIR_PU |= bPWM1 | bPWM2;
        SetPWMClk(sysctrl.pwmdiv); // PWM时钟配置	，Fsys/256/4分频
        ForceClearPWMFIFO(); // 强制清除PWM FIFO和COUNT
        CancleClearPWMFIFO(); // 取消清除PWM FIFO和COUNT
        setPWM();

        CH554WatchDogFeed(0);
        CH554WatchDog(1); // Enable Watchdog for reset the system
    }

    if (UsbConsole || PROXYMODE) {
        USBDeviceCfg();
        USBDeviceEndPointCfg(); // 端点配置
        USBDeviceIntCfg(); // 中断初始化
        UEP0_T_LEN = 0;
        UEP1_T_LEN = 0; // 预使用发送长度一定要清空
        UEP2_T_LEN = 0; // 预使用发送长度一定要清空
    } else {
        ES = 1; // 使能串口中断
    }
    EA = 1; // 允许单片机中断
    if (PROXYMODE) {
        proxySerial(); // NO RETURN
    }
}

void main()
{
    UINT16 i, j, k, l, m;
    init();

#ifdef DE_PRINTF
    puts("start...");
#endif
    while (1) {
        CH554WatchDogFeed(0);

        // DS18B20
        if (ds == 0) { // 0
            DS18B20_ConvertT();
            tsNeedRom = sysctrl.fan[0].ts && sysctrl.fan[1].ts;
            CH554WatchDogFeed(0);
            ET0 = 0;
            ds++;
            ET0 = 1;
        }

        if (ds == 128) { // 2s
            for (i = 0; i < 2; ++i) {
                if (sysctrl.fan[i].ts) {
                    if (tsNeedRom) {
                        temperature[i] = DS18B20_ReadT(sysctrl.fan[i].tsid);
                    } else {
                        temperature[0] = DS18B20_ReadT(NULL);
                        temperature[1] = temperature[0]; // sync
                    }
                }
                CH554WatchDogFeed(0);
            }
            ET0 = 0;
            ds++;
            ET0 = 1;
        }

#ifndef COMPACT
        currentCON = ~currentCON;
        if (currentCON) {
            CMDIN = CONIN;
            CMDOUT = CONOUT;
            CMDBUSY = CONBUSY;
            CMDLEN = CONLEN;
            CMDOK = CONOK;
        } else {
            CMDIN = LCDIN;
            CMDOUT = LCDOUT;
            CMDBUSY = 0;
            CMDLEN = LCDLEN;
            CMDOK = LCDOK;
        }
#endif
        if (!CMDOK || !CMDLEN || CMDBUSY)
            continue;
#ifndef COMPACT
        // LCD commands, preprocess
        if (!currentCON) {
            LCDCMDCount++;
            if (CMDIN[0] != '/') {
                if (CMDIN[0] == 0xD0) { // Get  CMD Count
                    l = sprintf(CMDOUT, "CmdC=%hu\xff\xff\xff", LCDCMDCount);
                    consoleSend(l);
                    consoleDone();
                    continue;
                }
                if (CMDIN[0] == 0xD1) { // Poll status of fan1
                    l = sprintf(CMDOUT, "T1=%hu\xff\xff\xff", temperature[0]);
                    l += sprintf(CMDOUT + l, "N1=%bu\xff\xff\xff", FANSet[0]);
                    l += sprintf(CMDOUT + l, "F1=%hu\xff\xff\xff", FANFreq[0]);
                    i = sysctrl.fan[0].mode;
                    l += sprintf(CMDOUT + l, "M1=%hu\xff\xff\xff", i);
                    consoleSend(l);
                    consoleDone();
                    continue;
                }
                if (CMDIN[0] == 0xD2) {
                    // fan2
                    l = sprintf(CMDOUT, "T2=%hu\xff\xff\xff", temperature[1]);
                    l += sprintf(CMDOUT + l, "N2=%bu\xff\xff\xff", FANSet[1]);
                    l += sprintf(CMDOUT + l, "F2=%hu\xff\xff\xff", FANFreq[1]);
                    i = sysctrl.fan[1].mode;
                    l += sprintf(CMDOUT + l, "M2=%hu\xff\xff\xff", i);
                    consoleSend(l);
                    consoleDone();
                    continue;
                }
                consoleDone();
                continue;
            }
        }
#endif
        memset(CMDIN + CMDLEN, 0, MAX_PACKET_SIZE - CMDLEN); // Clear the rest of buffer
        if (memcmp(CMDIN, "/fanstat", 8) == 0) {
            i = sscanf(CMDIN + 9, "%hu", &j);
            // #ifdef DETAIL_HELP
            //             if (i == 0 || i == EOF) {
            //                 consoleSendStr("Usage: /fanstat [fanid] - Get fan stat of id \n");
            //                 consoleDone();
            //                 continue;
            //             }
            // #endif
            if (j == 0 || j > 2) {
                consoleSendStr("No such fan\n");
                consoleDone();
                continue;
            }
            if (i == 1) {
                i = sprintf(CMDOUT, "T=%hx, N=%bu, F=%hu\n", temperature[j - 1], FANSet[j - 1], FANFreq[j - 1]); // Temperature, current value, feedback value
                consoleSend(i);
                consoleDone();
                continue;
            }
            consoleSendStr("Invalid args\n");
            consoleDone();
            continue;
        }

        if (memcmp(CMDIN, "/fanlevel", 9) == 0) {
            i = sscanf(CMDIN + 9, "%hu %hu %hu %hu", &j, &k, &l, &m);
            // #ifdef DETAIL_HELP
            //             if (i == 0 || i == EOF) {
            //                 consoleSendStr("Usage: /fanlevel [fanid] - Get fan levels of id \n"
            //                                "       /fanlevel [fanid] [level] [bound] [value] - Set fan level of id to given value\n");
            //                 consoleDone();
            //                 continue;
            //             }
            // #endif
            if (j == 0 || j > 2) {
                consoleSendStr("No such fan\n");
                consoleDone();
                continue;
            }
            if (i == 1) {
                // We dump the fan levels of id
                for (m = 0; m < FANLEVEL_COUNT + (sysctrl.fan[j - 1].ts ? 0 : 4); m += 4) {
                    l = 0;
                    for (k = m; k < m + 4; ++k) {
                        i = *(PUINT16X)&sysctrl.fan[j - 1].levels[k];
                        l += sprintf(CMDOUT + l, "#%hu %hu %hu  ", k, (i >> 8) & 0xFF, i & 0xFF);
                        k++;
                        i = *(PUINT16X)&sysctrl.fan[j - 1].levels[k];
                        l += sprintf(CMDOUT + l, "#%hu %hu %hu\n", k, (i >> 8) & 0xFF, i & 0xFF);
                    }
                    consoleSend(l);
                }
                consoleDone();
                continue;
            }
            if (i == 4) {
                // We enable or disable the fan level control
                if (k > (FANLEVEL_COUNT + (sysctrl.fan[j - 1].ts ? 0 : 4) - 1) || l > 255 || m > 255) {
                    consoleSendStr("Param too big\n");
                    consoleDone();
                    continue;
                }
                mutexFan = 1;
                sysctrl.fan[j - 1].levels[k].bound = l;
                sysctrl.fan[j - 1].levels[k].duty = m;
                mutexFan = 0;
                consoleSendStr("OK\n");
                consoleDone();
                continue;
            }
            consoleSendStr("Invalid args\n");
            consoleDone();
            continue;
        }

        if (memcmp(CMDIN, "/fanmode", 8) == 0) {
            i = sscanf(CMDIN + 8, "%hu %hu %hu", &j, &k, &l);
            // #ifdef DETAIL_HELP
            //             if (i == 0 || i == EOF) {
            //                 consoleSendStr("Usage: /fanmode [fanid] - Get fan mode of id \n"
            //                                "       /fanmode [fanid] [mode] [speed] - Set fan mode of id to given mode\n");
            //                 consoleDone();
            //                 continue;
            //             }
            // #endif
            if (j == 0 || j > 2) {
                consoleSendStr("No such fan\n");
                consoleDone();
                continue;
            }
            if (i == 1) {
                // We dump the fan mode of id
                k = sysctrl.fan[j - 1].mode;
                l = (k == 1 ? sysctrl.fan[j - 1].m1data : 0);
                i = sprintf(CMDOUT, "%hu, %hu\n", k, l);
                consoleSend(i);
                consoleDone();
                continue;
            }
            if (i >= 2) {
                if ((k & 0x3) > 2) {
                    consoleSendStr("Invalid mode\n");
                    consoleDone();
                    continue;
                }
                if (i == 3 && (k & 0x3) == 1) { // static mode, we need to set the value
                    if (l > 255) {
                        consoleSendStr("Speed too high\n");
                        consoleDone();
                        continue;
                    }
                    mutexFan = 1;
                    sysctrl.fan[j - 1].m1data = l;
                    mutexFan = 0;
                }
                mutexFan = 1;
                sysctrl.fan[j - 1].mode = k & 0x3;
                sysctrl.fan[j - 1].polar = (k & 0x4) >> 2;
                setPWM();
                mutexFan = 0;
                consoleSendStr("OK\n");
                consoleDone();
                continue;
            }
            consoleSendStr("Invalid args\n");
            consoleDone();
            continue;
        }

        if (memcmp(CMDIN, "/pwmdiv", 7) == 0) {
            j = sscanf(CMDIN + 7, "%hu", &i);

            if (j == 0 || j == EOF) {
                // j = ;
                i = sprintf(CMDOUT, "%bu\n", sysctrl.pwmdiv);
                consoleSend(i);
                consoleDone();
                continue;
            }

            if (i < 1 || i > 255) {
                consoleSendStr("Invalid value\n");
                consoleDone();
                continue;
            }
            sysctrl.pwmdiv = i;
            SetPWMClk(i);
            consoleSendStr("OK\n");
            consoleDone();
            continue;
        }

        if (memcmp(CMDIN, "/tscon", 6) == 0) {
            i = sscanf(CMDIN + 6, "%hu %hu", &j, &k);
            // #ifdef DETAIL_HELP
            //             if (i == 0 || i == EOF) {
            //                 consoleSendStr("Usage: /tscon [fanid] [0|x]- Configure sensor id to disable or current read\n");
            //                 consoleDone();
            //                 continue;
            //             }
            // #endif
            if (j == 0 || j > 2) {
                consoleSendStr("No such fan\n");
                consoleDone();
                continue;
            }
            if (i == 1) {
                if (sysctrl.fan[j - 1].ts) {
                    // consoleSendStr("Enabled\n");
                    k = (UINT16)sysctrl.fan[j - 1].tsid;
                    l = sprintf(CMDOUT, "1, 0x%lx %lx\n", *(UINT32X*)(k), *(UINT32X*)(k+4));
                    consoleSend(l);
                } else {
                    consoleSendStr("0\n");
                }
                consoleDone();
                continue;
            }
            if (i == 2) {
                mutexFan = 1;
                if (k == 0) {
                    memset(sysctrl.fan[j - 1].tsid, 0, 8);
                    sysctrl.fan[j - 1].ts = 0;
                } else {
                    DS18B20_ReadRomCode(sysctrl.fan[j - 1].tsid);
                    sysctrl.fan[j - 1].ts = 1;
                }
                mutexFan = 0;
                consoleSendStr("OK\n");
                consoleDone();
                continue;
            }
            consoleSendStr("Invalid args\n");
            consoleDone();
            continue;
        }

        if (memcmp(CMDIN, "/console", 8) == 0) {
            i = sscanf(CMDIN + 8, "%hu", &j);
            // #ifdef DETAIL_HELP
            //             if (i == 0 || i == EOF) {
            //                 consoleSendStr("Usage: /console [0|1] - Set console to USB or UART\n"
            //                                "       /console [baud] - Set UART baudrate to baud * 16\n");
            //                 consoleDone();
            //                 continue;
            //             }
            // #endif
            if (i == 1) {
                if (j == 0 || j == 1) {
                    sysctrl.usb.console = j;
                    consoleSendStr("Console set to ");
                    consoleSendStr(j ? "USB\n" : "UART\n");
                    consoleDone();
                    continue;
                }

                if (j >= 150 && j <= 3600) {
                    sysctrl.serial = j;
                    consoleSendStr("OK\n");
                    consoleDone();
                    continue;
                }
                consoleSendStr("Out of Range\n");
                consoleDone();
                continue;
            }
            consoleSendStr("Invalid args\n");
            consoleDone();
            continue;
        }

        if (memcmp(CMDIN, "/help", 5) == 0) {
            // #ifdef DETAIL_HELP
            // consoleSendStr("Commands:\n"
            //                "- Global:\n"
            //                "/RESET    - Reset the device\n"
            //                "/RESETUP  - Reset to update mode\n"
            //                "/ERASE    * Erase (or cancel) config after reset\n"
            //                "/console  * Set console parameters\n"
            //                "/help     - Show this help\n"
            //                "/save     - Save config\n"
            //                "\n- Fan Control:\n"
            //                "/pwmdiv   - Set PWM clock divider in range [1:255]\n"
            //                "/tscon    - Configure the temperature sensor\n"
            //                "/fanstat - Get fan stat\n"
            //                "/fanlevel - Get or Set fan level\n"
            //                "/fanmode  - Get or Set fan mode\n"
            //                "\n- USB Settings:\n"
            //                "/usbid    * Set USB Vendor and Product ID\n"
            //                "/usbman   * Set USB Manufacturer (IN HEX)\n"
            //                "/usbprd   * Set USB Product (IN HEX)\n"
            //                "\n Note that * commands require reset to take effect\n"
            //                "Visit https://t.dyldr.top/s/3 for details\n");
            // #else
            consoleSendStr("Commands:\n"
                           "/RESET      /RESETUP   /ERASE\n"
                           "/console    /help      /save\n"
                           "/pwmdiv     /tscon     /fanstat\n"
                           "/fanlevel   /fanmode   /usbid\n"
                           "/usbman     /usbprd\n"
                           "\nDetails: https://t.dyldr.top/s/3\n");
            // #endif
            consoleDone();
            continue;
        }

        if (memcmp(CMDIN, "/save", 5) == 0) {
            saveCfg();
            consoleSendStr("OK\n");
            consoleDone();
            continue;
        }

        if (memcmp(CMDIN, "/ERASE", 6) == 0) {
            RESET_KEEP ^= RSTFLAG_FACTORY;
            consoleSendStr("ERASE = ");
            if (RESET_KEEP & RSTFLAG_FACTORY) {
                consoleSendStr("1\n");
            } else {
                consoleSendStr("0\n");
            }
            consoleDone();
            continue;
        }

        if (memcmp(CMDIN, "/RESET", 6) == 0) {
            if (memcmp(CMDIN + 6, "UP", 2) == 0)
                // https://www.wch.cn/bbs/thread-87280-1.html
                RESET_KEEP |= RSTFLAG_UPDATE;
            EA = 0;
            SAFE_MOD = 0x55;
            SAFE_MOD = 0xAA; // 进入安全模式
            GLOBAL_CFG |= bSW_RESET; // 软件复位
            EA = 1;
            consoleSendStr("Failed\n");
            consoleDone();
            continue;
        }

        if (memcmp(CMDIN, "/usbid", 6) == 0) {
            i = sscanf(CMDIN + 6, "%hx %hx", &j, &k);
            // #ifdef DETAIL_HELP
            //             if (i == 0 || i == EOF) {
            //                 consoleSendStr("Usage: /usbid [vid] [pid] - Set USB VID and PID\n");
            //                 consoleDone();
            //                 continue;
            //             }
            // #endif
            if (i == 2) {
                sysctrl.usb.vid = j;
                sysctrl.usb.pid = k;
                consoleSendStr("OK\n");
                consoleDone();
                continue;
            }
            consoleSendStr("Invalid args\n");
            consoleDone();
            continue;
        }

        if (memcmp(CMDIN, "/usbman", 7) == 0) {
            memset(CMDOUT, 0, MAX_PACKET_SIZE);
            i = sscanf(CMDIN + 7, "%hx %s", &j, CMDOUT);
            // #ifdef DETAIL_HELP
            //             if (i < 2 || i == EOF) {
            //                 consoleSendStr("Usage: /usbman <flag> <hex string> - Set USB Manufacturer Info\n");
            //                 consoleDone();
            //                 continue;
            //             }
            // #endif
            if (i == 2) {
                k = 0;
                while (sscanf(CMDOUT + 2 * k, "%2hhx", &l) == 1 && k < sizeof(sysctrl.usb.man)) {
                    sysctrl.usb.man[k++] = l;
                }
                sysctrl.usb.manLen = k;
                sysctrl.usb.manExt = j & 1;
                consoleSendStr("OK\n");
                consoleDone();
                continue;
            }
            consoleSendStr("Invalid args\n");
            consoleDone();
            continue;
        }

        if (memcmp(CMDIN, "/usbprd", 7) == 0) {
            memset(CMDOUT, 0, MAX_PACKET_SIZE);
            i = sscanf(CMDIN + 7, "%hx %s", &j, CMDOUT);
            // #ifdef DETAIL_HELP
            //             if (i < 2 || i == EOF) {
            //                 consoleSendStr("Usage: /usbprd <flag> <hex string> - Set USB Product Info\n");
            //                 consoleDone();
            //                 continue;
            //             }
            // #endif
            if (i == 2) {
                k = 0;
                while (sscanf(CMDOUT + 2 * k, "%2hhx", &l) == 1 && k < sizeof(sysctrl.usb.prd)) {
                    sysctrl.usb.prd[k++] = l;
                }
                sysctrl.usb.prdLen = k;
                sysctrl.usb.prdExt = j & 1;
                consoleSendStr("OK\n");
                consoleDone();
                continue;
            }
            consoleSendStr("Invalid args\n");
            consoleDone();
            continue;
        }

        consoleSendStr("Invalid Command: ");
        waitConsole();
        memcpy(CMDOUT, CMDIN, CMDLEN);
        consoleSend(CMDLEN);
        consoleDone();
    }
}
