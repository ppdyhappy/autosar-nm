#include "autosarNMServer.h"

extern NMStateType_t NMCurrentState;
extern NMStateType_t NMPreState;

extern NMTypeU8_t NM_NodeCommReq;
extern NMTypeU8_t NM_RepeatMsgReq;

StatusType_t StartAutosarNM(void)
{
    NMCurrentState = NM_INIT;
    NMPreState = NM_OFF;

    NMStateManage();

    return E_OK;
}

StatusType_t StopAutosarNM(void)
{
    NMCurrentState = NM_SHUTDOWN;
    NMPreState = NM_INIT;

    return E_OK;
}

StatusType_t CanNm_NetworkRequest(void)
{
    NM_NodeCommReq = 1;
}

StatusType_t CanNm_NetworkReleases(void)
{
    NM_NodeCommReq = 0;
}

StatusType_t RepeatMessageRequest(void)
{
    NM_RepeatMsgReq = 1;
}

StatusType_t StartAutosarAppMsgSend(void)
{

}

StatusType_t EnableAppMsgTxAndRx(void)
{
    // 增加使能应用报文发送和接收前的处理
}

StatusType_t DisableAppMsgTxAndRx(void)
{
    // 增加失能应用报文发送和接收前的处理
}
