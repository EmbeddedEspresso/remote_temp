/**
  ******************************************************************************
  * @file           : ComHdlrModem.c
  * @brief          : Modem Uart Handler
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

static ComModemHdlrErrCode UartDebugTx (void);
static ComModemHdlrErrCode UartDebugRx (void);

static ComModemHdlrFsmSts fsmsts = COM_MODEM_HDLR_INIT;
static ComModemHdlrFsmSts fsmststx = COM_MODEM_HDLR_TX_IDLE;
static ComModemHdlrFsmSts fsmstsrx = COM_MODEM_HDLR_RX_CHECK;

static UART_HandleTypeDef *currChannel;

static uint8_t *currBufftx;
static uint32_t currSizetx;
static uint32_t currTransfPostx;

static uint8_t currBuffrx[256];
static uint32_t currSizerx=0;
static uint32_t currTransfPosrx;

static ComModemHdlrErrCode UartHdlrTx (void)
{
    ComModemHdlrErrCode result = COM_MODEM_HDLR_OK;

    switch (fsmststx)
    {
        case COM_MODEM_HDLR_TX_IDLE:

            break;

        case COM_MODEM_HDLR_TX_CHECK:
            if ((currChannel->Instance->SR & UART_FLAG_TXE) == UART_FLAG_TXE)
            {
                fsmststx = COM_MODEM_HDLR_TX_BYTE;
            }
            break;

        case COM_MODEM_HDLR_TX_BYTE:
            currChannel->Instance->DR = currBufftx[currTransfPostx];
            currTransfPostx++;
            if (currTransfPostx < currSizetx)
            {
                fsmststx = COM_MODEM_HDLR_TX_CHECK;
            }
            else
            {
                currTransfPostx = 0;
                fsmststx = COM_MODEM_HDLR_TX_IDLE;
            }
            break;
    }

    return result;
}

static ComModemHdlrErrCode UartHdlrRx (void)
{
    ComModemHdlrErrCode result = COM_MODEM_HDLR_OK;

//    switch (fsmstsrx)
//    {
//        case COM_MODEM_HDLR_RX_CHECK:
//            if ((currChannel->Instance->SR & UART_FLAG_RXNE) == UART_FLAG_RXNE)
//            {
//                fsmstsrx = COM_MODEM_HDLR_RX_BYTE;
//            }
//            break;
//
//        case COM_MODEM_HDLR_RX_BYTE:
//            if (currTransfPosrx <= 255)
//            {
//                currBuffrx[currTransfPosrx] = currChannel->Instance->DR;
//                currTransfPosrx++;
//            }
//            else
//            {
//                /* Buffer overflow, ignore extra bytes */
//                currChannel->Instance->DR;
//            }
//            fsmstsrx = COM_MODEM_HDLR_RX_CHECK;
//            break;
//    }

    return result;

}

ComModemHdlrErrCode UartModemHdlrInit (UART_HandleTypeDef *ch)
{
    currChannel = ch;
}

ComModemHdlrErrCode UartModemHdlrRun (void)
{
    ComModemHdlrErrCode result = COM_MODEM_HDLR_OK;

    switch (fsmsts)
    {
        case COM_MODEM_HDLR_INIT:
            MX_USART1_UART_Init();
            fsmsts = COM_MODEM_HDLR_COM_START;

            break;
        case COM_MODEM_HDLR_COM_START:
            UartHdlrTx();
            //UartHdlrRx();
            break;

    }

    return result;
}

/* Return the number of answers present in the queue */
uint32_t UartModemHdlrGetAnsNum(void)
{
    /* Store local actual value, avoid isr changes */
    uint32_t buffSize = currTransfPosrx;
    uint32_t ansCnt = 0;
    uint32_t idx;

    for(idx = 0;idx < buffSize; idx++)
    {
        if (currBuffrx[idx] == '\r')
        {
            ansCnt++;
        }
    }

    return ansCnt;
}

/* Return a specific answer based on the index */
ComModemHdlrErrCode UartModemHdlrGetAnsByIdx(uint32_t ansIdx, uint8_t **buff, uint32_t *size)
{
    /* Reach start of answer. Number of \r and ansIdx must match */
    uint32_t buffIdx = 0, buffLen = 0;
    /* Store local actual value, avoid isr changes */
    uint32_t buffSize = currTransfPosrx;
    ComModemHdlrErrCode result = COM_MODEM_HDLR_OK;

    while( (buffIdx < buffSize) && (ansIdx > 0) )
    {
        if (currBuffrx[buffIdx] == '\r')
        {
            ansIdx--;
        }
        buffIdx++;
    }

    if (ansIdx == 0)
    {
        *buff = &currBuffrx[buffIdx];

        while( (buffIdx < buffSize) && (currBuffrx[buffIdx] != '\r') )
        {
            buffIdx++;
            buffLen++;
        }

        *size = buffLen;
    }
    else
    {
        result = COM_MODEM_HDLR_ERR;
    }

    return result;

}

/* Remove a specific answer in the buffer */
ComModemHdlrErrCode UartModemHdlrRemoveAns(uint32_t ansIdx)
{
    /* Reach start of answer. Number of \r and ansIdx must match */
    uint32_t buffIdx = 0, buffLen = 0, startIdx;
    /* Store local actual value, avoid isr changes */
    uint32_t buffSize = currTransfPosrx;
    ComModemHdlrErrCode result = COM_MODEM_HDLR_OK;

    /* Interrupt must not interrupt the rx buffer access */
    __disable_irq();

    while( (buffIdx < buffSize) && (ansIdx > 0) )
    {
        if (currBuffrx[buffIdx] == '\r')
        {
            ansIdx--;
        }
        buffIdx++;
    }

    if (ansIdx == 0)
    {
        startIdx = buffIdx;

        while( (buffIdx < buffSize) && (currBuffrx[buffIdx] != '\r') )
        {
            buffIdx++;
            buffLen++;
        }

        buffLen++;

        if (currBuffrx[buffIdx] != '\r')
        {
            result = COM_MODEM_HDLR_ERR;
        }
        else
        {
            if (buffIdx >= buffLen)
            {
                while(buffIdx < buffSize)
                {
                    currBuffrx[buffIdx - buffLen] = currBuffrx[buffIdx];
                    buffIdx++;
                }
            }
            else
            {
                /* Shifting mechanism do not work if it is the first answer */
                for(startIdx = 0; startIdx < (buffSize-buffLen); startIdx++)
                {
                    currBuffrx[startIdx] = currBuffrx[startIdx + buffLen];
                }
            }
            currTransfPosrx -= buffLen;
        }
    }
    else
    {
        result = COM_MODEM_HDLR_ERR;
    }


    /* Re-enable interrupts */
    __enable_irq();

    return result;

}

/* Empty the buffer */
void UartModemHdlrClearRxBuffer(void)
{
    uint32_t idx;

    /* Interrupt must not interrupt the rx buffer access */
    __disable_irq();

    for(idx = 0; idx < currTransfPosrx; idx++)
    {
        currBuffrx[idx] = 0u;
    }

    currTransfPosrx = 0;

    /* Interrupt must not interrupt the rx buffer access */
    __enable_irq();
}

/* Check if a specific answer is present */
ComModemHdlrErrCode UartModemHdlrSearchAns(uint8_t *buff, uint32_t size, uint32_t *ansIdx)
{
    ComModemHdlrErrCode result = COM_MODEM_HDLR_ERR;
    uint32_t ansNum, idx=0, localLen;
    uint8_t isFound=0;
    uint8_t *localBuff;

    while( (idx < UartModemHdlrGetAnsNum()) && (isFound==0) )
    {
        UartModemHdlrGetAnsByIdx(idx, &localBuff, &localLen);
        if (!strncmp(buff, localBuff, size))
        {
            isFound = 1;
            *ansIdx = idx;
            result = COM_MODEM_HDLR_OK;
        }
        idx++;
    }
    return result;
}

ComModemHdlrErrCode UartModemHdlrTx(uint8_t *buff, uint32_t size)
{
    ComModemHdlrErrCode result = COM_MODEM_HDLR_OK;

    if (fsmststx == COM_MODEM_HDLR_TX_IDLE)
    {
        currBufftx = buff;
        currSizetx = size;
        fsmststx = COM_MODEM_HDLR_TX_CHECK;
    }
    else
    {
        result = COM_MODEM_HDLR_BUSY;
    }
    return result;

}

ComModemHdlrErrCode UartModemHdlrRx(uint8_t *buff, uint32_t size)
{
    ComModemHdlrErrCode result = COM_MODEM_HDLR_OK;

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
        result = COM_MODEM_HDLR_NODATA;
    }
    return result;

}

ComModemHdlrErrCode UartModemHdlrFlushRx(void)
{
    currTransfPosrx = 0u;
}

void UartModelIsr(void)
{
    if ((currChannel->Instance->SR & UART_FLAG_RXNE) == UART_FLAG_RXNE)
    {
        if (currTransfPosrx <= 255)
        {
            currBuffrx[currTransfPosrx] = currChannel->Instance->DR;

            if ( (currTransfPosrx > 0) &&
                 ( (currBuffrx[currTransfPosrx - 1] == '>') &&
                  (currBuffrx[currTransfPosrx] == ' ') ) )
            {
                /* This is a specific answer during SMS handling */
                /* Add an extra \r, to avoid issues during answer analysis */
                currTransfPosrx++;
                currBuffrx[currTransfPosrx] = '\r';
            }

            if( !( (currBuffrx[currTransfPosrx] == '\n') ||
                   ( (currTransfPosrx > 0) &&
                     (currBuffrx[currTransfPosrx - 1] == '\r') &&
                     (currBuffrx[currTransfPosrx] == '\r') ) ||
                   ( (currTransfPosrx == 0) &&
                     (currBuffrx[currTransfPosrx] == '\r') ) ) )
            {
                /* This trick allows to have onlz one \r always as separator */
                currTransfPosrx++;
            }
        }
        else
        {
            /* Buffer overflow, ignore extra bytes */
            currChannel->Instance->DR;
        }
    }
}
