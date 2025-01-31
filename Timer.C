
/********************************** (C) COPYRIGHT *******************************
* File Name          : Timer.C
* Author             : WCH
* Version            : V1.0
* Date               : 2017/01/20
* Description        : CH554 Time 初始化、定时器、计数器赋值、T2捕捉功能开启函数等
                       定时器中断函数
*******************************************************************************/
#include "..\Public\CH554.H"
#include "..\Public\Debug.H"
#include "Timer.H"
#include "PWM.H"
#include "DataFlash.H"
#include "DS18B20.H"
#include "CDC.H"
#include "stdio.h"

#pragma NOAREGS

volatile UINT16I CapCount[2];
// volatile UINT16I CapV[2], CapT[2];
volatile UINT16I FANFreq[2];
volatile UINT8 FANSet[2];
extern volatile BOOL mutexFan;

volatile BOOL CapCycle = 1;
volatile UINT8I ds;
volatile UINT16I temperature[2];


/*******************************************************************************
* Function Name  : mTimer_x_ModInit(UINT8 x ,UINT8 mode)
* Description    : CH554定时计数器x模式设置
* Input          : UINT8 mode,Timer模式选择
                   0：模式0，13位定时器，TLn的高3位无效
                   1：模式1，16位定时器
                   2：模式2，8位自动重装定时器
                   3：模式3，两个8位定时器  Timer0
                   3：模式3，Timer1停止
* Output         : None
* Return         : 成功  SUCCESS
                   失败  FAIL
*******************************************************************************/
UINT8 mTimer_x_ModInit(UINT8 x, UINT8 mode)
{
    if (x == 0) {
        TMOD = TMOD & 0xf0 | mode;
    } else if (x == 1) {
        TMOD = TMOD & 0x0f | (mode << 4);
    } else if (x == 2) {
        RCLK = 0;
        TCLK = 0;
        CP_RL2 = 0;
    } // 16位自动重载定时器
    else
        return FAIL;
    return SUCCESS;
}

/*******************************************************************************
 * Function Name  : mTimer_x_SetData(UINT8 x,UINT16 dat)
 * Description    : CH554Timer0 TH0和TL0赋值
 * Input          : UINT16 dat;定时器赋值
 * Output         : None
 * Return         : None
 *******************************************************************************/
void mTimer_x_SetData(UINT8 x, UINT16 dat)
{
    UINT16 tmp;
    tmp = 65536 - dat;
    if (x == 0) {
        TL0 = tmp & 0xff;
        TH0 = (tmp >> 8) & 0xff;
    } else if (x == 1) {
        TL1 = tmp & 0xff;
        TH1 = (tmp >> 8) & 0xff;
    } else if (x == 2) {
        RCAP2L = TL2 = tmp & 0xff; // 16位自动重载定时器
        RCAP2H = TH2 = (tmp >> 8) & 0xff;
    }
}

/*******************************************************************************
* Function Name  : CAP2Init(UINT8 mode)
* Description    : CH554定时计数器2 T2EX引脚捕捉功能初始化
                   UINT8 mode,边沿捕捉模式选择
                   0:T2ex从下降沿到下一个下降沿
                   1:T2ex任意边沿之间
                   3:T2ex从上升沿到下一个上升沿
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void CAP2Init(UINT8 mode)
{
    RCLK = 0;
    TCLK = 0;
    C_T2 = 0;
    EXEN2 = 1;
    CP_RL2 = 1; // 启动T2ex的捕捉功能
    T2MOD |= mode << 2; // 边沿捕捉模式选择
}

/*******************************************************************************
* Function Name  : CAP1Init(UINT8 mode)
* Description    : CH554定时计数器2 T2引脚捕捉功能初始化T2
                   UINT8 mode,边沿捕捉模式选择
                   0:T2ex从下降沿到下一个下降沿
                   1:T2ex任意边沿之间
                   3:T2ex从上升沿到下一个上升沿
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void CAP1Init(UINT8 mode)
{
    RCLK = 0;
    TCLK = 0;
    CP_RL2 = 1;
    C_T2 = 0;
    T2MOD = T2MOD & ~T2OE | (mode << 2) | bT2_CAP1_EN; // 使能T2引脚捕捉功能,边沿捕捉模式选择
}

#ifdef T0_INT
/*******************************************************************************
 * Function Name  : mTimer0Interrupt()
 * Description    : CH554定时计数器0定时计数器中断处理函数
 *******************************************************************************/
void mTimer0Interrupt(void) interrupt INT_NO_TMR0 using 2 // timer0中断服务程序,使用寄存器组2
{
    UINT8 i, j, t;
    // long k, b;
    if ((ds & 0x3f) == 0) {
        // if (CapCycle) {
        // CapCycle = 0;
        // for (i = 0; i < 2; ++i) {
        //     // CapT[i] = 0;
        //     // CapV[i] = 0;
        //     CapCount[i] = 0;
        // }
        // mTimer_x_SetData(2, 0);
        // mTimer2RunCTL(1);
        // } else {
        mTimer2RunCTL(0);
        EXF2 = 0;
        CAP1F = 0;
        TF2 = 0;
        CapCycle = 1;

        // Here we calculate the frequency
        for (i = 0; i < 2; ++i) {
            if (CapCount[i]) {
                // FANFreq[i] = CapCount[i] / CapT[i];
                FANFreq[i] = CapCount[i];
            } else {
                FANFreq[i] = 0;
            }
        }
        // printf("FAN1: %4x %8x -> %4x, ", Cap1Count, Cap1T,FANFreq[0]);
        // printf("FAN2: %4x %8x -> %4x\n", Cap2Count, Cap2T,FANFreq[1]);
        // }
    }

    if (!mutexFan) {
        

        if ((ds& 0xf) == 0) { // 250ms
            for (i = 0; i < 2; ++i) {
                if (sysctrl.fan[i].mode == 1) {
                    FANSet[i] = sysctrl.fan[i].m1data;
                    continue;
                }
                if (sysctrl.fan[i].mode == 2) {
                    if (temperature[i] < 0x8000) { // Positive
                        if (temperature[i] > 0x7F0) { // too high, just use the highest
                            FANSet[i] = sysctrl.fan[i].levels[FANLEVEL_COUNT + (sysctrl.fan[i].ts ? 0 : 4)].duty;
                            continue;
                        }

                        t = temperature[i] >> 3; // on 12 bit precision, +7.4 -> +7.1
                        for (j = 0; j < FANLEVEL_COUNT + (sysctrl.fan[i].ts ? 0 : 4); ++j) {
                            if (t < sysctrl.fan[i].levels[j].bound) { // find the upper bound
                                break;
                            }
                        }
                        // if (j == 12) {
                        //     FANSet[i] = sysctrl.fan[i].levels[11].duty; // the highest
                        //     continue;
                        // }
                        if (j == 0) {
                            FANSet[i] = sysctrl.fan[i].levels[0].duty; // the lowest
                            continue;
                        }
                        // b = (sysctrl.fan[i].levels[j].duty) - (sysctrl.fan[i].levels[j - 1].duty) * (t - sysctrl.fan[i].levels[j - 1].bound); // 31.1
                        // k = b << 15; // 16.16
                        // b = (long)(sysctrl.fan[i].levels[j].bound) - (sysctrl.fan[i].levels[j - 1].bound) << 15; // 16.16
                        // k /= b; // INT
                        // FANSet[i] = sysctrl.fan[i].levels[j - 1].duty + k;
                        FANSet[i] = sysctrl.fan[i].levels[j - 1].duty;
                    } else {
                        FANSet[i] = sysctrl.fan[i].levels[0].duty; // negative, the lowest
                    }
                }
            }

            SetPWM1Dat(FANSet[0]);
            SetPWM2Dat(FANSet[1]);
        }
    }

    if ((ds & 0x3f) == 0) { // 1s, Reset the cap
        for (i = 0; i < 2; ++i) {
            // CapT[i] = 0;
            // CapV[i] = 0;
            CapCount[i] = 0;
        }
        mTimer_x_SetData(2, 0);
        mTimer2RunCTL(1);
    }

    // if (PWM_CTRL & bPWM_IF_END) {
    //     PWM_CTRL |= bPWM_IF_END;
    //     SetPWM1Dat(FANSet[0]);
    //     SetPWM2Dat(FANSet[1]);
    // }

    ds++;
    mTimer_x_SetData(0, 15625); // 15.625ms
}
#endif

#ifdef T1_INT
/*******************************************************************************
 * Function Name  : mTimer1Interrupt()
 * Description    : CH554定时计数器0定时计数器中断处理函数
 *******************************************************************************/
void mTimer1Interrupt(void) interrupt INT_NO_TMR1 using 2 // timer1中断服务程序,使用寄存器组2
{ // 方式3时，Timer1停止

    //     mTimer_x_SetData(1,0x0000);                                          //非自动重载方式需重新给TH1和TL1赋值
}
#endif

#ifdef T2_INT
/*******************************************************************************
 * Function Name  : mTimer2Interrupt()
 * Description    : CH554定时计数器2定时计数器中断处理函数
 *******************************************************************************/
void mTimer2Interrupt(void) interrupt INT_NO_TMR2 using 2
{
    mTimer2RunCTL(0); // 关定时器
#ifdef T2_CAP

    if (CAP1F) // T2电平捕捉中断标志
    {
        // CapT[0] += (UINT16)(T2CAP1 - CapV[0]);
        // CapV[0] = T2CAP1;
        CapCount[0] += 1; //(FREQ_SYS / 12);
        CAP1F = 0; // 清空T2捕捉中断标志
    }
    if (EXF2) // T2ex电平变化中断中断标志
    {
        // CapT[1] += (UINT16)(RCAP2 - CapV[1]);
        // CapV[1] = RCAP2;
        CapCount[1] += 1; //(FREQ_SYS / 12);
        EXF2 = 0;
    }
#endif
    // if (TF2) {
    //     TF2 = 0; // 清空定时器2溢出中断
    // }
    mTimer2RunCTL(1); // 开定时器
}
#endif
