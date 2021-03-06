/*
 * robotTasks.c
 *
 *  Created on: Jan 5, 2022
 *      Author: ddomi
 */
#include "robotTasks.h"
#include "robotPeriferija.h"
#include "nrf24.h"
#include "math.h"
#include "MadgwickAHRS.h"
#include "usbd_cdc_if.h"
#include "stdint.h"

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

extern nRF24_RXResult pipe;
extern volatile uint8_t nRF24_dataReady;
extern volatile uint8_t nRF24_status;
extern volatile int payload_length;

extern volatile uint8_t GyroReady;
extern volatile uint8_t AccReady;
extern volatile uint8_t MagReady;
extern volatile uint8_t sendData;

extern QueueHandle_t PozDataQueueHandle;


extern volatile uint8_t SPIcommandRecived;

extern volatile uint8_t SpiTxData[SPI_BUFFER_SIZE];
extern volatile uint8_t SpiRxData[SPI_BUFFER_SIZE];

extern volatile uint32_t timeSinceLastCommand;



float normalize_v3f(float* x, float* y, float* z){
	float norm = sqrt( (*x) * (*x) + (*y) * (*y) + (*z) * (*z) );
	*x /= norm;
	*y /= norm;
	*z /= norm;
}

int16_t zgladiMotor(enum motor m, int16_t pwm){
	switch(m){
	case LF:
		return izracunajPovprecjeInt16(&M4,pwm,5);
	case RB:
		return izracunajPovprecjeInt16(&M2,pwm,5);
	case RF:
		return izracunajPovprecjeInt16(&M1,pwm,5);
	case LB:
		return izracunajPovprecjeInt16(&M3,pwm,5);
	}
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

void StartTransmitingData(void *argument){ //opravilo z nizko prioriteto

	while(1){
		if(xQueueReceive(PozDataQueueHandle, &P, portMAX_DELAY) == pdPASS){
			P.head = 0xAAAB;
			CDC_Transmit_FS((uint8_t*)&P,sizeof(P));
		}
		vTaskDelay(2);
	}
}

void StartCalculatingPoz(void *argument){
	uint32_t notification = 0;
	while(1){
		if(xTaskNotifyWait(0,0,&notification,pdMS_TO_TICKS(10)) == pdTRUE){
			if(MagReady){ //zamenjaj mag redy zastavice z task notification
				MagF.x = izracunajPovprecjeInt16(&MagX,Mag.x,10);
				MagF.y = izracunajPovprecjeInt16(&MagY,Mag.y,10);
				MagF.z = izracunajPovprecjeInt16(&MagZ,Mag.z,10);
				MagReady = 0;

				//vstavi podatke v queue za posiljanje 100 na sekundo
				xQueueSend(PozDataQueueHandle, &P, portMAX_DELAY);

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
					ax = ((float)AccF.x) *0.0006103515f;
					ay = ((float)AccF.y) *0.0006103515f;
					az = ((float)AccF.z) *0.0006103515f;//+-2g  2/(2^16/2)
					normalize_v3f(&ax,&ay,&az);
				}

				gx = ((float)GyroF.x) * 0.0175f * DEG_TO_RAD*2; //deg/s obcutljivost 500dps
				gy = ((float)GyroF.y) * -0.0175f * DEG_TO_RAD*2;
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

				P.magX = ax;
				P.magY = ay;
				P.magZ = az;

				for(int n=4; n<30; n++){ //pripravi podatke za spi
					SpiTxData[n-4] = ((uint8_t*)&P)[n];
				}
			}

		}
	    vTaskDelay(1);
	}
}
void StartRecivingCommandsNRF24(void *argument){
	MX_USB_DEVICE_Init();
	while(1){
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
		vTaskDelay(50);
	}
}
void StartMotorControl(void *argument){
	while(1){
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
	  vTaskDelay(10);
	}
}

volatile int32_t motorRFprevPoz=0;
volatile int32_t motorLFprevPoz=0;
volatile int32_t motorRBprevPoz=0;
volatile int32_t motorLBprevPoz=0;

void StartCalculatingPath(void *argument){
	while(1){
		float pot = (float)(motorRF.poz - motorRFprevPoz);
		pot *= PI*0.003f;
		motorRFprevPoz = motorRF.poz;
		P.pozX += sin(P.heading) * pot;
		P.pozY += cos(P.heading) * pot;
		vTaskDelay(600);
	}
}
