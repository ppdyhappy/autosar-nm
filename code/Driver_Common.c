#include "Driver_Common.h"

//报文缓冲区定义
static RecvFIFO_t RecvFIFO;

/*
#define NM_TIMER_REPEAT_MSG (0)
#define NM_TIMER_NM_TIMEOUT (1)
#define NM_TIMER_WAIT_BUS_SLEEP (2)
#define NM_TIMER_IMMEDIATE_CYCLE_TIME   (3)
#define NM_TIMER_NM_MSG_CYCLE   (4)
*/

//定时器设置
static char SetAlarm_REPEAT_MSG = 0;
static char SetAlarm_NM_TIMEOUT = 0;
static char SetAlarm_WAIT_BUS_SLEEP = 0;
static char SetAlarm_IMMEDIATE_CYCLE_TIME = 0;
static char SetAlarm_NM_MSG_CYCLE = 0;

//定时器计数器
static int REPEAT_MSG_Count = 0;
static int NM_TIMEOUT_Count = 0;
static int WAIT_BUS_SLEEP_Count = 0;
static int IMMEDIATE_CYCLE_TIME_Count = 0;
static int NM_MSG_CYCLE_Count = 0;

//报文缓冲区定义
static RecvFIFO_t RecvFIFO;

//定时器超时标志
static TimerOutFlag_t TimerOutFlag_REPEAT_MSG = 0;
static TimerOutFlag_t TimerOutFlag_NM_TIMEOUT = 0;
static TimerOutFlag_t TimerOutFlag_WAIT_BUS_SLEEP = 0;
static TimerOutFlag_t TimerOutFlag_IMMEDIATE_CYCLE_TIME = 0;
static TimerOutFlag_t TimerOutFlag_NM_MSG_CYCLE = 0;

void InitPlatform(void)
{
	/*缓冲区初始化*/
	RecvFIFO.GetMsg = GetFromFIFO;
	RecvFIFO.SetMsg = SetToFIFO;
	RecvFIFO.ClearBuff = ClearFIFO;
	RecvFIFO.ClearBuff();
	/*1.STM32相关的初始化*/
	#ifdef MCU_TYPE_MACRO
	/*定时器初始化*/
	TimerInit();//10ms中断一次
	/*CAN模块初始化*/
	MCU_CAN_Init();
	#endif
}

/*函数名：TX_CAN_Transmit
*参数：NMPDU
*返回值：成功 1
*说明：调用平台相关的报文发送函数
*/
NMTypeU8_t TX_CAN_Transmit(NMPDU_t* NMPDU)
{
	/*发送报文到总线*/
	#ifdef MCU_TYPE_MACRO
	return MCU_CAN_Transmit(NMPDU);
	#endif
}

//NMPDU初始化，保留位置1
void InitNMPDU(NMPDU_t* NMPDU)
{
	int i = 0;
	NMPDU->MsgID = NMID;
	NMPDU->MsgSNI = NMSNI;
	NMPDU->MsgCtl = 0x00;

	for (; i < 6; i++)
	{
		NMPDU->MsgData[i] = 0xff;
	}
}

//返回定时器是否超时,-1 失败
TimerOutFlag_t GetTimerIsOut(NMTimerType_t TimerType)
{
	switch (TimerType)
	{
	case NM_TIMER_REPEAT_MSG:
		return TimerOutFlag_REPEAT_MSG;
	case NM_TIMER_NM_TIMEOUT:
		return TimerOutFlag_NM_TIMEOUT;
	case NM_TIMER_WAIT_BUS_SLEEP:
		return TimerOutFlag_WAIT_BUS_SLEEP;
	case NM_TIMER_IMMEDIATE_CYCLE_TIME:
		return TimerOutFlag_IMMEDIATE_CYCLE_TIME;
	case NM_TIMER_NM_MSG_CYCLE:
		return TimerOutFlag_NM_MSG_CYCLE;
	}
	return -1;
}

//清除定时器超时标志
void ClcTimerOutFlag(NMTimerType_t TimerType)
{
	switch (TimerType)
	{
	case NM_TIMER_REPEAT_MSG:
		TimerOutFlag_REPEAT_MSG = 0;
		break;
	case NM_TIMER_NM_TIMEOUT:
		TimerOutFlag_NM_TIMEOUT = 0;
		break;
	case NM_TIMER_WAIT_BUS_SLEEP:
		TimerOutFlag_WAIT_BUS_SLEEP = 0;
		break;
	case NM_TIMER_IMMEDIATE_CYCLE_TIME:
		TimerOutFlag_IMMEDIATE_CYCLE_TIME = 0;
		break;
	case NM_TIMER_NM_MSG_CYCLE:
		TimerOutFlag_NM_MSG_CYCLE = 0;
		break;
	}
}


//FIFO相关的函数
/*说明：SetFIFO，将收到的报文放入FIFO,并调整FIFO
* 参数：GenericMessage* msg，报文指针
* 返回值：1:成功放入报文到FIFO，0：放入失败
*/
char SetToFIFO(NMPDU_t* msg)
{
	if (RecvFIFO.FullFlag == 1)//先判断缓冲区满否
		return 0;
	/*放入报文到缓冲区*/
	RecvFIFO.MSGs[RecvFIFO.Tail% FIFOMAX] = *msg;
	RecvFIFO.Tail = (RecvFIFO.Tail + 1) % FIFOMAX;
	/*清除空标识*/
	RecvFIFO.EmptyFlag = 0;
	if ((RecvFIFO.Tail + 1) == RecvFIFO.Head)//缓冲区满
		RecvFIFO.FullFlag = 1;
	return 1;
}
/*说明：GetFIFO，从FIFO取出报文,并调整FIFO
* 参数：GenericMessage* msg，报文指针
* 返回值：1:成功取出报文，0：取出失败
*/
char GetFromFIFO(NMPDU_t* msg)
{
	int i = 2;
	if (RecvFIFO.EmptyFlag == 1)//先判断缓冲区空否
		return 0;
	/*从缓冲区取出报文*/
	msg->MsgSNI = RecvFIFO.MSGs[RecvFIFO.Head% FIFOMAX].MsgSNI;
	msg->MsgCtl = RecvFIFO.MSGs[RecvFIFO.Head% FIFOMAX].MsgCtl;
	msg->MsgID = RecvFIFO.MSGs[RecvFIFO.Head% FIFOMAX].MsgID;
	//数据域直接复制
	for (; i < AUTOSARNM_DLC; i++)
	{
		msg->MsgData[i] = RecvFIFO.MSGs[RecvFIFO.Head% FIFOMAX].MsgData[i];
	}
	RecvFIFO.Head = (RecvFIFO.Head + 1) % FIFOMAX;
	/*清除满标识*/
	RecvFIFO.FullFlag = 0;
	if ((RecvFIFO.Tail) == RecvFIFO.Head)//缓冲区空
		RecvFIFO.EmptyFlag = 1;
	return 1;
}
/*说明：ClearFIFO，清空整个FIFO
* 参数：void
* 返回值：void
*/
void ClearFIFO(void)
{
	RecvFIFO.Total = 0;
	RecvFIFO.Head = 0;
	RecvFIFO.Tail = 0;
	RecvFIFO.FullFlag = 0;
	RecvFIFO.EmptyFlag = 1;
}

//10ms定时器
void Timer10()
{
	/*调用SetAlarm(xx)，定时器开始递增*/
	if (SetAlarm_REPEAT_MSG)
	{
		REPEAT_MSG_Count++;

		if (REPEAT_MSG_Count >= (T_REPEAT_MESSAGE / TIMER_PERIOD))// T_REPEAT_MESSAGE=1500ms
		{
			REPEAT_MSG_Count = 0;//重新计数
			TimerOutFlag_REPEAT_MSG = 1;
			SetAlarm_REPEAT_MSG = 0;//每次用完定时器都将其关闭,从而简化定时器管理
		}
	}
	else {
		REPEAT_MSG_Count = 0;
	}

	if (SetAlarm_NM_TIMEOUT)
	{
		NM_TIMEOUT_Count++;
		if (NM_TIMEOUT_Count >= (T_NM_TIMEOUT / TIMER_PERIOD))//T_NM_TIMEOUT=2000ms
		{
			NM_TIMEOUT_Count = 0;//重新计数
			TimerOutFlag_NM_TIMEOUT = 1;
			SetAlarm_NM_TIMEOUT = 0;//每次用完定时器都将其关闭
		}
	}
	else {
		NM_TIMEOUT_Count = 0;
	}

	if (SetAlarm_WAIT_BUS_SLEEP)
	{
		WAIT_BUS_SLEEP_Count++;
		if (WAIT_BUS_SLEEP_Count >= 100)//TError=1000ms
		{
			WAIT_BUS_SLEEP_Count = 0;//重新计数
			TimerOutFlag_WAIT_BUS_SLEEP = 1;
			SetAlarm_WAIT_BUS_SLEEP = 0;//每次用完定时器都将其关闭
		}
	}
	else {
		WAIT_BUS_SLEEP_Count = 0;
	}

	if (SetAlarm_IMMEDIATE_CYCLE_TIME)
	{
		IMMEDIATE_CYCLE_TIME_Count++;
		if (IMMEDIATE_CYCLE_TIME_Count >= (T_NM_ImmediateCycleTime / TIMER_PERIOD))//T_NM_ImmediateCycleTime=20ms
		{
			IMMEDIATE_CYCLE_TIME_Count = 0;//重新计数
			TimerOutFlag_IMMEDIATE_CYCLE_TIME = 1;
			SetAlarm_IMMEDIATE_CYCLE_TIME = 0;//每次用完定时器都将其关闭
		}
	}
	else {
		IMMEDIATE_CYCLE_TIME_Count = 0;
	}

	if (SetAlarm_NM_MSG_CYCLE)
	{
		NM_MSG_CYCLE_Count++;
		if (NM_MSG_CYCLE_Count >= (T_NM_MessageCycle / TIMER_PERIOD))//T_NM_MessageCycle=500ms
		{
			NM_MSG_CYCLE_Count = 0;//重新计数
			TimerOutFlag_NM_MSG_CYCLE = 1;
			SetAlarm_NM_MSG_CYCLE = 0;//每次用完定时器都将其关闭
		}
	}
	else {
		NM_MSG_CYCLE_Count = 0;
	}
}

/*
*自定义定时器函数：SetAlarm
参数：定时器类型
说明：每次调用都使定时器重新从0开始
*返回值：定时器ID，用定时器类型ID代替定时器ID
*/
int SetAlarm(NMTimerType_t timer)
{
	int Tid = 0;
	switch (timer)
	{
	case NM_TIMER_REPEAT_MSG:
		REPEAT_MSG_Count = 0;
		SetAlarm_REPEAT_MSG = 1;
		TimerOutFlag_REPEAT_MSG = 0;//每次设置定时器前先清除标志位
		Tid = NM_TIMER_REPEAT_MSG;
		break;
	case NM_TIMER_NM_TIMEOUT:
		NM_TIMEOUT_Count = 0;
		SetAlarm_NM_TIMEOUT = 1;
		TimerOutFlag_NM_TIMEOUT = 0;//每次设置定时器前先清除标志位
		Tid = NM_TIMER_NM_TIMEOUT;
		break;
	case NM_TIMER_WAIT_BUS_SLEEP:
		WAIT_BUS_SLEEP_Count = 0;
		SetAlarm_WAIT_BUS_SLEEP = 1;
		TimerOutFlag_WAIT_BUS_SLEEP = 0;//每次设置定时器前先清除标志位
		Tid = NM_TIMER_WAIT_BUS_SLEEP;
		break;
	case NM_TIMER_IMMEDIATE_CYCLE_TIME:
		IMMEDIATE_CYCLE_TIME_Count = 0;
		SetAlarm_IMMEDIATE_CYCLE_TIME = 1;
		TimerOutFlag_IMMEDIATE_CYCLE_TIME = 0;//每次设置定时器前先清除标志位
		Tid = NM_TIMER_IMMEDIATE_CYCLE_TIME;
		break;
	case NM_TIMER_NM_MSG_CYCLE:
		NM_MSG_CYCLE_Count = 0;
		SetAlarm_NM_MSG_CYCLE = 1;
		TimerOutFlag_NM_MSG_CYCLE = 0;//每次设置定时器前先清除标志位
		Tid = NM_TIMER_NM_MSG_CYCLE;
		break;
	}
	return Tid;
}
/*
*自定义定时器函数：CancelAlarm
参数：定时器类型
说明：定时器清0，不再计数
*/
void CancelAlarm(NMTimerType_t timer)
{
	switch (timer)
	{
	case NM_TIMER_REPEAT_MSG:
		SetAlarm_REPEAT_MSG = 0;
		REPEAT_MSG_Count = 0;
		TimerOutFlag_REPEAT_MSG = 0;//每次关闭定时器先清除标志位
		break;
	case NM_TIMER_NM_TIMEOUT:
		NM_TIMEOUT_Count = 0;
		SetAlarm_NM_TIMEOUT = 0;
		TimerOutFlag_NM_TIMEOUT = 0;//每次关闭定时器先清除标志位
		break;
	case NM_TIMER_WAIT_BUS_SLEEP:
		WAIT_BUS_SLEEP_Count = 0;
		SetAlarm_WAIT_BUS_SLEEP = 0;
		TimerOutFlag_WAIT_BUS_SLEEP = 0;//每次关闭定时器先清除标志位
		break;
	case NM_TIMER_IMMEDIATE_CYCLE_TIME:
		IMMEDIATE_CYCLE_TIME_Count = 0;
		SetAlarm_IMMEDIATE_CYCLE_TIME = 0;
		TimerOutFlag_IMMEDIATE_CYCLE_TIME = 0;//每次关闭定时器先清除标志位
		break;
	case NM_TIMER_NM_MSG_CYCLE:
		NM_MSG_CYCLE_Count = 0;
		SetAlarm_NM_MSG_CYCLE = 0;
		TimerOutFlag_NM_MSG_CYCLE = 0;//每次关闭定时器先清除标志位
		break;
	}
}
/*CAN中断收到的报文*/
void Recv_EveryMessage(NMPDU_t* p_Msg)
{
	/*将所有收到的NM报文放入缓冲区FIFO*/
	if (((p_Msg->MsgID) != NMID) && ((p_Msg->MsgID>>8) == (NMID>>8)))//过滤网络报文，不接收自己发出去的
	{
		RecvFIFO.SetMsg(p_Msg);//暂时不处理返回值
	}
}