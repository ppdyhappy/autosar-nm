#include "timer.h"

extern void Timer10(void);

void TimerInit(void)
{
    // add timer init period = 10ms
}


void TimerStop(void)
{
    // add stop timer code here
}

// 定时器中断服务函数
void TIME_IRQHandler(void)
{
    Timer10();
}

