#ifndef _DRIVER_COMMON_H_
#define _DRIVER_COMMON_H_

#include "autosarNM.h"

#define MCU_TYPE_MACRO

#ifdef MCU_TYPE_MACRO
#include "timer.h"
#include "can.h"
// #include "uart.h"
#endif

/*平台无关的定义*/
typedef  int TimerOutFlag_t;

#define NMID (0x0cffA019)
#define NMSNI (0x19)

#define TIMER_PERIOD    (10)

#define T_REPEAT_MESSAGE    (1500)
#define T_NM_TIMEOUT (2000)
#define T_WAIT_BUS_SLEEP (2000)
#define T_START_NM_TX   (50)
#define T_START_App_TX (20)
#define T_NM_ImmediateCycleTime (20)
#define T_NM_MessageCycle (500)
#define T_WakeUp (100)

#define N_ImmediateNM_TIMES (5)

#define NM_TIMER_REPEAT_MSG (0)
#define NM_TIMER_NM_TIMEOUT (1)
#define NM_TIMER_WAIT_BUS_SLEEP (2)
#define NM_TIMER_IMMEDIATE_CYCLE_TIME   (3)
#define NM_TIMER_NM_MSG_CYCLE   (4)

//FIFO大小
#define FIFOMAX (20)


//接受报文FIFO
typedef struct {
	NMPDU_t MSGs[FIFOMAX];//FIFO缓冲区
	char Total;//FIFO中的报文数量
	char Head;//指向队列头
	char Tail;//指向队尾
	char FullFlag;//缓冲区满
	char EmptyFlag;//缓冲区空
	char(*GetMsg) (NMPDU_t* msg); //从缓冲区获取报文
	char(*SetMsg) (NMPDU_t* smsg); //放入报文到缓冲区
	void(*ClearBuff)(void);//清空FIFO
}RecvFIFO_t;

char SetToFIFO(NMPDU_t* msg);
char GetFromFIFO(NMPDU_t* msg);
void ClearFIFO(void);
void InitPlatform(void);
void InitNMPDU(NMPDU_t* NMPDU);
NMTypeU8_t TX_CAN_Transmit(NMPDU_t* NMPDU);
TimerOutFlag_t GetTimerIsOut(NMTimerType_t TimerType);
void ClcTimerOutFlag(NMTimerType_t TimerType);
void Recv_EveryMessage(NMPDU_t* p_Msg);

#endif