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
#define STEVILO_CLENOV_TP 60
struct tekocePovprecjeInt16{
	uint8_t index;
	int16_t avrage;
	int32_t sum;
	int16_t vals[STEVILO_CLENOV_TP];
} M1,M2,M3,M4,AccX,AccY,AccZ,GyroX,GyroY,GyroZ,MagX,MagY,MagZ;
struct button{
	uint8_t flags;
	uint32_t presedConf;
	uint32_t relesedConf;
	GPIO_TypeDef* port;
	uint16_t pin;
	uint16_t debaunceCycles;
	uint8_t presses;
} B1; //moder gumb na ploscici
struct EMUdata{
	int16_t x;
	int16_t y;
	int16_t z;
} Acc, AccF, Gyro, GyroF, Mag, MagF;
struct motorData{
	int32_t poz;
	int32_t prevPoz;
	uint8_t num; //enumorator motorja
	float targetVel;
	float error;
	float prevError;
	float integralError;
	float vals[10];
	float sum;
	uint8_t index;
}motorLF, motorRF, motorRB, motorLB;
struct robotPayload{
	int8_t x1;
	int8_t y1;
	int8_t x2;
	int8_t y2;
	uint8_t speed;
	uint8_t crc; //sestej vse podatke v 16 bitno stevilo in vzami prvih 8 LSB
} robotPay;
struct SendIMU{
	uint16_t head;
	//uint16_t num;
	int16_t XAcc;
	int16_t YAcc;
	int16_t ZAcc;
	int16_t XGyro;
	int16_t YGyro;
	int16_t ZGyro;
	int16_t XMag;
	int16_t YMag;
	int16_t ZMag;
}IMUsend;
volatile struct CalculatePoz{
	uint32_t head;
	float pitch;
	float roll;
	float heading;
	float Q0;
	float Q1;
	float Q2;
	float Q3;
	float pozX;
	float pozY;
	float magX;
	float magY;
	float magZ;
}P;
volatile struct IMUError{
	int16_t Gyrox; //drift from 0
	int16_t Gyroy;
	int16_t Gyroz;
	float HardIronMagx;
	float HardIronMagy;
	float HardIronMagz;
	float SoftIronMagx;
	float SoftIronMagy;
	float SoftIronMagz;
	int16_t Accx;
	int16_t Accy;
	int16_t Accz;
}E;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define CS_SPI1_Pin GPIO_PIN_3
#define CS_SPI1_GPIO_Port GPIOE
#define PC14_OSC32_IN_Pin GPIO_PIN_14
#define PC14_OSC32_IN_GPIO_Port GPIOC
#define PC15_OSC32_OUT_Pin GPIO_PIN_15
#define PC15_OSC32_OUT_GPIO_Port GPIOC
#define PH0_OSC_IN_Pin GPIO_PIN_0
#define PH0_OSC_IN_GPIO_Port GPIOH
#define PH1_OSC_OUT_Pin GPIO_PIN_1
#define PH1_OSC_OUT_GPIO_Port GPIOH
#define OTG_FS_PowerSwitchOn_Pin GPIO_PIN_0
#define OTG_FS_PowerSwitchOn_GPIO_Port GPIOC
#define PDM_OUT_Pin GPIO_PIN_3
#define PDM_OUT_GPIO_Port GPIOC
#define me_rb_Pin GPIO_PIN_1
#define me_rb_GPIO_Port GPIOA
#define me_lb_Pin GPIO_PIN_2
#define me_lb_GPIO_Port GPIOA
#define me_lf_Pin GPIO_PIN_3
#define me_lf_GPIO_Port GPIOA
#define I2S3_WS_Pin GPIO_PIN_4
#define I2S3_WS_GPIO_Port GPIOA
#define SPI1_SCK_Pin GPIO_PIN_5
#define SPI1_SCK_GPIO_Port GPIOA
#define SPI1_MISO_Pin GPIO_PIN_6
#define SPI1_MISO_GPIO_Port GPIOA
#define SPI1_MOSI_Pin GPIO_PIN_7
#define SPI1_MOSI_GPIO_Port GPIOA
#define mf_lf_Pin GPIO_PIN_7
#define mf_lf_GPIO_Port GPIOE
#define mb_lf_Pin GPIO_PIN_8
#define mb_lf_GPIO_Port GPIOE
#define mb_rb_Pin GPIO_PIN_10
#define mb_rb_GPIO_Port GPIOE
#define mf_rb_Pin GPIO_PIN_12
#define mf_rb_GPIO_Port GPIOE
#define mb_lb_Pin GPIO_PIN_14
#define mb_lb_GPIO_Port GPIOE
#define CLK_IN_Pin GPIO_PIN_10
#define CLK_IN_GPIO_Port GPIOB
#define NRF_IRQ_Pin GPIO_PIN_14
#define NRF_IRQ_GPIO_Port GPIOB
#define NRF_IRQ_EXTI_IRQn EXTI15_10_IRQn
#define NRF_CSN_Pin GPIO_PIN_8
#define NRF_CSN_GPIO_Port GPIOD
#define NRF_CE_Pin GPIO_PIN_9
#define NRF_CE_GPIO_Port GPIOD
#define LD4_Pin GPIO_PIN_12
#define LD4_GPIO_Port GPIOD
#define LD3_Pin GPIO_PIN_13
#define LD3_GPIO_Port GPIOD
#define LD5_Pin GPIO_PIN_14
#define LD5_GPIO_Port GPIOD
#define LD6_Pin GPIO_PIN_15
#define LD6_GPIO_Port GPIOD
#define I2S3_MCK_Pin GPIO_PIN_7
#define I2S3_MCK_GPIO_Port GPIOC
#define VBUS_FS_Pin GPIO_PIN_9
#define VBUS_FS_GPIO_Port GPIOA
#define OTG_FS_DM_Pin GPIO_PIN_11
#define OTG_FS_DM_GPIO_Port GPIOA
#define OTG_FS_DP_Pin GPIO_PIN_12
#define OTG_FS_DP_GPIO_Port GPIOA
#define SWDIO_Pin GPIO_PIN_13
#define SWDIO_GPIO_Port GPIOA
#define SWCLK_Pin GPIO_PIN_14
#define SWCLK_GPIO_Port GPIOA
#define I2S3_SCK_Pin GPIO_PIN_10
#define I2S3_SCK_GPIO_Port GPIOC
#define I2S3_SD_Pin GPIO_PIN_12
#define I2S3_SD_GPIO_Port GPIOC
#define CE1_SPI5_Pin GPIO_PIN_0
#define CE1_SPI5_GPIO_Port GPIOD
#define CSN1_SPI5_Pin GPIO_PIN_1
#define CSN1_SPI5_GPIO_Port GPIOD
#define Audio_RST_Pin GPIO_PIN_4
#define Audio_RST_GPIO_Port GPIOD
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB
#define Audio_SCL_Pin GPIO_PIN_6
#define Audio_SCL_GPIO_Port GPIOB
#define Audio_SDA_Pin GPIO_PIN_9
#define Audio_SDA_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
