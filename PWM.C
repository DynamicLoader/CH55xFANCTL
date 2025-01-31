/********************************** (C) COPYRIGHT *******************************
 * File Name          : PWM.C
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2018/02/27
 * Description        : CH554 PWM中断使能和中断处理
 *******************************************************************************/

#include "..\Public\CH554.H"
#include "..\Public\Debug.H"
#include "DataFlash.H"
#include "PWM.H"
#include "stdio.h"

#pragma NOAREGS

void setPWM()
{
    // putchar('1');

    if (sysctrl.fan[0].mode != 0) {
        // putchar('2');
        PWM1OutEnable(); // 允许PWM1输出
    } else {
        DsiablePWM1Out(); // 关闭PWM1输出
    }

    if (sysctrl.fan[1].mode != 0) {
        PWM2OutEnable(); // 允许PWM2输出
    } else {
        DisablePWM2Out(); // 关闭PWM2输出
    }

    if (sysctrl.fan[0].polar) {
        // putchar('3');
        PWM1OutPolarHighAct();
    } else {
        PWM1OutPolarLowAct();
    }
    if (sysctrl.fan[1].polar) {
        PWM2OutPolarHighAct();
    } else {
        PWM2OutPolarLowAct();
    }
}

#if PWM_INTERRUPT
/*******************************************************************************
 * Function Name  : PWMInterruptEnable()
 * Description    : PWM中断使能
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void PWMInterruptEnable()
{
    PWM_CTRL |= bPWM_IF_END | bPWM_IE_END; // 清除PWM中断，使能PWM中断
    IE_PWMX = 1;
}

/*******************************************************************************
 * Function Name  : PWMInterrupt(void)
 * Description    : PWM中断服务程序
 *******************************************************************************/
void PWMInterrupt(void) interrupt INT_NO_PWMX using 1 // PWM1&2中断服务程序,使用寄存器组1
{
    PWM_CTRL |= bPWM_IF_END; // 清除PWM中断

    // SetPWM2Dat(0x10);
    // SetPWM1Dat(0x10);
    // printf("PWM_CTRL  INT\n");                                                //开启可以用于查看是否进入中断
}
#endif
