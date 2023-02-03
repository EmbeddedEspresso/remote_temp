/**
  ******************************************************************************
  * @file           : Timer.h
  * @brief          : Basic timer handler header
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

#ifndef INC_TIMER_H_
#define INC_TIMER_H_

void TimerSet(int *timer, int expval);
int isTimerExpired(int timer);


#endif /* INC_TIMER_H_ */
