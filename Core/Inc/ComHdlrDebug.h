/**
  ******************************************************************************
  * @file           : ComHdlrDebug.h
  * @brief          : Debug Uart Handler Header
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

#ifndef COM_HDLR_DEBUG_H
#define COM_HDLR_DEBUG_H

typedef enum
{
    COM_DEBUG_HDLR_INIT = 0,
    COM_DEBUG_HDLR_COM_START,
    COM_DEBUG_HDLR_TX_IDLE,
    COM_DEBUG_HDLR_TX_CHECK,
    COM_DEBUG_HDLR_TX_BYTE,
    COM_DEBUG_HDLR_RX_IDLE,
    COM_DEBUG_HDLR_RX_CHECK,
    COM_DEBUG_HDLR_RX_BYTE
}ComDebugHdlrFsmSts;

typedef enum
{
    COM_DEBUG_HDLR_OK = 0,
    COM_DEBUG_HDLR_BUSY,
    COM_DEBUG_HDLR_NODATA
}ComDebugHdlrErrCode;

ComDebugHdlrErrCode UartDebugHdlrInit(UART_HandleTypeDef *ch);
ComDebugHdlrErrCode UartDebugHdlrRun(void);
ComDebugHdlrErrCode UartDebugHdlrTx(uint8_t *buff, uint32_t size);
ComDebugHdlrErrCode UartDebugHdlrRx(uint8_t *buff, uint32_t size);
ComDebugHdlrErrCode UartDebugHdlrFlushRx(void);







#endif
