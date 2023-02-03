/**
  ******************************************************************************
  * @file           : TempSensorHdlr.c
  * @brief          : Temperature Sensor Handler
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
    TEMPSENSOR_HDLR_INIT = 0,
    TEMPSENSOR_HDLR_IDLE,
    TEMPSENSOR_HDLR_READTEMP,
    TEMPSENSOR_HDLR_READTEMP_WAIT,
    TEMPSENSOR_HDLR_ERR_STS

}TempSensorHdlrFsmSts;

static TempSensorHdlrFsmSts fsmsts = TEMPSENSOR_HDLR_INIT;
static char debugLocalStr[256];
static float TempSensorTempVal;
static boolean TempSensorIsValid = FALSE;
TempSensorHdlrErrCode TempSensorHdlrInit (void)
{
    fsmsts = TEMPSENSOR_HDLR_INIT;
}

TempSensorHdlrErrCode TempSensorHdlrRun (void)
{
    TempSensorHdlrErrCode result = TEMPSENSOR_HDLR_OK;
    static int tmr;
    uint32_t ansIdx;
    static char msg[256];
    static char num[32];
    ModemHdlrErrCode cmdsts;

    uint16_t devAddress = 0x30;
    uint8_t tempReg = 0x5;
    static uint8_t dataReg[2] = {0};
    uint16_t dataRegLong;

    float tempVal = 0;
    float tempValDec;

    switch (fsmsts)
    {
        case TEMPSENSOR_HDLR_INIT:
            PRINT_DEBUG("[TempSensor]: Initialization completed\r\n");
            fsmsts = TEMPSENSOR_HDLR_IDLE;
            TimerSet(&tmr, 8000);
            TempSensorTempVal = 0u;
            break;

        case TEMPSENSOR_HDLR_IDLE:
            if (isTimerExpired(tmr))
            {
                TimerSet(&tmr, 8000);
                fsmsts = TEMPSENSOR_HDLR_READTEMP;
            }
            break;

        case TEMPSENSOR_HDLR_READTEMP:
            /* Address the temperature register */
            /* This implementation could be substituted by a non-blocking one */
            /* Consider to provide an API for reading a register, which does write and read */
            dataReg[0] = dataReg[1] = 0x00u;
            HAL_I2C_Master_Transmit(&hi2c1, devAddress, &tempReg, 1, 2000u);
            fsmsts = TEMPSENSOR_HDLR_READTEMP_WAIT;
            break;

        case TEMPSENSOR_HDLR_READTEMP_WAIT:

          HAL_I2C_Master_Receive(&hi2c1, devAddress | 0x01, dataReg, 2, 2000u);

          dataRegLong = ((dataReg[0] << 8u) | dataReg[1]);

          /* extract integer part */
          tempVal = ((dataRegLong & 0x0FFF) >> 4);
          /* Extract decimal part */
          tempValDec = 0.0625;
          for (int i=0;i<4;i++)
          {
              tempVal += ((dataRegLong >> i) & 0x0001) * tempValDec;
              tempValDec *= 2u;
          }
          //tempVal = 20.5;
          sprintf(debugLocalStr, "[TempSensor]: Temperature sampled ( %.2f °C )\r\n", tempVal);
          PRINT_DEBUG(debugLocalStr);
          TempSensorTempVal = tempVal;
          TempSensorIsValid = TRUE;
          fsmsts = TEMPSENSOR_HDLR_IDLE;
          break;


        case TEMPSENSOR_HDLR_ERR_STS:
            break;

    }

    return result;
}

TempSensorHdlrErrCode TempSensorGetValue (float *val)
{
    TempSensorHdlrErrCode retval = TEMPSENSOR_HDLR_ERR;

    if (TempSensorIsValid == TRUE)
    {
        *val = TempSensorTempVal;
        retval = TEMPSENSOR_HDLR_OK;
    }

    return retval;
}
