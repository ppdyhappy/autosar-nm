#include "can.h"

void MCU_CAN_Init(void)
{
    // add can init code here
}


/*
    return:
        0: send fail
        1: send success
*/
int MCU_CAN_Transmit(NMPDU_t* NMPDU)
{
    // add can transmit code here

    return 1;
}

// CAN 中断服务函数
void MCU_CAN_RX_IRQHandler(void)
{
    NMPDU_t NMPDU;

    // 添加填充 NMPDU

    //将报文放入缓冲区，这里确保只有网络管理报文才可以放进缓冲区
	Recv_EveryMessage(&NMPDU);
}
