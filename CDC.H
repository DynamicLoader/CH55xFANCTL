
#include "CH554.H"
#include "DEBUG.H"

// extern UINT8X Ep0Buffer[DEFAULT_ENDP0_SIZE]; // 端点0
//                                                   // OUT&IN缓冲区，必须是偶地址
// extern UINT8X Ep1Buffer[8] _at_ 0x00C0;                   // 端点1上传缓冲区
extern UINT8X Ep2Buffer[2 * MAX_PACKET_SIZE]; // 端点2 IN &
                                              // OUT缓冲区,必须是偶地
extern UINT8 UsbConfig;

extern volatile UINT8 USBByteCount; // 代表USB端点接收到的数据
extern volatile BOOL UpPoint2_Busy; // 上传端点是否忙标志
extern volatile UINT8 USBBufOutPoint;

extern volatile UINT8 Uart_Input_Point;   //循环缓冲区写入指针，总线复位需要初始化为0
extern volatile UINT8 Uart_Output_Point;  //循环缓冲区取出指针，总线复位需要初始化为0
extern volatile UINT8 UartByteCount;      //当前缓冲区剩余待取字节数
extern UINT8V CDCRSTCount;

extern code UINT8 hextable[16];

void USBDeviceCfg();
void USBDeviceEndPointCfg();
void USBDeviceIntCfg();

extern UINT8X LCDIN[MAX_PACKET_SIZE];
extern UINT8X LCDOUT[MAX_PACKET_SIZE];
extern UINT8 LCDLEN;
extern BOOL LCDOK;

#define CONIN (Ep2Buffer)
#define CONOUT (Ep2Buffer + MAX_PACKET_SIZE)
#define CONBUSY (UpPoint2_Busy)
#define CONLEN (USBByteCount)
#define CONOK (UsbConfig)

#ifdef COMPACT
#define CMDIN CONIN
#define CMDOUT CONOUT
#define CMDBUSY CONBUSY
#define CMDLEN CONLEN
#define CMDOK CONOK
#else
extern PUINT8 CMDIN;
extern PUINT8 CMDOUT;
extern volatile BOOL CMDBUSY;
extern volatile UINT8 CMDLEN;
extern volatile UINT8 CMDOK;
#endif
