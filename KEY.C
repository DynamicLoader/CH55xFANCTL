
#include "CH554.H"
#include "DEBUG.H"
#include "DataFlash.H"
// #include "PWM.H"

sbit KINT1 = P3 ^ 3;

void INT1_ISR(void) interrupt INT_NO_INT1 using 2
{
    mDelaymS(5);
    if (!KINT1) {
        PWM_CTRL ^= (sysctrl.fan[0].mode ? bPWM1_OUT_EN : 0) |  (sysctrl.fan[1].mode ? bPWM2_OUT_EN : 0);
    }
}