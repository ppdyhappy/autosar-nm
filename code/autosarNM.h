#ifndef _AUTOSAR_NM_H_
#define _AUTOSAR_NM_H_

typedef unsigned char NMStateType_t;
typedef unsigned char NMTypeU8_t;
typedef unsigned int NMTypeU16_t;
typedef unsigned long NMTypeU32_t;

typedef unsigned int  NMTimerType_t;

typedef unsigned long NMMsgID_t;
typedef unsigned char NMMsgSNI_t;
typedef unsigned char NMMsgCtl_t;
typedef unsigned char NMMsgData_t;

enum
{
    NM_SHUTDOWN = 255,
    NM_OFF = 0,
    NM_INIT,
    NM_BUS_SLEEP,
    NM_PRE_BUS_SLEEP,
    NM_REPEAT_MSG,
    NM_NORMAL_OP,
    NM_READY_SLEEP,
};

#define AUTOSARNM_DLC   (8)

#define NM_CTRL_REPEAT_MSG (1 << 0)
#define NM_CTRL_ACTIVE_WAKEUP (1 << 1)

/*
#define T_REPEAT_MESSAGE    (1500)
#define T_NM_TIMEOUT (2000)
#define T_WAIT_BUS_SLEEP (2000)
#define T_START_NM_TX   (50)
#define T_START_App_TX (20)
#define T_NM_ImmediateCycleTime (20)
#define T_NM_MessageCycle (500)
#define T_WakeUp (100)
#define N_ImmediateNM_TIMES (5)
*/

typedef struct 
{
    NMTypeU16_t t_repeat_message;
    NMTypeU16_t t_nm_timeout;
    NMTypeU16_t t_wait_bus_sleep;
    NMTypeU16_t t_start_nm_tx;
    NMTypeU16_t t_start_app_tx;
    NMTypeU16_t t_nm_immediate_cycle_time;
    NMTypeU16_t t_nm_message_cycle;
    NMTypeU16_t t_wake_up;
    NMTypeU8_t n_immediate_nm_times;
}ConfPara_t;


typedef struct 
{
	NMMsgID_t MsgID; //报文ID
	NMMsgSNI_t MsgSNI; //源节点地址
	NMMsgCtl_t MsgCtl;//控制字节
	NMMsgData_t MsgData[6];//数据域
}NMPDU_t;


void NMStateManage(void);

#endif