/**
  ******************************************************************************
  * @file           : Utils.c
  * @brief          : Series of general purpose functions
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

/* Helpers function */

tReturn StringExtract(char *inStr, char *delimiter, uint8_t pos, char *outStr)
{
    boolean result = RET_OK;
    char localStr[256];
    char *pTokStr;
    uint8_t tokCnt = pos;

    if (strlen(inStr) > 255)
    {
        result = RET_ERROR;
    }
    else
    {
        strcpy(localStr, inStr);
        pTokStr = strtok(localStr, delimiter);

        if (pTokStr == NULL)
        {
            result = RET_ERROR;
        }
        else
        {
            while( (pTokStr != NULL) && (tokCnt != 0) )
            {
                pTokStr = strtok(NULL, delimiter);
                tokCnt--;
            }
        }

        if (result == RET_OK)
        {
            if(tokCnt > 0)
            {
                result = RET_ERROR;
            }
            else
            {
                /* Copy string */
                strcpy (outStr, pTokStr);
            }
        }
    }

    return result;

}
