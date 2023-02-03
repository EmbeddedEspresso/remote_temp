/**
  ******************************************************************************
  * @file           : Types.h
  * @brief          : Generic types
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

#ifndef INC_TYPES_H_
#define INC_TYPES_H_

typedef uint8_t boolean;
#define TRUE 0u
#define FALSE 1u

typedef enum
{
    RET_OK = 0u,
    RET_ERROR = 1u
}tReturn;

#endif /* INC_TYPES_H_ */
