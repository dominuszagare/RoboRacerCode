/*
 * robotTasks.h
 *
 *  Created on: Jan 5, 2022
 *      Author: ddomi
 */

#ifndef INC_ROBOTTASKS_H_
#define INC_ROBOTTASKS_H_
#include "main.h"


void StartCalculatingPoz(void *argument);
void StartRecivingCommandsNRF24(void *argument);
void StartMotorControl(void *argument);
void StartCalculatingPath(void *argument);
void StartTransmitingData(void *argument);


#endif /* INC_ROBOTTASKS_H_ */
