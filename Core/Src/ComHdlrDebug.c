/**
  ******************************************************************************
  * @file           : ComHdlrDebug.c
  * @brief          : Debug Uart Handler
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

#define UART_RX_BUFFER_SIZE  256
#define UART_DEBUG_MAX_PEND_JOB 10u

typedef struct
{
    uint8_t *buff;
    uint32_t size;
}ComDebugPendJob;

static ComDebugHdlrErrCode UartDebugTx (void);
static ComDebugHdlrErrCode UartDebugRx (void);

static ComDebugHdlrFsmSts fsmsts = COM_DEBUG_HDLR_INIT;
static ComDebugHdlrFsmSts fsmststx = COM_DEBUG_HDLR_TX_IDLE;
static ComDebugHdlrFsmSts fsmstsrx = COM_DEBUG_HDLR_RX_CHECK;

static UART_HandleTypeDef *currChannel;

static uint8_t *currBufftx;
static uint32_t currSizetx;
static uint32_t currTransfPostx;

static uint8_t currBuffrx[256];
static uint32_t currSizerx=0;
static uint32_t currTransfPosrx;

static ComDebugPendJob pendJob[UART_DEBUG_MAX_PEND_JOB];
static uint8_t pendJobNum = 0;

static ComDebugHdlrErrCode UartHdlrTx (void)
{
    ComDebugHdlrErrCode result = COM_DEBUG_HDLR_OK;

    switch (fsmststx)
    {
        case COM_DEBUG_HDLR_TX_IDLE:

            break;

        case COM_DEBUG_HDLR_TX_CHECK:
            if ((currChannel->Instance->SR & UART_FLAG_TXE) == UART_FLAG_TXE)
            {
                fsmststx = COM_DEBUG_HDLR_TX_BYTE;
            }
            break;

        case COM_DEBUG_HDLR_TX_BYTE:
            currChannel->Instance->DR = currBufftx[currTransfPostx];
            currTransfPostx++;
            if (currTransfPostx < currSizetx)
            {
                fsmststx = COM_DEBUG_HDLR_TX_CHECK;
            }
            else
            {
                currTransfPostx = 0;
                fsmststx = COM_DEBUG_HDLR_TX_IDLE;
            }
            break;
    }

    return result;
}

static ComDebugHdlrErrCode UartHdlrRx (void)
{
    ComDebugHdlrErrCode result = COM_DEBUG_HDLR_OK;

    switch (fsmstsrx)
    {
        case COM_DEBUG_HDLR_RX_CHECK:
            if ((currChannel->Instance->SR & UART_FLAG_RXNE) == UART_FLAG_RXNE)
            {
                fsmstsrx = COM_DEBUG_HDLR_RX_BYTE;
            }
            break;

        case COM_DEBUG_HDLR_RX_BYTE:
            if (currTransfPosrx <= 255)
            {
                currBuffrx[currTransfPosrx] = currChannel->Instance->DR;
                currTransfPosrx++;
            }
            else
            {
                /* Buffer overflow, ignore extra bytes */
                currChannel->Instance->DR;
            }
            fsmstsrx = COM_DEBUG_HDLR_RX_CHECK;
            break;
    }

    return result;

}

ComDebugHdlrErrCode UartDebugHdlrInit (UART_HandleTypeDef *ch)
{
    currChannel = ch;
}

ComDebugHdlrErrCode UartDebugHdlrRun (void)
{
    ComDebugHdlrErrCode result = COM_DEBUG_HDLR_OK;
    int idx;

    switch (fsmsts)
    {
        case COM_DEBUG_HDLR_INIT:
            MX_USART2_UART_Init();
            fsmsts = COM_DEBUG_HDLR_COM_START;
            break;

        case COM_DEBUG_HDLR_COM_START:
            UartHdlrTx();
            UartHdlrRx();
            if ( (fsmststx == COM_DEBUG_HDLR_TX_IDLE) &&
                 (pendJobNum != 0) )
            {
                currBufftx = pendJob[0].buff;
                currSizetx = pendJob[0].size;
                fsmststx = COM_DEBUG_HDLR_TX_CHECK;
                pendJobNum--;
                for(idx = 0; idx < (UART_DEBUG_MAX_PEND_JOB-1); idx++)
                {
                    pendJob[idx].buff = pendJob[idx+1].buff;
                    pendJob[idx].size = pendJob[idx+1].size;
                }
            }
            break;

    }

    return result;
}

ComDebugHdlrErrCode UartDebugHdlrTx(uint8_t *buff, uint32_t size)
{
    ComDebugHdlrErrCode result = COM_DEBUG_HDLR_OK;

    if (pendJobNum < UART_DEBUG_MAX_PEND_JOB)
    {
        pendJob[pendJobNum].buff = buff;
        pendJob[pendJobNum].size = size;
        pendJobNum++;
    }
    else
    {
        result = COM_DEBUG_HDLR_BUSY;
    }

    return result;
}

ComDebugHdlrErrCode UartDebugHdlrRx(uint8_t *buff, uint32_t size)
{
    ComDebugHdlrErrCode result = COM_DEBUG_HDLR_OK;

    if (currTransfPosrx >= size)
    {
        /* Enough data to read */
        for (int idx = 0;idx < size;idx ++)
        {
            buff[idx] = currBuffrx[idx];
        }
        for (int idx = 0;idx < (256-size);idx ++)
        {
            currBuffrx[idx] = currBuffrx[idx+size];
        }
        currTransfPosrx -= size;
    }
    else
    {
        result = COM_DEBUG_HDLR_NODATA;
    }
    return result;

}

ComDebugHdlrErrCode UartDebugHdlrFlushRx(void)
{
    currTransfPosrx = 0u;
}
