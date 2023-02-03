/**
  ******************************************************************************
  * @file           : ModemHdlr.h
  * @brief          : Modem Handler Header
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

#ifndef MODEM_HDLR_H
#define MODEM_HDLR_H

typedef enum
{
    MODEM_HDLR_OK = 0,
    MODEM_HDLR_BUSY,
    MODEM_HDLR_NODATA,
    MODEM_HDLR_ERR
}ModemHdlrErrCode;

#define MODEM_HDLR_STS_SMS_READY    0x00000001uL
#define MODEM_HDLR_STS_CALL_READY    0x00000002uL
#define MODEM_HDLR_STS_SMS_RECEIVED    0x00000004uL

#define ModemHdlrSetSMSReady()         (ModemSts |= MODEM_HDLR_STS_SMS_READY)
#define ModemHdlrSetCallReady()     (ModemSts |= MODEM_HDLR_STS_CALL_READY)
#define ModemHdlrSetSMSReceived()     (ModemSts |= MODEM_HDLR_STS_SMS_RECEIVED)

#define ModemHdlrClrSMSReady()         (ModemSts &= ~MODEM_HDLR_STS_SMS_READY)
#define ModemHdlrClrCallReady()     (ModemSts &= ~MODEM_HDLR_STS_CALL_READY)
#define ModemHdlrClrSMSReceived()     (ModemSts &= ~MODEM_HDLR_STS_SMS_RECEIVED)

#define ModemHdlrIsSMSReceived()     ( (ModemSts & MODEM_HDLR_STS_SMS_RECEIVED) == MODEM_HDLR_STS_SMS_RECEIVED)
#define ModemHdlrGetSmsIndex()        (SmsRecIdx)

extern uint32_t ModemSts;
extern uint8_t SmsRecIdx;

ModemHdlrErrCode ModemHdlrInit(void);
ModemHdlrErrCode ModemHdlrRun(void);
boolean ModemHdlrIsHdlrBusy(void);
ModemHdlrErrCode ModemHdlrGetSignalLevel(uint8_t *rssi, uint8_t *ber);
ModemHdlrErrCode ModemHdlrGetDateTime(uint8_t *year, uint8_t *month, uint8_t *day, uint8_t *hour, uint8_t *min, uint8_t *sec);
ModemHdlrErrCode ModemHdlrSendSMS(char *msg, char *num);
ModemHdlrErrCode ModemHdlrReadSMS(uint8_t msgIdx, char *msg, char *num);
ModemHdlrErrCode ModemHdlrDeleteSMS(uint8_t index, uint8_t deflag);







#endif
