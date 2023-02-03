/**
  ******************************************************************************
  * @file           : ComHdlrModem.h
  * @brief          : Modem Uart Handler Header
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

#ifndef COM_HDLR_MODEM_H
#define COM_HDLR_MODEM_H

typedef enum
{
    COM_MODEM_HDLR_INIT = 0,
    COM_MODEM_HDLR_COM_START,
    COM_MODEM_HDLR_TX_IDLE,
    COM_MODEM_HDLR_TX_CHECK,
    COM_MODEM_HDLR_TX_BYTE,
    COM_MODEM_HDLR_RX_IDLE,
    COM_MODEM_HDLR_RX_CHECK,
    COM_MODEM_HDLR_RX_BYTE

}ComModemHdlrFsmSts;

typedef enum
{
    COM_MODEM_HDLR_OK = 0,
    COM_MODEM_HDLR_ERR,
    COM_MODEM_HDLR_BUSY,
    COM_MODEM_HDLR_NODATA
}ComModemHdlrErrCode;

ComModemHdlrErrCode UartModemHdlrInit(UART_HandleTypeDef *ch);
ComModemHdlrErrCode UartModemHdlrRun(void);
ComModemHdlrErrCode UartModemHdlrTx(uint8_t *buff, uint32_t size);
ComModemHdlrErrCode UartModemHdlrRx(uint8_t *buff, uint32_t size);
ComModemHdlrErrCode UartModemHdlrFlushRx(void);
uint32_t UartModemHdlrGetAnsNum(void);






#endif
