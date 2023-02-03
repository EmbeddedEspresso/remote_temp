/**
  ******************************************************************************
  * @file           : ModemHdlr.c
  * @brief          : Modem Handler
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 EmbeddedEspresso.
  * All rights reserved.
  *
  * This software component is licensed by EmbeddedEspresso under BSD 3-Clause 
  * license. You may not use this file except in compliance with the License. 
  * You may obtain a copy of the License at: 
  * opensource.org/licenses/BSD-3-Clause
  ******************************************************************************
  */

#include "main.h"

typedef enum
{
    MODEM_HDLR_RST = 0,
    MODEM_HDLR_WAITRST,
    MODEM_HDLR_INIT,
    MODEM_HDLR_DETECT,
    MODEM_HDLR_DETECT_ECHO,
    MODEM_HDLR_DETECT_WAIT,
    MODEM_HDLR_UNSOLICITED,
    MODEM_HDLR_PINSTS,
    MODEM_HDLR_PINSTS_WAIT,
    MODEM_HDLR_CHECKPIN,
    MODEM_HDLR_CHECKPIN_WAIT,
    MODEM_HDLR_PINREADY_WAIT,
    MODEM_HDLR_GET_SIGNAL_LEVEL,
    MODEM_HDLR_GET_DATE_TIME,
    MODEM_HDLR_READ_SMS,
    MODEM_HDLR_SEND_SMS,
    MODEM_HDLR_SEND_SMS_MSGTEXT,
    MODEM_HDLR_DELETE_SMS,
    MODEM_HDLR_PREIDLE,
    MODEM_HDLR_IDLE,
    MODEM_HDLR_PREERR_STS,
    MODEM_HDLR_ERR_STS

}ModemHdlrFsmSts;

typedef enum
{
    MODEM_CMD_IDLE,
    MODEM_CMD_SEND,
    MODEM_CMD_SENDECHO,
    MODEM_CMD_GETANS,
}ModemCmdFsmSts;

typedef enum
{
    MODEM_CMD_AT = 0,
    MODEM_CMD_ATCPIN,
    MODEM_CMD_ATCSQ,
    MODEM_CMD_ATCCLK,
    MODEM_CMD_ATCMGS_NUM,
    MODEM_CMD_ATCMGS_MSG,
    MODEM_CMD_ATCMGR,
    MODEM_CMD_ATCMGD
}ModemHdlrCmdList;

typedef enum
{
    MODEM_REQ_START,
    MODEM_REQ_WAIT,
}ModemRequestFsmSts;

// Data type for special handling
typedef enum
{
    MODEM_SHDL_NONE = 0u,
    MODEM_SHDL_CTRLZ = 1u,
    MODEM_SHDL_SKIPECHO = 2u,
    MODEM_SHDL_SMS_MULTIANS = 4u
};

typedef struct
{
    uint8_t cmdIdx;
    char cmd[32];
    char resp[32];
    uint16_t tmout;
    uint8_t retry;
    uint8_t spcHndlr;
    char finalResp[16];
}ModemCmd;

uint32_t ModemSts = 0u;
uint8_t SmsRecIdx = 0u;

ModemCmd modemCmdList[] =
{    /* Cmd type,           Request             Response    Timeout     Retry   Special flags    */
    {MODEM_CMD_AT,          "AT\r",             "OK",       1500,       10,     MODEM_SHDL_NONE,                        NULL},
    {MODEM_CMD_ATCPIN,      "AT+CPIN?\r",       "OK",       1500,       3,      MODEM_SHDL_NONE,                        NULL},
    {MODEM_CMD_ATCSQ,       "AT+CSQ\r",         "+CSQ:",    1500,       3,      MODEM_SHDL_NONE,                        NULL},
    {MODEM_CMD_ATCCLK,      "AT+CCLK?\r",       "+CCLK:",   1500,       3,      MODEM_SHDL_NONE,                        NULL},
    {MODEM_CMD_ATCMGS_NUM,  "AT+CMGS=\"%s\"\r", ">",        1500,       3,      MODEM_SHDL_NONE,                        NULL},
    {MODEM_CMD_ATCMGS_MSG,  "%s\r",             "OK",       15000,      3,      MODEM_SHDL_CTRLZ | MODEM_SHDL_SKIPECHO, NULL},
    {MODEM_CMD_ATCMGR,      "AT+CMGR=%d\r",     "+CMGR:",   1500,       3,      MODEM_SHDL_SMS_MULTIANS,                "OK"},
    {MODEM_CMD_ATCMGD,      "AT+CMGD=%d,%d\r",  "OK",       1500,       3,      MODEM_SHDL_NONE,                        NULL}
};

static ModemHdlrFsmSts fsmsts = MODEM_HDLR_INIT;
static ModemCmdFsmSts fsmCmdSts = MODEM_CMD_IDLE;
char cmdAns[32];
char cmdMdmStart[] = "AT\r";
char cmdMdmPinSts[] ="AT+CPIN?\r";
char cmdDeleteSms[] = "AT+CMGD=%d,%d\r";
static uint8_t gcAns[256];
static uint32_t gcAnsLen;
char *gMsg;
char *gNum;
char gLocalCmd[64];
uint8_t par1, par2;

ModemHdlrErrCode ModemHdlrExecCmd(ModemCmd *cmd, char *cAns, uint32_t *cAnsLen, ...);

ModemHdlrErrCode ModemHdlrInit (void)
{
    fsmsts = MODEM_HDLR_RST;
    fsmCmdSts = MODEM_CMD_IDLE;
}

ModemHdlrErrCode ModemHdlrRun (void)
{
    ModemHdlrErrCode result = MODEM_HDLR_OK;
    static int tmr;
    uint32_t ansIdx, ansIdx2, idx;
    char *localAnsPnt;
    char localCmd[64];
    char localPar[16];
    uint32_t localAnsLen;
    uint8_t rssi, ber;
    ModemCmd *currCmd;
    ModemHdlrErrCode currErrCode;
    static retryCnt = 0;

    switch (fsmsts)
    {
        case MODEM_HDLR_RST:
            HAL_GPIO_WritePin(GPIOA, MDMRST_Pin, GPIO_PIN_RESET);
            TimerSet(&tmr, 500);
            fsmsts = MODEM_HDLR_WAITRST;
            break;

        case MODEM_HDLR_WAITRST:
            if (isTimerExpired(tmr))
            {
                HAL_GPIO_WritePin(GPIOA, MDMRST_Pin, GPIO_PIN_SET);
                fsmsts = MODEM_HDLR_INIT;
                PRINT_DEBUG("[Modem]: Modem has been reset\r\n");
            }
            break;

        case MODEM_HDLR_INIT:
            ModemHdlrClrSMSReady();
            ModemHdlrClrCallReady();
            fsmsts = MODEM_HDLR_DETECT;
            break;

        case MODEM_HDLR_DETECT:
            currCmd = &modemCmdList[MODEM_CMD_AT];

            if (fsmCmdSts == MODEM_CMD_IDLE )
            {
                fsmCmdSts = MODEM_CMD_SEND;
            }

            currErrCode = ModemHdlrExecCmd(&modemCmdList[MODEM_CMD_AT], gcAns, &gcAnsLen);

            if (currErrCode == MODEM_HDLR_OK)
            {
                fsmsts = MODEM_HDLR_PREIDLE;
            }
            else if (currErrCode == MODEM_HDLR_ERR)
            {
                fsmsts = MODEM_HDLR_PREERR_STS;
            }
            break;

        case MODEM_HDLR_UNSOLICITED:
            if(UartModemHdlrGetAnsNum() != 0)
            {
                if (UartModemHdlrSearchAns("+CPIN: READY\r", 13, &ansIdx) == COM_MODEM_HDLR_OK)
                {
                    PRINT_DEBUG("[Modem]: Unsolicited detected -> +CPIN: Ready\r\n");
                    UartModemHdlrRemoveAns(ansIdx);
                    fsmsts = MODEM_HDLR_PINSTS;
                }
                else if (UartModemHdlrSearchAns("+CPIN: NOT INSERTED\r", 20, &ansIdx) == COM_MODEM_HDLR_OK)
                {
                    PRINT_DEBUG("[Modem]: Unsolicited detected -> +CPIN: Not inserted\r\n");
                    UartModemHdlrRemoveAns(ansIdx);
                    fsmsts = MODEM_HDLR_IDLE;
                }
                else if (UartModemHdlrSearchAns("Call Ready\r", 11, &ansIdx) == COM_MODEM_HDLR_OK)
                {
                    PRINT_DEBUG("[Modem]: Unsolicited detected -> Call Ready\r\n");
                    UartModemHdlrRemoveAns(ansIdx);
                    ModemHdlrSetCallReady();
                    fsmsts = MODEM_HDLR_IDLE;
                }
                else if (UartModemHdlrSearchAns("SMS Ready\r", 10, &ansIdx) == COM_MODEM_HDLR_OK)
                {
                    PRINT_DEBUG("[Modem]: Unsolicited detected -> SMSReady\r\n");
                    UartModemHdlrRemoveAns(ansIdx);
                    ModemHdlrSetSMSReady();
                    fsmsts = MODEM_HDLR_IDLE;
                }
                else if (UartModemHdlrSearchAns("+CMTI:", 6, &ansIdx) == COM_MODEM_HDLR_OK)
                {
                    PRINT_DEBUG("[Modem]: Unsolicited detected -> +CMTI\r\n");
                    UartModemHdlrGetAnsByIdx(ansIdx, &localAnsPnt, &localAnsLen);
                    strncpy(localCmd, localAnsPnt, localAnsLen);
                    localCmd[localAnsLen] = 0;
                    (void)StringExtract(localCmd, ",", 1, localPar);
                    SmsRecIdx = atoi(localPar);
                    ModemHdlrSetSMSReceived();
                    UartModemHdlrRemoveAns(ansIdx);
                    fsmsts = MODEM_HDLR_IDLE;
                }
                else
                {
                    idx = UartModemHdlrGetAnsNum();
                    /* Clear eventual pending not-used/defined unsolicited */
                    while(idx)
                    {
                        UartModemHdlrRemoveAns(0);
                        idx--;
                    }
                    fsmsts = MODEM_HDLR_IDLE;
                }
            }

            break;

        case MODEM_HDLR_PINSTS:
            if (UartModemHdlrTx(cmdMdmPinSts, strlen(cmdMdmPinSts)) == COM_MODEM_HDLR_OK)
            {
                TimerSet(&tmr, 1500);
                fsmsts = MODEM_HDLR_PINSTS_WAIT;
            }
            break;

        case MODEM_HDLR_PINSTS_WAIT:
            if (!isTimerExpired(tmr))
            {
                if (UartModemHdlrSearchAns("+CPIN: SIM PIN\r", 15, &ansIdx) == COM_MODEM_HDLR_OK)
                {
                    PRINT_DEBUG("[Modem]: PIN must be provided\r\n");
                    fsmsts = MODEM_HDLR_CHECKPIN;
                }
                else if (UartModemHdlrSearchAns("+CPIN: READY\r", 13, &ansIdx) == COM_MODEM_HDLR_OK)
                {
                    PRINT_DEBUG("[Modem]: No PIN must be provided\r\n");
                    UartModemHdlrClearRxBuffer();
                    fsmsts = MODEM_HDLR_IDLE;
                }
            }
            else
            {
                result = MODEM_HDLR_PINSTS;
            }
            break;

//        case MODEM_HDLR_CHECKPIN:
//            if (UartModemHdlrTx(cmdMdmPin, strlen(cmdMdmPin)) == COM_MODEM_HDLR_OK)
//            {
//                PRINT_DEBUG("[Modem]: Configuring PIN code\r\n");
//                TimerSet(&tmr, 1500);
//                fsmsts = MODEM_HDLR_CHECKPIN_WAIT;
//            }
//            break;
//
//        case MODEM_HDLR_CHECKPIN_WAIT:
//            if (!isTimerExpired(tmr))
//            {
//                if (UartModemHdlrRx(cmdAns, strlen(cmdMdmPin) + 6) == COM_MODEM_HDLR_OK)
//                {
//                    if (!strncmp(cmdAns[strlen(cmdMdmPin) + 2], "OK\r", 2))
//                    {
//                        PRINT_DEBUG("[Modem]: PIN configured correctly\r\n");
//                        fsmsts = MODEM_HDLR_IDLE;
//                    }
//                    else if (!strncmp(cmdAns[strlen(cmdMdmPin) + 2], "ERR\r", 3))
//                    {
//                        UartModemHdlrFlushRx();
//                        PRINT_DEBUG("[Modem]: PIN configuration error\r\n");
//                        fsmsts = MODEM_HDLR_CHECKPIN;
//                    }
//                }
//            }
//            else
//            {
//                fsmsts = MODEM_HDLR_CHECKPIN;
//            }
//            break;

        case MODEM_HDLR_GET_SIGNAL_LEVEL:
            currErrCode = ModemHdlrExecCmd(&modemCmdList[MODEM_CMD_ATCSQ], gcAns, &gcAnsLen);

            if (currErrCode == MODEM_HDLR_OK)
            {
                fsmsts = MODEM_HDLR_IDLE;
            }
            else if (currErrCode == MODEM_HDLR_ERR)
            {
                fsmsts = MODEM_HDLR_PREERR_STS;
            }
            break;

        case MODEM_HDLR_GET_DATE_TIME:
            currErrCode = ModemHdlrExecCmd(&modemCmdList[MODEM_CMD_ATCCLK], gcAns, &gcAnsLen);

            if (currErrCode == MODEM_HDLR_OK)
            {
                fsmsts = MODEM_HDLR_IDLE;
            }
            else if (currErrCode == MODEM_HDLR_ERR)
            {
                fsmsts = MODEM_HDLR_PREERR_STS;
            }
            break;

        case MODEM_HDLR_READ_SMS:
            currErrCode = ModemHdlrExecCmd(&modemCmdList[MODEM_CMD_ATCMGR], gcAns, &gcAnsLen, par1);

            if (currErrCode == MODEM_HDLR_OK)
            {
                fsmsts = MODEM_HDLR_IDLE;
            }
            else if (currErrCode == MODEM_HDLR_ERR)
            {
                fsmsts = MODEM_HDLR_PREERR_STS;
            }
            break;

        case MODEM_HDLR_SEND_SMS:
            currErrCode = ModemHdlrExecCmd(&modemCmdList[MODEM_CMD_ATCMGS_NUM], gcAns, &gcAnsLen, gNum);

            if (currErrCode == MODEM_HDLR_OK)
            {
                fsmsts = MODEM_HDLR_SEND_SMS_MSGTEXT;
                fsmCmdSts = MODEM_CMD_SEND;
            }
            else if (currErrCode == MODEM_HDLR_ERR)
            {
                fsmsts = MODEM_HDLR_PREERR_STS;
            }
            break;

        case MODEM_HDLR_SEND_SMS_MSGTEXT:
            /* SMS Sending is splitted in two steps. The second part contains only the message body */
            currErrCode = ModemHdlrExecCmd(&modemCmdList[MODEM_CMD_ATCMGS_MSG], gcAns, &gcAnsLen, gMsg);

            if (currErrCode == MODEM_HDLR_OK)
            {
                fsmsts = MODEM_HDLR_IDLE;
            }
            else if (currErrCode == MODEM_HDLR_ERR)
            {
                fsmsts = MODEM_HDLR_PREERR_STS;
            }
            break;

        case MODEM_HDLR_DELETE_SMS:
            currErrCode = ModemHdlrExecCmd(&modemCmdList[MODEM_CMD_ATCMGD], gcAns, &gcAnsLen, par1, par2);

            if (currErrCode == MODEM_HDLR_OK)
            {
                fsmsts = MODEM_HDLR_IDLE;
            }
            else if (currErrCode == MODEM_HDLR_ERR)
            {
                fsmsts = MODEM_HDLR_PREERR_STS;
            }
            break;

        case MODEM_HDLR_PREIDLE:
            PRINT_DEBUG("[Modem]: Modem initialized\r\n");
            TimerSet(&tmr, 2000);
            fsmsts = MODEM_HDLR_IDLE;
            break;

        case MODEM_HDLR_IDLE:
            if (isTimerExpired(tmr))
            {
                TimerSet(&tmr, 2000);
                if (UartModemHdlrGetAnsNum() != 0)
                {
                    fsmsts = MODEM_HDLR_UNSOLICITED;
                }
            }
            break;

        case MODEM_HDLR_PREERR_STS:
            PRINT_DEBUG("[Modem]: Modem initialization error\r\n");
            fsmsts = MODEM_HDLR_ERR_STS;
            break;

        case MODEM_HDLR_ERR_STS:
            break;

    }

    return result;
}

ModemHdlrErrCode ModemHdlrExecCmd(ModemCmd *cmd, char *cAns, uint32_t *cAnsLen, ...)
{
    ModemHdlrErrCode result = MODEM_HDLR_BUSY;
    static int tmr;
    uint32_t ansIdx, andIdx2;
    char *localAnsPnt;
    uint32_t localAnsLen, idx;
    uint32_t gLocalCmdLen;
    static uint8_t retryCnt;
    va_list argptr;

    switch (fsmCmdSts)
    {
        case MODEM_CMD_SEND:
            va_start(argptr,cAnsLen);
            vsprintf(gLocalCmd, cmd->cmd, argptr);
            va_end(argptr);
            if (cmd->spcHndlr & MODEM_SHDL_CTRLZ)
            {
                strcat(gLocalCmd, "\x1a");
            }
            if (UartModemHdlrTx(gLocalCmd, strlen(gLocalCmd)) == COM_MODEM_HDLR_OK)
            {
                if (retryCnt == 0)
                {
                    retryCnt = cmd->retry;
                }
                TimerSet(&tmr, cmd->tmout);
                if (cmd->spcHndlr & MODEM_SHDL_SKIPECHO)
                {
                    fsmCmdSts = MODEM_CMD_GETANS;
                }
                else
                {
                    fsmCmdSts = MODEM_CMD_SENDECHO;
                }
            }
            break;

        case MODEM_CMD_SENDECHO:
            va_start(argptr,cAnsLen);
            vsprintf(gLocalCmd, cmd->cmd, argptr);
            va_end(argptr);
            if (!isTimerExpired(tmr))
            {
                if (UartModemHdlrSearchAns(gLocalCmd, strlen(gLocalCmd), &ansIdx) == COM_MODEM_HDLR_OK)
                {
                    UartModemHdlrRemoveAns(ansIdx);
                    fsmCmdSts = MODEM_CMD_GETANS;
                }
            }
            else
            {
                retryCnt--;

                if (retryCnt == 0u)
                {
                    result = MODEM_HDLR_ERR;
                    fsmCmdSts = MODEM_CMD_IDLE;
                }
                else
                {
                    PRINT_DEBUG("[Modem]: Command failed, retry\r\n");
                    fsmCmdSts = MODEM_CMD_SEND;
                }

            }
            break;

        case MODEM_CMD_GETANS:
            if (!isTimerExpired(tmr))
            {
                if ((cmd->spcHndlr & MODEM_SHDL_SMS_MULTIANS) == 0u)
                {
                    if (UartModemHdlrSearchAns(cmd->resp, strlen(cmd->resp), &ansIdx) == COM_MODEM_HDLR_OK)
                    {
                        UartModemHdlrGetAnsByIdx(ansIdx, &localAnsPnt, &localAnsLen);

                        *cAnsLen = 0u;
                        /* TODO: Improve if condition, for example define the "OK" in the cmd struct */
                        for (idx=0;idx<localAnsLen;idx++, (*cAnsLen)++)
                        {
                            cAns[*cAnsLen] = localAnsPnt[idx];
                        }
                        //*cAnsLen = localAnsLen;
                        cAns[*cAnsLen] = 0;
                        UartModemHdlrClearRxBuffer();
                        result = MODEM_HDLR_OK;
                        fsmCmdSts = MODEM_CMD_IDLE;
                    }
                }
                else
                {
                    if (UartModemHdlrSearchAns(cmd->finalResp, strlen(cmd->finalResp), &andIdx2) == COM_MODEM_HDLR_OK)
                    {
                        if (UartModemHdlrSearchAns(cmd->resp, strlen(cmd->resp), &ansIdx) == COM_MODEM_HDLR_OK)
                        {
                            *cAnsLen = 0u;
                            UartModemHdlrGetAnsByIdx(ansIdx, &localAnsPnt, &localAnsLen);

                            for (idx=0;idx<localAnsLen;idx++, (*cAnsLen)++)
                            {
                                cAns[*cAnsLen] = localAnsPnt[idx];
                            }
                            if ((cmd->spcHndlr & MODEM_SHDL_SMS_MULTIANS) != 0u)
                            {
                                do
                                {
                                    ansIdx++;
                                    UartModemHdlrGetAnsByIdx(ansIdx, &localAnsPnt, &localAnsLen);
                                    for (idx=0;idx<localAnsLen;idx++, (*cAnsLen)++)
                                    {
                                        cAns[*cAnsLen] = localAnsPnt[idx];
                                    }
                                }
                                while(ansIdx < andIdx2);
                            }

                        }

                        //*cAnsLen = localAnsLen;
                        cAns[*cAnsLen] = 0;
                        UartModemHdlrClearRxBuffer();
                        result = MODEM_HDLR_OK;
                        fsmCmdSts = MODEM_CMD_IDLE;

                    }
                }
            }
            else
            {
                retryCnt--;

                if (retryCnt == 0u)
                {
                    result = MODEM_HDLR_ERR;
                    fsmCmdSts = MODEM_CMD_IDLE;
                }
                else
                {
                    PRINT_DEBUG("[Modem]: Command failed, retry\r\n");
                    fsmCmdSts = MODEM_CMD_SEND;
                }
            }
            break;

        case MODEM_CMD_IDLE:
            break;

    }

    return result;
}

boolean ModemHdlrIsHdlrBusy(void)
{
    boolean isBusy = FALSE;

    if (fsmsts != MODEM_HDLR_IDLE)
    {
        isBusy = TRUE;
    }

    return isBusy;
}

boolean ModemHdlrIsCmdHdlrBusy(void)
{
    boolean isBusy = FALSE;

    if (fsmCmdSts != MODEM_CMD_IDLE)
    {
        isBusy = TRUE;
    }

    return isBusy;
}

boolean ModemHdlrIsCmdHdlrFinish(void)
{
    boolean isFinish = FALSE;

    if (fsmCmdSts == MODEM_CMD_IDLE)
    {
        isFinish = TRUE;
    }

    return isFinish;
}

ModemHdlrErrCode ModemHdlrGetSignalLevel(uint8_t *rssi, uint8_t *ber)
{
    ModemHdlrErrCode result = MODEM_HDLR_BUSY;
    static ModemRequestFsmSts fsmReqSts = MODEM_REQ_START;

    if ( (ModemHdlrIsHdlrBusy() == FALSE) &&
          (ModemHdlrIsCmdHdlrBusy() == FALSE) )
    {
        switch (fsmReqSts)
        {
         case MODEM_REQ_START:
            fsmsts = MODEM_HDLR_GET_SIGNAL_LEVEL;
            fsmCmdSts = MODEM_CMD_SEND;
            fsmReqSts = MODEM_REQ_WAIT;
            result = MODEM_HDLR_OK;
            break;

         case MODEM_REQ_WAIT:
            /* Command execution is finished */
            /* Copy parameters */
            sscanf(gcAns, "+CSQ: %d, %d", rssi, ber);
            fsmCmdSts = MODEM_CMD_IDLE;
            fsmReqSts = MODEM_REQ_START;
            result = MODEM_HDLR_OK;
            break;
        }

    }
    return result;
}

ModemHdlrErrCode ModemHdlrGetDateTime(uint8_t *year, uint8_t *month, uint8_t *day, uint8_t *hour, uint8_t *min, uint8_t *sec)
{
    ModemHdlrErrCode result = MODEM_HDLR_BUSY;
    static ModemRequestFsmSts fsmReqSts = MODEM_REQ_START;

    if ( (ModemHdlrIsHdlrBusy() == FALSE) &&
          (ModemHdlrIsCmdHdlrBusy() == FALSE) )
    {
        switch (fsmReqSts)
        {
         case MODEM_REQ_START:
            /* Modem handler is idle, issue the command */
            fsmsts = MODEM_HDLR_GET_DATE_TIME;
            fsmCmdSts = MODEM_CMD_SEND;
            fsmReqSts = MODEM_REQ_WAIT;
            result = MODEM_HDLR_OK;
            break;

         case MODEM_REQ_WAIT:
            /* Command execution is finished */
            /* Copy parameters */
            sscanf(gcAns, "+CCLK: \"%d/%d/%d,%d:%d:%d", year, month, day, hour, min, sec);
            fsmCmdSts = MODEM_CMD_IDLE;
            fsmReqSts = MODEM_REQ_START;
            result = MODEM_HDLR_OK;
            break;
        }
    }
    return result;
}

ModemHdlrErrCode ModemHdlrSendSMS(char *msg, char *num)
{
    ModemHdlrErrCode result = MODEM_HDLR_BUSY;
    static ModemRequestFsmSts fsmReqSts = MODEM_REQ_START;

    if ( (ModemHdlrIsHdlrBusy() == FALSE) &&
          (ModemHdlrIsCmdHdlrBusy() == FALSE) )
    {
        switch (fsmReqSts)
        {
         case MODEM_REQ_START:
            /* Modem handler is idle, issue the command */
            fsmsts = MODEM_HDLR_SEND_SMS;
            fsmCmdSts = MODEM_CMD_SEND;
            fsmReqSts = MODEM_REQ_WAIT;
            gMsg = msg;
            gNum = num;
            result = MODEM_HDLR_OK;
            break;

         case MODEM_REQ_WAIT:
            /* Command execution is finished */
            /* Copy parameters */
            fsmCmdSts = MODEM_CMD_IDLE;
            fsmReqSts = MODEM_REQ_START;
            result = MODEM_HDLR_OK;
            break;
        }
    }
    return result;

}

ModemHdlrErrCode ModemHdlrReadSMS(uint8_t msgIdx, char *msg, char *num)
{
    ModemHdlrErrCode result = MODEM_HDLR_BUSY;
    static ModemRequestFsmSts fsmReqSts = MODEM_REQ_START;

    if ( (ModemHdlrIsHdlrBusy() == FALSE) &&
          (ModemHdlrIsCmdHdlrBusy() == FALSE) )
    {
        switch (fsmReqSts)
        {
         case MODEM_REQ_START:
            /* Modem handler is idle, issue the command */
            fsmsts = MODEM_HDLR_READ_SMS;
            fsmCmdSts = MODEM_CMD_SEND;
            fsmReqSts = MODEM_REQ_WAIT;
            par1 = msgIdx;
            result = MODEM_HDLR_OK;
            break;

         case MODEM_REQ_WAIT:
            /* Command execution is finished */
            /* Copy parameters */
            if (strncmp(gcAns, modemCmdList[MODEM_CMD_ATCMGR].resp, strlen(modemCmdList[MODEM_CMD_ATCMGR].resp)) == 0u)
            {
                (void)StringExtract(gcAns, "\"", 3, num);
                (void)StringExtract(gcAns, "\"", 7, msg);
                /* Remove OK */
                msg[strlen(msg)-2] = 0;
                result = MODEM_HDLR_OK;
            }
            else
            {
                result = MODEM_HDLR_ERR;
            }

            fsmCmdSts = MODEM_CMD_IDLE;
            fsmReqSts = MODEM_REQ_START;

            break;
        }
    }
    return result;

}


ModemHdlrErrCode ModemHdlrDeleteSMS(uint8_t index, uint8_t deflag)
{

    ModemHdlrErrCode result = MODEM_HDLR_BUSY;
    static ModemRequestFsmSts fsmReqSts = MODEM_REQ_START;

    if ( (ModemHdlrIsHdlrBusy() == FALSE) &&
          (ModemHdlrIsCmdHdlrBusy() == FALSE) )
    {
        switch (fsmReqSts)
        {
         case MODEM_REQ_START:
            /* Modem handler is idle, issue the command */
            fsmsts = MODEM_HDLR_DELETE_SMS;
            fsmCmdSts = MODEM_CMD_SEND;
            fsmReqSts = MODEM_REQ_WAIT;
            par1 = index;
            par2 = deflag;
            result = MODEM_HDLR_OK;
            break;

         case MODEM_REQ_WAIT:
            /* Command execution is finished */
            fsmCmdSts = MODEM_CMD_IDLE;
            fsmReqSts = MODEM_REQ_START;
            result = MODEM_HDLR_OK;
            break;
        }
    }

    return result;
}
