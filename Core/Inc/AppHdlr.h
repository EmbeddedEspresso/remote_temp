/**
  ******************************************************************************
  * @file           : AppHdlr.h
  * @brief          : Application Handler Header
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

#ifndef APP_HDLR_H
#define APP_HDLR_H

typedef enum
{
    APP_HDLR_OK = 0,
    APP_HDLR_BUSY,
    APP_HDLR_NODATA
}AppHdlrErrCode;

AppHdlrErrCode AppHdlrInit(void);
AppHdlrErrCode AppHdlrRun(void);

#endif
