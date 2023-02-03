/**
  ******************************************************************************
  * @file           : TempSensorHdlr.h
  * @brief          : Temperature Sensor Handler header
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

#ifndef TEMPSENSOR_HDLR_H
#define TEMPSENSOR_HDLR_H

typedef enum
{
    TEMPSENSOR_HDLR_OK = 0,
    TEMPSENSOR_HDLR_BUSY,
    TEMPSENSOR_HDLR_NODATA,
    TEMPSENSOR_HDLR_ERR
}TempSensorHdlrErrCode;

TempSensorHdlrErrCode TempSensorHdlrInit(void);
TempSensorHdlrErrCode TempSensorHdlrRun(void);
TempSensorHdlrErrCode TempSensorGetValue (float *val);

#endif
