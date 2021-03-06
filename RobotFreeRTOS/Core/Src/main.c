/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "usb_device.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdint.h"
#include "robotPeriferija.h".h"
#include "nrf24.h"
#include "math.h"
#include "MadgwickAHRS.h"
#include "usbd_cdc_if.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin); //external interupti
//void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi);
//void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi);
//void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi);
void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c);
//void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PID_CONTROL 1
#define PID_P 0.8f
#define PID_P_LIMIT 1.0f
#define PID_I 0.02f
#define PID_I_LIMIT 50.0f
#define PID_D 0.1f
#define PID_LIMIT 1.0f
#define SPEEDMOD 0.5f

#define PI 3.14159265358979323846f
#define DEG_TO_RAD 0.01745329252f

#define I2C_BUFFER_SIZE 100
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define scalePwm(p) ((int16_t)(p*1000))

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c3;

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim5;

/* Definitions for CalculatePoz */
osThreadId_t CalculatePozHandle;
const osThreadAttr_t CalculatePoz_attributes = {
  .name = "CalculatePoz",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal7,
};
/* Definitions for ReciveCommandsN */
osThreadId_t ReciveCommandsNHandle;
const osThreadAttr_t ReciveCommandsN_attributes = {
  .name = "ReciveCommandsN",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for MotorControl */
osThreadId_t MotorControlHandle;
const osThreadAttr_t MotorControl_attributes = {
  .name = "MotorControl",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for calculatePath */
osThreadId_t calculatePathHandle;
const osThreadAttr_t calculatePath_attributes = {
  .name = "calculatePath",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal6,
};
/* USER CODE BEGIN PV */
nRF24_RXResult pipe;
volatile uint8_t nRF24_dataReady = 0;
volatile uint8_t nRF24_status = 0;
volatile int payload_length = 6;

volatile uint8_t GyroReady = 0;
volatile uint8_t AccReady = 0;
volatile uint8_t MagReady = 0;
volatile uint8_t sendData = 0;

volatile float Xold = 0.0f;
volatile float Yold = 0.0f;

volatile int32_t motorRFprevPoz=0;
volatile int32_t motorLFprevPoz=0;
volatile int32_t motorRBprevPoz=0;
volatile int32_t motorLBprevPoz=0;

volatile uint8_t I2CTxData[I2C_BUFFER_SIZE];
volatile uint8_t I2CRxData[I2C_BUFFER_SIZE];


volatile uint32_t timeSinceLastCommand = 0;
volatile uint8_t activateRasbetyPI = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM2_Init(void);
static void MX_SPI2_Init(void);
static void MX_TIM5_Init(void);
static void MX_TIM3_Init(void);
static void MX_I2C3_Init(void);
void StartCalculatingPoz(void *argument);
void StartRecivingCommandsNRF24(void *argument);
void StartMotorControl(void *argument);
void StartCalculatingPath(void *argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void nRF24SetChip(){
	nRF24_Init();

	nRF24_DisableAA(0xFF);
	nRF24_SetRFChannel(115);
	nRF24_SetDataRate(nRF24_DR_1Mbps);
	nRF24_SetCRCScheme(nRF24_CRC_off);
	nRF24_SetAddrWidth(3);

	static const uint8_t nRF24_ADDR[] = { 0xE7, 0x1C, 0xE3 };
	nRF24_SetAddr(nRF24_PIPE1, nRF24_ADDR); // program address for RX pipe #1
	nRF24_SetRXPipe(nRF24_PIPE1, nRF24_AA_OFF, payload_length); // Auto-ACK: disabled, payload length: 5 bytes
	nRF24_SetOperationalMode(nRF24_MODE_RX);
	nRF24_SetPowerMode(nRF24_PWR_UP);
	nRF24_CE_H();
}

void inicilizirajCipe(){
	__HAL_I2C_ENABLE(&hi2c1); //omogocimo I2C1 za komunikacijo z vgrajenimi cipi
	__HAL_SPI_ENABLE(&hspi1); //komunikacija gyro
	__HAL_SPI_ENABLE(&hspi2); //komunikacija z nRF24
	HAL_Delay(50);
	nRF24SetChip();
	nastaviPospeskometer();
	nastaviMagnetometer();
	nastaviGiroskop();
	//prvic preberi podatke da se generirajo interupti
	spi1_beriRegistre(0x28, (uint8_t*)&Gyro, 6);
	i2c1_beriRegistre(0x19, 0x28,(uint8_t*)&Acc, 6);
	i2c1_beriRegistre(0x1e, 0x68,(uint8_t*)&Mag, 6);
}



float normalize_v3f(float* x, float* y, float* z){
	float norm = sqrt( (*x) * (*x) + (*y) * (*y) + (*z) * (*z) );
	*x /= norm;
	*y /= norm;
	*z /= norm;
}

void speedControl(struct motorData* m, float deltaT){
	//premakni motor z zeljeno hitrostjo
	//izracunaj hitrost z tekocim povprecjem
	float vel = (float)((m->poz - m->prevPoz)/deltaT)/1000;
	m->prevPoz = m->poz;
	m->sum =  m->sum + vel - m->vals[m->index]; //pristejemo trenutno vrednost in odstejemo zadnjo
	m->vals[m->index] = vel; //na zadnjo zamenjamo z novo
	m->index++;
	if(m->index > 9){m->index = 0;}
	vel = m->sum/10; //izracunamo povprecje

	if(m->targetVel < 0.05 && m->targetVel > -0.05){
		m->targetVel = 0;
		m->integralError = 0;
	}

	if(PID_CONTROL == 1){
		m->error = (m->targetVel) - vel;
		if(m->error > PID_P_LIMIT){m->error=PID_P_LIMIT;}
		if(m->error < -PID_P_LIMIT){m->error=-PID_P_LIMIT;}

		m->integralError = m->integralError + m->error;
		if(m->integralError > PID_I_LIMIT){m->integralError = PID_I_LIMIT;}
		if(m->integralError < -PID_I_LIMIT){m->integralError = -PID_I_LIMIT;}

		float deltaE = 0.0f;
		if((m->error - m->prevError)>0.01f && (m->error - m->prevError)<-0.01f){deltaE=((m->error - m->prevError)/deltaT);}
		m->prevError = m->error;

		float power = PID_P*m->error + PID_I*m->integralError + PID_D*deltaE;
		if(power > PID_LIMIT){power = PID_LIMIT;}
		if(power < -PID_LIMIT){power = -PID_LIMIT;}
		nastaviMotor(m->num,zgladiMotor(m->num,scalePwm(power)));
	}
	else{
		int Pwm = zgladiMotor(m->num,scalePwm(m->targetVel));
		nastaviMotor(m->num,Pwm);
	}
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_TIM2_Init();
  MX_SPI2_Init();
  MX_TIM5_Init();
  MX_TIM3_Init();
  MX_I2C3_Init();
  /* USER CODE BEGIN 2 */
  //!--ce generator kode inicilizira DMA zadnje pomakni funkcijo visje da se incilizira prvo

  //iniciliziraj komunikacijo in senzorje
  HAL_Delay(100);
  inicilizirajCipe();
  nRF24_status = nRF24_Check();
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12,nRF24_status); //ce dela prizgi ledico

  B1.flags = 0;
  B1.presedConf = 0;
  B1.relesedConf = 0;
  B1.port = GPIOA;
  B1.pin = GPIO_PIN_0;
  B1.debaunceCycles = 10;
  B1.presses = 0;

  HAL_TIM_Base_Start(&htim2);
  HAL_TIM_Base_Start(&htim3);
  HAL_TIM_Base_Start(&htim5);
  // zazenemo PWM
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);

  robotPay.x1 = 0; //nastavimo na srednje vrednosti
  robotPay.x2 = 0;
  robotPay.y1 = 0;
  robotPay.y2 = 0;
  robotPay.speed = 100;

  nastaviMotor(RF,0);
  nastaviMotor(LF,0);
  nastaviMotor(LB,0);
  nastaviMotor(RB,0);


  motorLF.num = LF;
  motorLB.num = LB;
  motorRB.num = RB;
  motorRF.num = RF;

  E.Accx = 0;
  E.Accy = 0;
  E.Accz = 0;
  E.Gyrox = 0;
  E.Gyroy = 0;
  E.Gyroz = 0;
  E.HardIronMagx = 0;
  E.HardIronMagy = 0;
  E.HardIronMagz = 0;
  E.SoftIronMagx = 1;
  E.SoftIronMagy = 1;
  E.SoftIronMagz = 1;

  P.pozX = 0;
  P.pozY = 0;

  getDrift();

  __HAL_I2C_ENABLE(&hi2c3);
  HAL_Delay(100);
  activateRasbetyPI = 1;

  //HAL_SPI_Receive_DMA(&hspi5, (uint8_t*)&rasberyReq, rasberyReqSize);
  //HAL_SPI_TransmitReceive_DMA(&hspi5, SpiTxData, SpiRxData, SPI_BUFFER_SIZE);
  //HAL_SPI_TransmitReceive_DMA(&hspi5, SpiTxData, SpiRxData, 2); //beremo po dva
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of CalculatePoz */
  CalculatePozHandle = osThreadNew(StartCalculatingPoz, NULL, &CalculatePoz_attributes);

  /* creation of ReciveCommandsN */
  ReciveCommandsNHandle = osThreadNew(StartRecivingCommandsNRF24, NULL, &ReciveCommandsN_attributes);

  /* creation of MotorControl */
  MotorControlHandle = osThreadNew(StartMotorControl, NULL, &MotorControl_attributes);

  /* creation of calculatePath */
  calculatePathHandle = osThreadNew(StartCalculatingPath, NULL, &calculatePath_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2C3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C3_Init(void)
{

  /* USER CODE BEGIN I2C3_Init 0 */

  /* USER CODE END I2C3_Init 0 */

  /* USER CODE BEGIN I2C3_Init 1 */

  /* USER CODE END I2C3_Init 1 */
  hi2c3.Instance = I2C3;
  hi2c3.Init.ClockSpeed = 100000;
  hi2c3.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c3.Init.OwnAddress1 = 0;
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c3.Init.OwnAddress2 = 0;
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C3_Init 2 */

  /* USER CODE END I2C3_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 83;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 84;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief TIM5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM5_Init(void)
{

  /* USER CODE BEGIN TIM5_Init 0 */

  /* USER CODE END TIM5_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM5_Init 1 */

  /* USER CODE END TIM5_Init 1 */
  htim5.Instance = TIM5;
  htim5.Init.Prescaler = 83;
  htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim5.Init.Period = 4294967295;
  htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim5) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim5, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM5_Init 2 */

  /* USER CODE END TIM5_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, CS_SPI1_Pin|mf_lf_Pin|mb_lf_Pin|GPIO_PIN_9
                          |mb_rb_Pin|GPIO_PIN_11|mf_rb_Pin|GPIO_PIN_13
                          |mb_lb_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(OTG_FS_PowerSwitchOn_GPIO_Port, OTG_FS_PowerSwitchOn_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, NRF_CSN_Pin|NRF_CE_Pin|LD4_Pin|LD3_Pin
                          |LD5_Pin|LD6_Pin|CE1_SPI5_Pin|CSN1_SPI5_Pin
                          |Audio_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : PE2 PE4 PE1 */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_4|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : CS_SPI1_Pin */
  GPIO_InitStruct.Pin = CS_SPI1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(CS_SPI1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : OTG_FS_PowerSwitchOn_Pin */
  GPIO_InitStruct.Pin = OTG_FS_PowerSwitchOn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(OTG_FS_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PDM_OUT_Pin */
  GPIO_InitStruct.Pin = PDM_OUT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(PDM_OUT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : I2S3_WS_Pin */
  GPIO_InitStruct.Pin = I2S3_WS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
  HAL_GPIO_Init(I2S3_WS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PC5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PB2 */
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : mf_lf_Pin mb_lf_Pin PE9 mb_rb_Pin
                           PE11 mf_rb_Pin PE13 mb_lb_Pin */
  GPIO_InitStruct.Pin = mf_lf_Pin|mb_lf_Pin|GPIO_PIN_9|mb_rb_Pin
                          |GPIO_PIN_11|mf_rb_Pin|GPIO_PIN_13|mb_lb_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : CLK_IN_Pin PB12 */
  GPIO_InitStruct.Pin = CLK_IN_Pin|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : NRF_IRQ_Pin */
  GPIO_InitStruct.Pin = NRF_IRQ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(NRF_IRQ_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : NRF_CSN_Pin NRF_CE_Pin LD4_Pin LD3_Pin
                           LD5_Pin LD6_Pin CE1_SPI5_Pin CSN1_SPI5_Pin
                           Audio_RST_Pin */
  GPIO_InitStruct.Pin = NRF_CSN_Pin|NRF_CE_Pin|LD4_Pin|LD3_Pin
                          |LD5_Pin|LD6_Pin|CE1_SPI5_Pin|CSN1_SPI5_Pin
                          |Audio_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : I2S3_MCK_Pin I2S3_SCK_Pin I2S3_SD_Pin */
  GPIO_InitStruct.Pin = I2S3_MCK_Pin|I2S3_SCK_Pin|I2S3_SD_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA15 */
  GPIO_InitStruct.Pin = GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PC11 */
  GPIO_InitStruct.Pin = GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PD2 PD5 PD6 */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_5|GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : PD3 PD7 */
  GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : PB8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI2_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

  HAL_NVIC_SetPriority(EXTI3_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);

  HAL_NVIC_SetPriority(EXTI4_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	//kateri pin je poklical EXIT event
	if(GPIO_Pin == GPIO_PIN_14){
		nRF24_dataReady = 1; //spremenil se je status register pejt pogledat kaj se je zgodilo
	}
	else if(GPIO_Pin == GPIO_PIN_15){
		if(HAL_GPIO_ReadPin(GPIOD,GPIO_PIN_2)){motorLF.poz--;}
		else{motorLF.poz++;}
	}
	else if(GPIO_Pin == GPIO_PIN_7){
		if(HAL_GPIO_ReadPin(GPIOD,GPIO_PIN_5)){motorRF.poz--;}
		else{motorRF.poz++;}
	}
	else if(GPIO_Pin == GPIO_PIN_3){
		if(HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_11)){motorRB.poz--;}
		else{motorRB.poz++;}
	}
	else if(GPIO_Pin == GPIO_PIN_8){
		if(HAL_GPIO_ReadPin(GPIOD,GPIO_PIN_6)){motorLB.poz--;}
		else{motorLB.poz++;}
	}
	else if(GPIO_Pin == GPIO_PIN_1){ //vsakic ko dobis interupt posodobi podatke
		spi1_beriRegistre(0x28, (uint8_t*)&Gyro, 6);
		GyroReady = 1; //zastavica da so na voljo novi podatki 200Hz
	}
	else if(GPIO_Pin == GPIO_PIN_4){
		//data ready pospeskometer
		i2c1_beriRegistre(0x19, 0x28,(uint8_t*)&Acc, 6);
		AccReady = 1; //200Hz
	}
	else if(GPIO_Pin == GPIO_PIN_2){
		//data ready megnetometer
		i2c1_beriRegistre(0x1e, 0x68,(uint8_t*)&Mag, 6);
		MagReady = 1; //100Hz
	}
	else if(GPIO_Pin == GPIO_PIN_5 && activateRasbetyPI){
		HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_13); //oranzna
		HAL_I2C_Slave_Receive(&hi2c3, I2CRxData, 8, 100);
		HAL_I2C_Slave_Transmit(&hi2c3, I2CTxData, 48,100);
		//P.head = 0xAAAB;
		//CDC_Transmit_FS((uint8_t*)&P,(sizeof(float)*12)+4);


		//HAL_I2C_Slave_Transmit(&hi2c3, I2CTxData, I2C_BUFFER_SIZE, 100);
		//Rasbery pi zahteva podatke in posilja ukaze
		//HAL_SPI_TransmitReceive_IT(&hspi5, SpiTxData, SpiRxData, SPI_BUFFER_SIZE);
		//HAL_SPI_TransmitReceive(&hspi5, SpiTxData, SpiRxData, SPI_BUFFER_SIZE, 10);
		//HAL_SPI_Transmit(&hspi5, SpiTxData, SPI_BUFFER_SIZE, 200);

	}

}
void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c){
	HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_12);
}

//void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi){
	//HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, 1);
	//HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_13); //oranzna
	//HAL_SPI_Receive_DMA(&hspi5, (uint8_t*)&rasberyReq, rasberyReqSize);
	//..rasberyReq = *(struct recivedRasberyPiPayload*)SpiRxData;
	//if( SpiRxData[0] == 22){
	//	SPIcommandRecived = 1;
	//	//HAL_SPI_Transmit(&hspi5, (uint8_t*)&P, 28, 100);
	//}

//}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartCalculatingPoz */
/**
  * @brief  Function implementing the CalculatePoz thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartCalculatingPoz */
void StartCalculatingPoz(void *argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;){
	  if(MagReady){
		  MagF.x = izracunajPovprecjeInt16(&MagX,Mag.x,10);
		  MagF.y = izracunajPovprecjeInt16(&MagY,Mag.y,10);
		  MagF.z = izracunajPovprecjeInt16(&MagZ,Mag.z,10);
		  MagReady = 0;

		  P.head = 0xAAAB;
		  CDC_Transmit_FS((uint8_t*)&P,(sizeof(float)*12)+4);
	  }
	  if(AccReady){
		  Acc.x -= E.Accx;
		  Acc.y -= E.Accy;
		  AccF.x = izracunajPovprecjeInt16(&AccX,Acc.x,50);
		  AccF.y = izracunajPovprecjeInt16(&AccY,Acc.y,50);
		  AccF.z = izracunajPovprecjeInt16(&AccZ,Acc.z,50);
		  AccReady = 0;
	  }
	  if(GyroReady){
		  Gyro.x -= E.Gyrox;
		  Gyro.y -= E.Gyroy;
		  Gyro.z -= E.Gyroz;
		  GyroF.x = izracunajPovprecjeInt16(&GyroX,Gyro.x,50);
		  GyroF.y = izracunajPovprecjeInt16(&GyroY,Gyro.y,50);
		  GyroF.z = izracunajPovprecjeInt16(&GyroZ,Gyro.z,50);
		  GyroReady = 0;

		  //poracunamo podatke
		  float gx,gy,gz,ax,ay,az,mx,my,mz;

		  if(AccF.x == 0 && AccF.y == 0 && AccF.z==0){ax=0.0f; ay=0.0f; az=1.0f;}
		  else{
			  ax = ((float)AccF.x) *-0.0006103515f;
			  ay = ((float)AccF.y) *0.0006103515f;
			  az = ((float)AccF.z) *0.0006103515f;//+-2g  2/(2^16/2)
			  normalize_v3f(&ax,&ay,&az);
		  }

		  gx = ((float)GyroF.x) * 0.0175f * DEG_TO_RAD*2; //deg/s obcutljivost 500dps
		  gy = ((float)GyroF.y) * 0.0175f * DEG_TO_RAD*2;
		  gz = ((float)GyroF.z) * 0.0175f * DEG_TO_RAD*2;

		  if(MagF.x == 0 && MagF.y == 0 && MagF.z==0){mx = 0.2f; my = 0.2f; mz = 0.1f;}
		  else{
			  mx = ((float)MagF.x) * 0.0015f; //magnetic sesnetivity 1.5 mgauss/LSB
			  my = ((float)MagF.y) * 0.0015f;
			  mz = ((float)MagF.z) * 0.0015f;
		  }
		  normalize_v3f(&mx,&my,&mz);

		  MadgwickAHRSupdate(gx,gy,gz,ax,ay,az,0,0,0);
		  P.heading = atan2(2*(q0*q3+q1*q2),1-2*(q2*q2+q3*q3));
		  P.roll = atan2(2*(q0*q1+q2*q3),1-2*(q1*q1+q2*q2));
		  P.pitch = asin(2*(q0*q2 - q3*q1));
		  P.Q0 = q0; P.Q1 = q1; P.Q2 = q2; P.Q3 = q3;

		  //rotiraj po X za roll
		  ay = ay*cos(-P.roll)-az*sin(P.roll);
		  az = ay*sin(-P.roll)+az*cos(P.roll);
		  //rotiraj vektor okoli Y za pitch
		  ax = ax*cos(-P.pitch)-az*sin(-P.pitch);
		  az = -ax*sin(-P.pitch)+az*cos(-P.pitch);

		  //pretvorimo v globalni kordinatni sistem
		  /*
		  if(ax > 0.0005 || ay > 0.0005){
			  P.pozX = Xold + 0.5*ax*0.025;
			  P.pozY = Yold + 0.5*ay*0.025;
			  Xold = P.pozX;
			  Yold = P.pozY;
		  }
		  */


		  P.magX = ax;
		  P.magY = ay;
		  P.magZ = az;

		  for(int n=4; n<52; n++){ //pripravi podatke za spi
			  I2CTxData[n-4] = ((uint8_t*)&P)[n];
		  }

	  }

	  osDelay(1);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartRecivingCommandsNRF24 */
/**
* @brief Function implementing the ReciveCommandsN thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartRecivingCommandsNRF24 */
void StartRecivingCommandsNRF24(void *argument)
{
  /* USER CODE BEGIN StartRecivingCommandsNRF24 */
  /* Infinite loop */
  for(;;)
  {
	  if(nRF24_dataReady && nRF24_status){
		  uint8_t status = nRF24_GetStatus();
		  nRF24_ClearIRQFlags();
		  nRF24_dataReady = 0;
		  if (status != nRF24_STATUS_RXFIFO_EMPTY) {
			  //HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_15);

			  pipe = nRF24_ReadPayload((uint8_t*)(&robotPay), &payload_length);
			  nRF24_ClearIRQFlags();

			  uint8_t CRC_calc = (robotPay.x1 + robotPay.y1 + robotPay.x2 + robotPay.y2 + robotPay.speed);
			  if(robotPay.crc == CRC_calc){
				  //uporabi podatke
				  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, 1);

				  timeSinceLastCommand = HAL_GetTick();
				  float speed = SPEEDMOD*(float)robotPay.speed/255;
				  motorRF.targetVel = speed*(float)(robotPay.y1-robotPay.x1+robotPay.x2)/128;
				  motorLF.targetVel = speed*(float)(robotPay.y1+robotPay.x1-robotPay.x2)/128;
				  motorLB.targetVel = speed*(float)(robotPay.y1+robotPay.x1+robotPay.x2)/128;
				  motorRB.targetVel = speed*(float)(robotPay.y1-robotPay.x1-robotPay.x2)/128;
				  /*
				  motorRF.targetVel = speed*(float)(robotPay.y1)/128;
				  motorLF.targetVel = speed*(float)(robotPay.x1)/128;
				  motorRB.targetVel = speed*(float)(robotPay.y2)/128;
				  motorLB.targetVel = speed*(float)(robotPay.x2)/128;
				  */
			  }
		  }
	  }
	  osDelay(50);
  }
  /* USER CODE END StartRecivingCommandsNRF24 */
}

/* USER CODE BEGIN Header_StartMotorControl */
/**
* @brief Function implementing the MotorControl thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartMotorControl */
void StartMotorControl(void *argument)
{
  /* USER CODE BEGIN StartMotorControl */
  /* Infinite loop */
  for(;;)
  {
	  if((HAL_GetTick() - timeSinceLastCommand)>200){
		  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, 0);
		  nastaviMotor(RF,0); motorRF.integralError = 0; motorRF.targetVel = 0;
		  nastaviMotor(LF,0); motorLF.integralError = 0; motorLF.targetVel = 0;
		  nastaviMotor(LB,0); motorLB.integralError = 0; motorLB.targetVel = 0;
		  nastaviMotor(RB,0); motorRB.integralError = 0; motorRB.targetVel = 0;
	  }
	  speedControl(&motorLB,0.01);
	  speedControl(&motorLF,0.01);
	  speedControl(&motorRB,0.01);
	  speedControl(&motorRF,0.01);
	  osDelay(10);
  }
  /* USER CODE END StartMotorControl */
}

/* USER CODE BEGIN Header_StartCalculatingPath */
/**
* @brief Function implementing the calculatePath thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartCalculatingPath */
void StartCalculatingPath(void *argument)
{
  /* USER CODE BEGIN StartCalculatingPath */
  /* Infinite loop */
  for(;;)
  {

	  int32_t sum = (motorRF.poz - motorRFprevPoz)+(motorLF.poz - motorLFprevPoz)+(motorLB.poz - motorLBprevPoz);
	  float pot = (float)(sum)/3;
	  pot *= PI*0.003f;
	  motorRFprevPoz = motorRF.poz;
	  motorRBprevPoz = motorRB.poz;
	  motorLFprevPoz = motorLF.poz;
	  motorLBprevPoz = motorLB.poz;
	  P.pozX += cos(P.heading) * pot;
	  P.pozY -= sin(P.heading) * pot;
	  osDelay(100);
  }
  /* USER CODE END StartCalculatingPath */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM11 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM11) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, 0);
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, 1);
  nastaviMotor(RF,0); motorRF.integralError = 0;
  nastaviMotor(LF,0); motorLF.integralError = 0;
  nastaviMotor(LB,0); motorLB.integralError = 0;
  nastaviMotor(RB,0); motorRB.integralError = 0;
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
