/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "math.h"
#include "MadgwickAHRS.h"
#include "usbd_cdc_if.h"
#include "stdint.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
struct EMUdata{
	int16_t x;
	int16_t y;
	int16_t z;
} Acc, AccF, Gyro, GyroF, Mag, MagF;
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
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PI 3.14159265358979323846f
#define DEG_TO_RAD 0.01745329252f
#define SPI_BUFFER_SIZE 32

#define STEVILO_CLENOV_TP 60
struct tekocePovprecjeInt16{
	uint8_t index;
	int16_t avrage;
	int32_t sum;
	int16_t vals[STEVILO_CLENOV_TP];
} AccX,AccY,AccZ,GyroX,GyroY,GyroZ,MagX,MagY,MagZ;

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi5;
DMA_HandleTypeDef hdma_spi5_rx;
DMA_HandleTypeDef hdma_spi5_tx;

/* USER CODE BEGIN PV */
volatile uint8_t GyroReady = 0;
volatile uint8_t AccReady = 0;
volatile uint8_t MagReady = 0;
volatile uint8_t sendData = 0;

volatile float Xold = 0.0f;
volatile float Yold = 0.0f;

uint32_t rasberyReqSize = 4;
volatile uint8_t SPIcommandRecived = 0;

volatile uint8_t SpiTxData[SPI_BUFFER_SIZE];
volatile uint8_t SpiRxData[SPI_BUFFER_SIZE];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_DMA_Init(void);
static void MX_SPI5_Init(void);
/* USER CODE BEGIN PFP */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

uint8_t i2c1_pisiRegister(uint8_t naprava, uint8_t reg, uint8_t podatek) {
  naprava <<= 1;
  return HAL_I2C_Mem_Write(&hi2c1, naprava, reg, I2C_MEMADD_SIZE_8BIT, &podatek, 1, 10);
}

void i2c1_beriRegistre(uint8_t naprava, uint8_t reg, uint8_t* podatek, uint8_t dolzina) {
  if ((dolzina>1)&&(naprava==0x19))  // ce je naprava 0x19 moramo postaviti ta bit, ce zelimo brati vec zlogov
    reg |= 0x80;
  naprava <<= 1;
  HAL_I2C_Mem_Read(&hi2c1, naprava, reg, I2C_MEMADD_SIZE_8BIT, podatek, dolzina, dolzina);
}
uint8_t spi1_beriRegister(uint8_t reg) {
	uint16_t buf_out, buf_in;
	reg |= 0x80; // najpomembnejsi bit na 1
	buf_out = reg; // little endian, se postavi na pravo mesto ....
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_RESET);
	//HAL_SPI_TransmitReceive(&hspi1, (uint8_t*)&buf_out, (uint8_t*)&buf_in, 2, 2); // blocking posiljanje ....
	HAL_SPI_TransmitReceive(&hspi1, &((uint8_t*)&buf_out)[0], &((uint8_t*)&buf_in)[0], 1, 2); // razbito na dva dela, da se podaljsa cas in omogoci pravilno delovanje testa
	HAL_SPI_TransmitReceive(&hspi1, &((uint8_t*)&buf_out)[1], &((uint8_t*)&buf_in)[1], 1, 2); // razbito na dva dela, da se podaljsa cas in omogoci pravilno delovanje testa
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_SET);
	return buf_in >> 8; // little endian...
}
void spi1_pisiRegister(uint8_t reg, uint8_t vrednost) {
  uint16_t buf_out;
  buf_out = reg | (vrednost<<8); // little endian, se postavi na pravo mesto ....
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_RESET);
  //HAL_SPI_Transmit(&hspi1, (uint8_t*)&buf_out, 2, 2); // blocking posiljanje ....
  HAL_SPI_Transmit(&hspi1, &((uint8_t*)&buf_out)[0], 1, 2); // razbito na dva dela, da se podaljsa cas in omogoci pravilno delovanje testa
  HAL_SPI_Transmit(&hspi1, &((uint8_t*)&buf_out)[1], 1, 2); // razbito na dva dela, da se podaljsa cas in omogoci pravilno delovanje testa
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_SET);
}
void spi1_beriRegistre(uint8_t reg, uint8_t* buffer, uint8_t velikost) {
  reg |= 0xC0; // najpomembnejsa bita na 1
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_RESET);
  HAL_SPI_Transmit(&hspi1, &reg, 1, 10); // blocking posiljanje....
  HAL_SPI_Receive(&hspi1,  buffer, velikost, velikost); // blocking posiljanje....
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_SET);
}
void nastaviPospeskometer(){
	//register maping lsm303agr.pdf page(43)
	//0x23 0x40 BLE litle endian
	i2c1_pisiRegister(0x19, 0x21, 0x0);
	i2c1_pisiRegister(0x19, 0x24, 0x0);
	i2c1_pisiRegister(0x19, 0x25, 0x0);
	i2c1_pisiRegister(0x19, 0x26, 0x0);
	i2c1_pisiRegister(0x19, 0x20, 0x67);  //ctrl_reg1 zbudi pospeskometer in omogoci osi //0x20 0x67 200Hz enable XYZ
	//block data update HR=1 oprating mode selection 12bit data output LPen = 0 (low power mode disabled) FS0 = 1(+-4g obcutljivost)
	i2c1_pisiRegister(0x19, 0x23, 0x8);  //ctrl_reg4 nastavi posodobitev samo ko se prebere vrednost ter locljivost +-2g
	i2c1_pisiRegister(0x19, 0x22, 0x10);  //0x22 0x10 DRDY1 INT1 pin enable pe4
}
void nastaviMagnetometer(){
	i2c1_pisiRegister(0x1e, 0x60, 0xC); //cfg_reg_a_m ODR= 100HZ
	i2c1_pisiRegister(0x1e, 0x61, 0x0); //cfg_reg_b_m
	i2c1_pisiRegister(0x1e, 0x62, 0x1); //cfg_reg_c_m INT_MAG=1 INT_MAG_PIN=0 BDU=0
	i2c1_pisiRegister(0x1e, 0x63, 0xE5); //int_ctrl_reg_m enable XYZ in IEN=1 interupt generation enable IEA=1
}
void nastaviGiroskop() {
  // preverimo ali smo "poklicali" pravi senzor
  uint8_t cip = spi1_beriRegister(0x0F);
  if (cip!=0xD4 && cip!=0xD3) {
	for (;;); //napaka ni senzorja
  }
  spi1_pisiRegister(0x21,0);//REG2 register za high pass filter
  spi1_pisiRegister(0x22,0x8);//REG3 interupt INT1 disable DRDY/INT2 enable
    //0001 0000 FS-01 500dps
  spi1_pisiRegister(0x23,0x10);//REG4 MSB@lower addres 500deg/s 0x10 BLE = 0 BIGendian

  spi1_pisiRegister(0x25,0);//REFRENCE
  spi1_pisiRegister(0x2e,0);//FIFO_CTRL
  spi1_pisiRegister(0x32,0);//INT1_THS
  spi1_pisiRegister(0x33,0);//INT1_THS
  spi1_pisiRegister(0x34,0);//INT1_THS
  spi1_pisiRegister(0x35,0);//INT1_THS
  spi1_pisiRegister(0x36,0);//INT1_THS
  spi1_pisiRegister(0x37,0);//INT1_THS
  spi1_pisiRegister(0x38,0);//INT1_DUR
  spi1_pisiRegister(0x30,0);//INT1_CFG

  spi1_pisiRegister(0x24,0);//REG5
  //0101 1111 : (1111)=(PD,Zen,Yen,Xen)
  spi1_pisiRegister(0x20,0x5f);//CTRL_REG1 omogoci x-1,y-1,z-1,PD-1(normal mode) DR = 01 BW = 10 cutof=25 200hz

  spi1_pisiRegister(0x20, 0x0F);//CTRL REG1 zbudi ziroskop in omogoci osi
}

float normalize_v3f(float* x, float* y, float* z){
	float norm = sqrt( (*x) * (*x) + (*y) * (*y) + (*z) * (*z) );
	*x /= norm;
	*y /= norm;
	*z /= norm;
}

int16_t izracunajPovprecjeInt16(struct tekocePovprecjeInt16* data,int16_t nov, uint8_t cleni){
	data->sum =  data->sum + nov - data->vals[data->index]; //pristejemo trenutno vrednost in odstejemo zadnjo
	data->vals[data->index] = nov; //na zadnjo zamenjamo z novo
	data->index++;
	if(cleni > STEVILO_CLENOV_TP){cleni = STEVILO_CLENOV_TP;}
	if(data->index >= cleni){data->index = 0;}
	data->avrage =  data->sum/cleni;//izracunamo povprecje
	return data->avrage;
}

void getDrift(){
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, 1);
	int32_t sumGyrox = 0;
	int32_t sumGyroy = 0;
	int32_t sumGyroz = 0;
	int32_t sumAccx = 0;
	int32_t sumAccy = 0;
	int32_t sumAccz = 0;
	for(int i=0; i < 400; i++){
		sumGyrox += Gyro.x;
		sumGyroy += Gyro.y;
		sumGyroz += Gyro.z;
		sumAccx += Acc.x;
		sumAccy += Acc.y;
		sumAccz += Acc.z;
		HAL_Delay(6);
	}
	E.Accx = sumAccx/400; //vektor gravitacije
	E.Accy = sumAccy/400;
	E.Accz = sumAccz/400;
	E.Gyrox = sumGyrox/400;
	E.Gyroy = sumGyroy/400;
	E.Gyroz = sumGyroz/400;

	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, 0);

	P.pozX = 0;
	P.pozY = 0;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	//kateri pin je poklical EXIT event
	if(GPIO_Pin == GPIO_PIN_1){ //vsakic ko dobis interupt posodobi podatke
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
  MX_DMA_Init();
  MX_SPI5_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  __HAL_I2C_ENABLE(&hi2c1); //omogocimo I2C1 za komunikacijo z vgrajenimi cipi
  __HAL_SPI_ENABLE(&hspi1); //komunikacija gyro

  __HAL_SPI_ENABLE(&hspi5); //rasbery pi
  HAL_Delay(50);
  nastaviPospeskometer();
  nastaviMagnetometer();
  nastaviGiroskop();
  //prvic preberi podatke da se generirajo interupti
  spi1_beriRegistre(0x28, (uint8_t*)&Gyro, 6);
  i2c1_beriRegistre(0x19, 0x28,(uint8_t*)&Acc, 6);
  i2c1_beriRegistre(0x1e, 0x68,(uint8_t*)&Mag, 6);

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

  getDrift();

  while (1)
  {
    /* USER CODE END WHILE */
	  if(MagReady){
		  MagF.x = izracunajPovprecjeInt16(&MagX,Mag.x,10);
		  MagF.y = izracunajPovprecjeInt16(&MagY,Mag.y,10);
		  MagF.z = izracunajPovprecjeInt16(&MagZ,Mag.z,10);
		  MagReady = 0;

		  //posiljanje podatkov

		  //HAL_SPI_TransmitReceive_DMA(&hspi5, SpiTxData, SpiRxData, 32);
		  //HAL_SPI_Transmit_DMA(&hspi5, SpiTxData, 32);
		  //HAL_SPI_Receive_DMA(&hspi5, SpiRxData, 32);

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
			  az = ((float)AccF.z) *0.0006103515f; //+-2g  2/(2^16/2)
			  normalize_v3f(&ax,&ay,&az);
		  }

		  gx = ((float)GyroF.x) * 0.0175f * DEG_TO_RAD*2; //deg/s obcutljivost 500dps
		  gy = ((float)GyroF.y) * 0.0175f * DEG_TO_RAD*2;
		  gz = ((float)GyroF.z) * 0.0175f * DEG_TO_RAD*2;

		  if(MagF.x == 0 && MagF.y == 0 && MagF.z==0){mx = 0.2f; my = 0.2f; mz = 0.1f;}
		  else{
			  mx = ((float)MagF.x);// * 0.0015f; //magnetic sesnetivity 1.5 mgauss/LSB
			  my = ((float)MagF.y);// * -0.0015f;
			  mz = ((float)MagF.z);// * 0.0015f;
		  }
		  normalize_v3f(&mx,&my,&mz);

		  //MadgwickAHRSupdate(gx,gy,gz,ax,ay,az,mx,my,mz);
		  MadgwickAHRSupdate(gx,gy,gz,ax,ay,az,0,0,0);
		  P.heading = atan2(2*(q0*q3+q1*q2),1-2*(q2*q2+q3*q3));//*(180/PI);
		  P.roll = atan2(2*(q0*q1+q2*q3),1-2*(q1*q1+q2*q2));//*(180/PI);
		  P.pitch = asin(2*(q0*q2 - q3*q1));//*(180/PI);
		  P.Q0 = q0; P.Q1 = q1; P.Q2 = q2; P.Q3 = q3;


		  //rotiraj po X za roll
		  ay = ay*cos(P.roll)-az*sin(P.roll);
		  az = ay*sin(P.roll)+az*cos(P.roll);
		  //rotiraj vektor okoli Y za pitch
		  ax = ax*cos(-P.pitch)-az*sin(-P.pitch);
		  az = -ax*sin(-P.pitch)+az*cos(-P.pitch);

		  //pretvorimo v globalni kordinatni sistem

		  if(ax > 0.0005 || ay > 0.0005){
			  P.pozX = Xold + 0.5*ax*0.025;
			  P.pozY = Yold + 0.5*ay*0.025;
			  Xold = P.pozX;
			  Yold = P.pozY;
		  }
		  P.magX = ax;
		  P.magY = ay;
		  P.magZ = az;


		  for(int n=4; n<30; n++){ //pripravi podatke za spi
			  SpiTxData[n-4] = ((uint8_t*)&P)[n];
		  }
	  }
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
  * @brief SPI5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI5_Init(void)
{

  /* USER CODE BEGIN SPI5_Init 0 */

  /* USER CODE END SPI5_Init 0 */

  /* USER CODE BEGIN SPI5_Init 1 */

  /* USER CODE END SPI5_Init 1 */
  /* SPI5 parameter configuration*/
  hspi5.Instance = SPI5;
  hspi5.Init.Mode = SPI_MODE_SLAVE;
  hspi5.Init.Direction = SPI_DIRECTION_2LINES;
  hspi5.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi5.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi5.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi5.Init.NSS = SPI_NSS_SOFT;
  hspi5.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi5.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi5.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi5.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi5) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI5_Init 2 */

  /* USER CODE END SPI5_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);
  /* DMA2_Stream4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream4_IRQn);

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
  HAL_GPIO_WritePin(GPIOD, LD4_Pin|LD3_Pin|LD5_Pin|LD6_Pin
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

  /*Configure GPIO pins : LD4_Pin LD3_Pin LD5_Pin LD6_Pin
                           Audio_RST_Pin */
  GPIO_InitStruct.Pin = LD4_Pin|LD3_Pin|LD5_Pin|LD6_Pin
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

  /*Configure GPIO pin : PC11 */
  GPIO_InitStruct.Pin = GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

  HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
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
