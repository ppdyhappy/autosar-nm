#ifndef _AUTOSAR_NM_SERVER_H_
#define _AUTOSAR_NM_SERVER_H_

#include "autosarNM.h"

#define E_OK        0
#define E_ERROR     -1

typedef unsigned char StatusType_t;

StatusType_t StartAutosarNM(void);
StatusType_t StopAutosarNM(void);

StatusType_t CanNm_PassiveStartup(void);
StatusType_t CanNm_NetworkRequest(void);

StatusType_t CanNm_NetworkReleases(void);
StatusType_t RepeatMessageRequest(void);

StatusType_t StartAutosarAppMsgSend(void);
StatusType_t EnableAppMsgTxAndRx(void);
StatusType_t DisableAppMsgTxAndRx(void);

#endif
