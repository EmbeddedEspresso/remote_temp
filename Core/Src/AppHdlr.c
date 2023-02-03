/**
  ******************************************************************************
  * @file           : AppHdlr.c
  * @brief          : Application Handler
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
    APP_HDLR_INIT = 0,
    APP_HDLR_IDLE,
    APP_HDLR_SMS_RECEIVED,
    APP_HDLR_SMS_GETCONTENT,
    APP_HDLR_SMS_CHECKCONTENT,
    APP_HDLR_SMS_SENDRESP,
    APP_HDLR_SMS_SENDRESPWAIT,
    APP_HDLR_ERR_STS

}AppHdlrFsmSts;

static AppHdlrFsmSts fsmsts = APP_HDLR_INIT;
static char debugLocalStr[256];

AppHdlrErrCode AppHdlrInit (void)
{
    fsmsts = APP_HDLR_INIT;
}

AppHdlrErrCode AppHdlrRun (void)
{
    AppHdlrErrCode result = APP_HDLR_OK;
    static int tmr;
    uint32_t ansIdx;
    static char msg[256];
    static char num[32];
    ModemHdlrErrCode cmdsts;
    float tempVal;

    switch (fsmsts)
    {
        case APP_HDLR_INIT:
            PRINT_DEBUG("[Application]: Initialization completed\r\n");
            fsmsts = APP_HDLR_IDLE;
            break;

        case APP_HDLR_IDLE:
            if (ModemHdlrIsSMSReceived())
            {
                PRINT_DEBUG("[Application]: SMS received\r\n");
                ModemHdlrClrSMSReceived();
                fsmsts = APP_HDLR_SMS_RECEIVED;
            }
            break;

        case APP_HDLR_SMS_RECEIVED:
            /* Read the SMS */
            if (ModemHdlrReadSMS(ModemHdlrGetSmsIndex(),  msg, num) == MODEM_HDLR_OK)
            {
                PRINT_DEBUG("[Application]: Reading SMS content\r\n");
                fsmsts = APP_HDLR_SMS_GETCONTENT;
            }
            else
            {
                PRINT_DEBUG("[Application]: Reading SMS failed\r\n");
                fsmsts = APP_HDLR_IDLE;
            }
            break;

        case APP_HDLR_SMS_GETCONTENT:
            cmdsts = ModemHdlrReadSMS(ModemHdlrGetSmsIndex(), msg, num);
            if (cmdsts == MODEM_HDLR_OK)
            {
                sprintf(debugLocalStr, "[Application]: SMS received from number: %s\r\n[Application]: SMS content: %s\r\n", num, msg);
                PRINT_DEBUG(debugLocalStr);
                fsmsts = APP_HDLR_SMS_CHECKCONTENT;
            }
            else if (cmdsts == MODEM_HDLR_ERR)
            {
                PRINT_DEBUG("[Application]: Reading SMS failed\r\n");
                fsmsts = APP_HDLR_IDLE;
            }

            break;

        case APP_HDLR_SMS_CHECKCONTENT:
            if (strncmp(msg, "Get Temperature", 15) == 0u)
            {
                PRINT_DEBUG("[Application]: Get Temperature request detected\r\n");
                fsmsts = APP_HDLR_SMS_SENDRESP;
            }
            else
            {
                PRINT_DEBUG("[Application]: Unknown request\r\n");
                fsmsts = APP_HDLR_IDLE;
            }
            break;

        case APP_HDLR_SMS_SENDRESP:
            if (TempSensorGetValue(&tempVal) == TEMPSENSOR_HDLR_OK)
            {
                sprintf(msg, "Temperature home is %.2f", tempVal);
            }
            else
            {
                sprintf(msg, "Temperature home is not available", tempVal);
            }

            if (ModemHdlrSendSMS(msg, num) == MODEM_HDLR_OK)
            {
                PRINT_DEBUG("[Application]: Sending SMS content\r\n");
                fsmsts = APP_HDLR_SMS_SENDRESPWAIT;
            }
            else
            {
                PRINT_DEBUG("[Application]: Sending SMS failed\r\n");
                fsmsts = APP_HDLR_IDLE;
            }
            break;


        case APP_HDLR_SMS_SENDRESPWAIT:
            cmdsts = ModemHdlrSendSMS(msg, num);
            if (cmdsts == MODEM_HDLR_OK)
            {
                PRINT_DEBUG("[Application]: SMS sent successfully\r\n");
                fsmsts = APP_HDLR_IDLE;
            }
            else if (cmdsts == MODEM_HDLR_ERR)
            {
                PRINT_DEBUG("[Application]: Sending SMS failed\r\n");
                fsmsts = APP_HDLR_IDLE;
            }
            break;


        case APP_HDLR_ERR_STS:
            break;

    }

    return result;
}
