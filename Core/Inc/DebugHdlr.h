/**
  ******************************************************************************
  * @file           : DebugHdlr.h
  * @brief          : Debug Handler Header
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

#ifndef DEBUG_HDLR_H
#define DEBUG_HDLR_H

typedef enum
{
    DEBUG_HDLR_OK = 0,
    DEBUG_HDLR_BUSY,
    DEBUG_HDLR_NODATA
}DebugHdlrErrCode;

DebugHdlrErrCode DebugHdlrInit(void);
DebugHdlrErrCode DebugHdlrRun(void);
DebugHdlrErrCode DebugHdlrPrintMsg(uint8_t *buff);

#define PRINT_DEBUG(msg) DebugHdlrPrintMsg(msg)

#endif
