/*
 * robotPeriferija.h
 *
 *  Created on: Jan 5, 2022
 *      Author: ddomi
 */

#ifndef INC_ROBOTPERIFERIJA_H_
#define INC_ROBOTPERIFERIJA_H_

#include "main.h"
#include "stdint.h"

#define scalePwm(p) ((int16_t)(p*1000))
#define MOTORJI_IZHOD_TP_CLENI 10


extern I2C_HandleTypeDef hi2c1;
extern SPI_HandleTypeDef hspi1;

extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;

extern volatile uint8_t AccReady;


enum motor{RF,RB,LB,LF};
enum direction{F,B};

void spi1_beriRegistre(uint8_t reg, uint8_t* buffer, uint8_t velikost);
void spi1_pisiRegister(uint8_t reg, uint8_t vrednost);
uint8_t spi1_beriRegister(uint8_t reg);
void i2c1_beriRegistre(uint8_t naprava, uint8_t reg, uint8_t* podatek, uint8_t dolzina);
uint8_t i2c1_pisiRegister(uint8_t naprava, uint8_t reg, uint8_t podatek);
int16_t izracunajPovprecjeInt16(struct tekocePovprecjeInt16* data,int16_t nov, uint8_t cleni);

void nastaviMagnetometer();
void nastaviPospeskometer();
void nastaviGiroskop();
void nastaviMotor(enum motor m,int16_t pwm);

void getDrift();
int16_t zgladiMotor(enum motor m, int16_t pwm);

void ifButtonPresed(void(*f)(),struct button* b);

#endif /* INC_ROBOTPERIFERIJA_H_ */
