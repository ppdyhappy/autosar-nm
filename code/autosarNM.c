#include "autosarNM.h"
#include "Driver_Common.h"
#include "autosarNMServer.h"

// 串口相关的部分这里没涉及
#define AUTOSARNM_DEBUG
#ifdef AUTOSARNM_DEBUG
#define AUTOSARNM_PRINT(...) printf(__VA_ARGS__)
#else
#define AUTOSARNM_PRINT(...)
#endif

NMStateType_t NMCurrentState = NM_OFF;
NMStateType_t NMPreState = NM_OFF;

ConfPara_t ConfigPara;

NMTypeU8_t NM_NodeCommReq = 0;
NMTypeU8_t NM_RepeatMsgReq = 0;

static void NMInit(void)
{
    AUTOSARNM_PRINT("NMInit\n");

    ConfigPara.t_repeat_message = T_REPEAT_MESSAGE;
    ConfigPara.t_nm_timeout = T_NM_TIMEOUT;
    ConfigPara.t_wait_bus_sleep = T_WAIT_BUS_SLEEP;
    ConfigPara.t_start_nm_tx = T_START_NM_TX;
    ConfigPara.t_start_app_tx = T_START_App_TX;
    ConfigPara.t_nm_immediate_cycle_time = T_NM_ImmediateCycleTime;
    ConfigPara.t_nm_message_cycle = T_NM_MessageCycle;
    ConfigPara.t_wake_up = T_WakeUp;
    ConfigPara.n_immediate_nm_times = N_ImmediateNM_TIMES;

    InitPlatform();

    // 失能应用报文发送和接收
    DisableAppMsgTxAndRx();

    NMCurrentState = NM_BUS_SLEEP;
    NMPreState = NM_INIT;
}

static void NMBusSleep(void)
{
    NMPDU_t NMMsgRecv;

    AUTOSARNM_PRINT("NMBusSleep\n");

    while(1)
    {
        // 收到其他节点发送的网络管理报文
        if(GetFromFIFO(&NMMsgRecv))
        {
            AUTOSARNM_PRINT("receive id:%lx sni:%x\n", NMMsgRecv.MsgID, NMMsgRecv.MsgSNI);

            NMCurrentState = NM_REPEAT_MSG;
            NMPreState = NM_BUS_SLEEP;

            CancelAlarm(NM_TIMER_NM_TIMEOUT);
            SetAlarm(NM_TIMER_NM_TIMEOUT);
            SetAlarm(NM_TIMER_NM_MSG_CYCLE);

            break;
        }

        // 如果当前节点需要与其他节点通信，则进入重复发送报文状态
        if(NM_NodeCommReq)
        {
            AUTOSARNM_PRINT("NM_NodeCommReq = 1\n");

            NMCurrentState = NM_REPEAT_MSG;
            NMPreState = NM_BUS_SLEEP;

            CancelAlarm(NM_TIMER_NM_TIMEOUT);
            SetAlarm(NM_TIMER_NM_TIMEOUT);

            break;
        }
    } // end while
}

static void NMRepeatMsg(void)
{
    NMTypeU8_t N_ImmediateNM_TIMES_Cnt = 0;
    NMPDU_t NMMsgTx;
    NMPDU_t NMMsgRecv;
    NMTypeU8_t NMTxFlag = 0;

    AUTOSARNM_PRINT("NMRepeatMsg\n");

    // 使能应用报文的发送和接受
    EnableAppMsgTxAndRx();

    InitNMPDU(&NMMsgTx);

    // 如果是因为节点需要通信进来的，则开启 NM_TIMER_IMMEDIATE_CYCLE_TIME 定时器
    if(NM_NodeCommReq)
    {
        NMMsgTx.MsgCtl |= NM_CTRL_ACTIVE_WAKEUP;

        SetAlarm(NM_TIMER_IMMEDIATE_CYCLE_TIME);
    }

    // 如果通过 repeatMessageRequest 函数调用而进入重复报文状态的，则开启 NM_TIMER_IMMEDIATE_CYCLE_TIME 定时器
    if(NM_RepeatMsgReq)
    {
        NM_RepeatMsgReq = 0;

        NMMsgTx.MsgCtl |= NM_CTRL_REPEAT_MSG;

        SetAlarm(NM_TIMER_IMMEDIATE_CYCLE_TIME);
    }

    // 先发送一帧
    NMTxFlag = TX_CAN_Transmit(&NMMsgTx);

    // 如果是从总线睡眠模式或准备总线睡眠模式进入到重复报文状态，则在发送第一帧网络管理报文后在 T_START_App_TX 时间内将第一帧 CAN应用报文发送出来
    if((NMPreState == NM_BUS_SLEEP) || (NMPreState == NM_PRE_BUS_SLEEP))
    {
        StartAutosarAppMsgSend();
    }

    while(1)
    {
        if(GetTimerIsOut(NM_TIMER_IMMEDIATE_CYCLE_TIME))
        {
            CancelAlarm(NM_TIMER_IMMEDIATE_CYCLE_TIME);

            N_ImmediateNM_TIMES_Cnt += 1;

            // send nm msg
            NMTxFlag = TX_CAN_Transmit(&NMMsgTx);

            if(N_ImmediateNM_TIMES_Cnt <= (N_ImmediateNM_TIMES - 1))
                SetAlarm(NM_TIMER_IMMEDIATE_CYCLE_TIME);
            else
            {
                SetAlarm(NM_TIMER_NM_MSG_CYCLE);
            }
        }

        // 成功发送CAN报文，重启 NM_TIMER_NM_TIMEOUT
        if(NMTxFlag)
        {
            NMTxFlag = 0;

            CancelAlarm(NM_TIMER_NM_TIMEOUT);
            SetAlarm(NM_TIMER_NM_TIMEOUT);
        }

        // 收到其他节点发送的网络管理报文，重启 NM_TIMER_NM_TIMEOUT
        if(GetFromFIFO(&NMMsgRecv))
        {
            AUTOSARNM_PRINT("receive id:%lx sni:%x\n", NMMsgRecv.MsgID, NMMsgRecv.MsgSNI);

            CancelAlarm(NM_TIMER_NM_TIMEOUT);
            SetAlarm(NM_TIMER_NM_TIMEOUT);
        }

        if(GetTimerIsOut(NM_TIMER_NM_MSG_CYCLE))
        {
            CancelAlarm(NM_TIMER_NM_MSG_CYCLE);
            SetAlarm(NM_TIMER_NM_MSG_CYCLE);

            // send nm msg
            NMTxFlag = TX_CAN_Transmit(&NMMsgTx);
        }

        if(GetTimerIsOut(NM_TIMER_NM_TIMEOUT))
        {
            // 重启 NM_TIMEOUT
            CancelAlarm(NM_TIMER_NM_TIMEOUT);
            SetAlarm(NM_TIMER_NM_TIMEOUT);
        }

        if(GetTimerIsOut(NM_TIMER_REPEAT_MSG))
        {
            // NM_NodeCommReq = 1，表示当前节点需要与网络上其他节点通信，准备进入 NM_NORMAL_OP 状态
            if(NM_NodeCommReq)
            {
                // REPEAT_MSG 超时要进入正常操作模式
                CancelAlarm(NM_TIMER_REPEAT_MSG);

                // 进入到 NM_NORMAL_OP 状态时不关闭 NM_TIMER_NM_MSG_CYCLE 和 NM_TIMER_NM_TIMEOUT 定时器

                NMCurrentState = NM_NORMAL_OP;
                NMPreState = NM_REPEAT_MSG;

                break;
            }
            // NM_NodeCommReq = 0，表示当前节点不再需要与网络上其他节点通信，准备进入 NM_READY_SLEEP 状态
            else
            {
                CancelAlarm(NM_TIMER_REPEAT_MSG);

                NMCurrentState = NM_READY_SLEEP;
                NMPreState = NM_REPEAT_MSG;

                break;
            }

        }
    } // end while
}

static void NMNormalOperation(void)
{
    NMPDU_t NMMsgTx;
    NMPDU_t NMMsgRecv;
    NMTypeU8_t NMTxFlag = 0;

    AUTOSARNM_PRINT("NMNormalOperation\n");

    InitNMPDU(&NMMsgTx);

    if(NM_NodeCommReq)
    {
        NMMsgTx.MsgCtl |= NM_CTRL_ACTIVE_WAKEUP;
    }

    while(1)
    {
        if(GetTimerIsOut(NM_TIMER_NM_MSG_CYCLE))
        {
            CancelAlarm(NM_TIMER_NM_MSG_CYCLE);
            SetAlarm(NM_TIMER_NM_MSG_CYCLE);

            // send nm msg
            NMTxFlag = TX_CAN_Transmit(&NMMsgTx);
        }

        if(GetTimerIsOut(NM_TIMER_NM_TIMEOUT))
        {
            // 重启 NM_TIMEOUT
            CancelAlarm(NM_TIMER_NM_TIMEOUT);
            SetAlarm(NM_TIMER_NM_TIMEOUT);
        }

        // 成功发送CAN报文，重启 NM_TIMER_NM_TIMEOUT
        if(NMTxFlag)
        {
            NMTxFlag = 0;

            CancelAlarm(NM_TIMER_NM_TIMEOUT);

            SetAlarm(NM_TIMER_NM_TIMEOUT);
        }

        // 收到其他节点发送的网络管理报文，重启 NM_TIMER_NM_TIMEOUT
        if(GetFromFIFO(&NMMsgRecv))
        {
            AUTOSARNM_PRINT("receive id:%lx sni:%x\n", NMMsgRecv.MsgID, NMMsgRecv.MsgSNI);

            CancelAlarm(NM_TIMER_NM_TIMEOUT);
            SetAlarm(NM_TIMER_NM_TIMEOUT);

            // 如果收到 repeat message request bit 为1 的网络管理报文， 则进入重复报文状态
            if(NMMsgRecv.MsgCtl & NM_CTRL_REPEAT_MSG)
            {
                CancelAlarm(NM_TIMER_NM_MSG_CYCLE);
                SetAlarm(NM_TIMER_NM_MSG_CYCLE);

                CancelAlarm(NM_TIMER_NM_TIMEOUT);
                SetAlarm(NM_TIMER_NM_TIMEOUT);

                NMCurrentState = NM_REPEAT_MSG;
                NMPreState = NM_NORMAL_OP;

                break;
            }
        }

        // 如果 repeatMessageRequest 函数被调用需进入重复报文状态
        if(NM_RepeatMsgReq)
        {
            CancelAlarm(NM_TIMER_NM_MSG_CYCLE);
            SetAlarm(NM_TIMER_NM_MSG_CYCLE);

            CancelAlarm(NM_TIMER_NM_TIMEOUT);
            SetAlarm(NM_TIMER_NM_TIMEOUT);

            NMCurrentState = NM_REPEAT_MSG;
            NMPreState = NM_NORMAL_OP;

            break;
        }

        // 如果本地睡眠条件满足，即不需要与其他节点通信，则进入准备睡眠状态
        if(!NM_NodeCommReq)
        {
            CancelAlarm(NM_TIMER_NM_MSG_CYCLE);

            CancelAlarm(NM_TIMER_NM_TIMEOUT);
            SetAlarm(NM_TIMER_NM_TIMEOUT);

            NMCurrentState = NM_READY_SLEEP;
            NMPreState = NM_NORMAL_OP;

            break;
        }
    } // end while
}

static void NMReadySleepState(void)
{
    NMPDU_t NMMsgRecv;

    AUTOSARNM_PRINT("NMReadySleepState\n");

    while(1)
    {
        // 如果 NM_TIMER_NM_TIMEOUT 超时，则进入准备总线睡眠状态
        if(GetTimerIsOut(NM_TIMER_NM_TIMEOUT))
        {
            CancelAlarm(NM_TIMER_NM_TIMEOUT);

            CancelAlarm(NM_TIMER_WAIT_BUS_SLEEP);
            SetAlarm(NM_TIMER_WAIT_BUS_SLEEP);

            NMCurrentState = NM_PRE_BUS_SLEEP;
            NMPreState = NM_READY_SLEEP;

            break;
        }

        // 收到其他节点发送的网络管理报文，重启 NM_TIMER_NM_TIMEOUT
        if(GetFromFIFO(&NMMsgRecv))
        {
            AUTOSARNM_PRINT("receive id:%lx sni:%x\n", NMMsgRecv.MsgID, NMMsgRecv.MsgSNI);

            CancelAlarm(NM_TIMER_NM_TIMEOUT);
            SetAlarm(NM_TIMER_NM_TIMEOUT);

            // 如果收到 repeat message request bit 为1 的网络管理报文， 则进入重复报文状态
            if(NMMsgRecv.MsgCtl & NM_CTRL_REPEAT_MSG)
            {
                CancelAlarm(NM_TIMER_NM_MSG_CYCLE);
                SetAlarm(NM_TIMER_NM_MSG_CYCLE);

                NMCurrentState = NM_REPEAT_MSG;
                NMPreState = NM_READY_SLEEP;

                break;
            }
        }

        // 如果 repeatMessageRequest 函数被调用需进入重复报文状态
        if(NM_RepeatMsgReq)
        {
            CancelAlarm(NM_TIMER_NM_MSG_CYCLE);
            SetAlarm(NM_TIMER_NM_MSG_CYCLE);

            CancelAlarm(NM_TIMER_NM_TIMEOUT);
            SetAlarm(NM_TIMER_NM_TIMEOUT);

            NMCurrentState = NM_REPEAT_MSG;
            NMPreState = NM_READY_SLEEP;

            break;
        }

        // 如果本地睡眠条件不再满足，则进入正常操作状态
        if(NM_NodeCommReq)
        {
            CancelAlarm(NM_TIMER_NM_TIMEOUT);
            SetAlarm(NM_TIMER_NM_TIMEOUT);

            NMCurrentState = NM_NORMAL_OP;
            NMPreState = NM_READY_SLEEP;

            break;
        }
    } // end while
}

static void NMPreBusSleep(void)
{
    NMPDU_t NMMsgRecv;

    AUTOSARNM_PRINT("NMPreBusSleep\n");

    // 失能应用报文发送和接收
    DisableAppMsgTxAndRx();

    while(1)
    {
        // 如果 NM_TIMER_WAIT_BUS_SLEEP 超时，则进入总线睡眠模式
        if(GetTimerIsOut(NM_TIMER_WAIT_BUS_SLEEP))
        {
            CancelAlarm(NM_TIMER_WAIT_BUS_SLEEP);

            NMCurrentState = NM_BUS_SLEEP;
            NMPreState = NM_PRE_BUS_SLEEP;

            break;
        }

        // 收到其他节点发送的网络管理报文，进入重复报文状态
        if(GetFromFIFO(&NMMsgRecv))
        {
            AUTOSARNM_PRINT("receive id:%lx sni:%x\n", NMMsgRecv.MsgID, NMMsgRecv.MsgSNI);

            CancelAlarm(NM_TIMER_NM_MSG_CYCLE);
            SetAlarm(NM_TIMER_NM_MSG_CYCLE);

            CancelAlarm(NM_TIMER_NM_TIMEOUT);
            SetAlarm(NM_TIMER_NM_TIMEOUT);

            NMCurrentState = NM_REPEAT_MSG;
            NMPreState = NM_PRE_BUS_SLEEP;

            break;
        }

        // 如果本地节点有通信需求，则进入重复报文状态
        if(NM_NodeCommReq)
        {
            CancelAlarm(NM_TIMER_NM_TIMEOUT);
            SetAlarm(NM_TIMER_NM_TIMEOUT);

            NMCurrentState = NM_REPEAT_MSG;
            NMPreState = NM_PRE_BUS_SLEEP;

            break;
        }
    } // end while
}

/*
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
*/
void NMStateManage(void)
{
    while(1)
    {
        switch(NMCurrentState)
        {
            case NM_INIT:
            NMInit();
            break;

            case NM_BUS_SLEEP:
            NMBusSleep();
            break;

            case NM_REPEAT_MSG:
            NMRepeatMsg();
            break;

            case NM_NORMAL_OP:
            NMNormalOperation();
            break;

            case NM_READY_SLEEP:
            NMReadySleepState();
            break;

            case NM_PRE_BUS_SLEEP:
            NMPreBusSleep();
            break;
        } // end switch
    } // end while

}