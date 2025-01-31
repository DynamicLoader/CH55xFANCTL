
#include "CH554.H"
#include "DEBUG.H"

volatile UINT8 Uart_Input_Point = 0; // 循环缓冲区写入指针，总线复位需要初始化为0
volatile UINT8 Uart_Output_Point = 0; // 循环缓冲区取出指针，总线复位需要初始化为0
volatile UINT8 UartByteCount = 0; // 当前缓冲区剩余待取字节数

#ifndef COMPACT

PUINT8 CMDIN;
PUINT8 CMDOUT;
volatile BOOL CMDBUSY;
volatile UINT8 CMDLEN;
volatile UINT8 CMDOK;

UINT8X LCDIN[MAX_PACKET_SIZE];
UINT8X LCDOUT[MAX_PACKET_SIZE];
volatile UINT8 LCDLEN;
volatile BOOL LCDOK;

extern BOOL PROXYMODE;
UINT8 TermCount = 0;

void Uart1_ISR(void) interrupt INT_NO_UART1 using 1
{
    UINT8 tmp;
    if (U1RI) // 收到数据
    {

        if (PROXYMODE) {
            LCDIN[Uart_Input_Point++] = SBUF1;
            UartByteCount++; // 当前缓冲区剩余待取字节数
            if (Uart_Input_Point >= 64)
                Uart_Input_Point = 0; // 写入指针

        } else {
            tmp = SBUF1;
            if (tmp == 0xff) {
                TermCount++;
            } else {
                TermCount = 0;
            }

            if (TermCount == 3 || LCDLEN >= MAX_PACKET_SIZE) {
                if (LCDLEN > 2) {
                    LCDLEN -= 2; // The last 2 bytes are 0xff
                    LCDOK = 1;
                    U1REN = 0; // 关闭接收
                }
                TermCount = 0;
            } else {
                LCDIN[LCDLEN++] = tmp;
            }
        }
        U1RI = 0;
    }
}

#endif