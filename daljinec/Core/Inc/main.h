/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define NRF_IRQ_Pin GPIO_PIN_0
#define NRF_IRQ_GPIO_Port GPIOB
#define NRF_IRQ_EXTI_IRQn EXTI0_IRQn
#define BUTTON_GREEN_Pin GPIO_PIN_1
#define BUTTON_GREEN_GPIO_Port GPIOB
#define TOGGLE_R_Pin GPIO_PIN_2
#define TOGGLE_R_GPIO_Port GPIOB
#define BUTTON_RED_Pin GPIO_PIN_10
#define BUTTON_RED_GPIO_Port GPIOB
#define TOGGLE_L_Pin GPIO_PIN_12
#define TOGGLE_L_GPIO_Port GPIOB
#define NRF_CSN_Pin GPIO_PIN_13
#define NRF_CSN_GPIO_Port GPIOB
#define NRF_CE_Pin GPIO_PIN_14
#define NRF_CE_GPIO_Port GPIOB
#define ENC_B_L_Pin GPIO_PIN_9
#define ENC_B_L_GPIO_Port GPIOA
#define ENC_A_L_Pin GPIO_PIN_10
#define ENC_A_L_GPIO_Port GPIOA
#define ENC_A_R_Pin GPIO_PIN_11
#define ENC_A_R_GPIO_Port GPIOA
#define ENC_B_R_Pin GPIO_PIN_12
#define ENC_B_R_GPIO_Port GPIOA
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
