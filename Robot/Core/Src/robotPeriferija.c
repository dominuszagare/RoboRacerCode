/*
 * robotPeriferija.c
 *
 *  Created on: Jan 5, 2022
 *      Author: ddomi
 */
#include "robotPeriferija.h"

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



int16_t izracunajPovprecjeInt16(struct tekocePovprecjeInt16* data,int16_t nov, uint8_t cleni){
	data->sum =  data->sum + nov - data->vals[data->index]; //pristejemo trenutno vrednost in odstejemo zadnjo
	data->vals[data->index] = nov; //na zadnjo zamenjamo z novo
	data->index++;
	if(cleni > STEVILO_CLENOV_TP){cleni = STEVILO_CLENOV_TP;}
	if(data->index >= cleni){data->index = 0;}
	data->avrage =  data->sum/cleni;//izracunamo povprecje
	return data->avrage;
}

void nastaviMotor(enum motor m,int16_t pwm){
	int8_t foward = 1;
	if(pwm < 0){
		pwm = (~(pwm)+1); foward=0;
	}
	if(pwm > 999){pwm = 999;}
	if(pwm == 0){foward = -1;}
	switch(m){
	case LF:
		//TIM3->CCR4 = izracunajPovprecjeInt16(&M1,pwm,MOTORJI_IZHOD_TP_CLENI);
		TIM3->CCR4 = pwm;
		if(foward == 1){GPIOE->ODR |= (1<<10); GPIOE->ODR &= ~(1<<12);}
		else if(foward < 0){GPIOE->ODR &= ~(1<<10); GPIOE->ODR &= ~(1<<12);}
		else{GPIOE->ODR &= ~(1<<10); GPIOE->ODR |= (1<<12);}
		break;
	case RB:
		//TIM2->CCR4 = izracunajPovprecjeInt16(&M2,pwm,MOTORJI_IZHOD_TP_CLENI);
		TIM2->CCR4 = pwm;
		if(foward == 1){GPIOE->ODR |= (1<<14); GPIOE->ODR &= ~(1<<13);}
		else if(foward < 0){GPIOE->ODR &= ~(1<<14); GPIOE->ODR &= ~(1<<13);}
		else{GPIOE->ODR &= ~(1<<14); GPIOE->ODR |= (1<<13);}
		break;
	case RF:
		//TIM2->CCR3 = izracunajPovprecjeInt16(&M3,pwm,MOTORJI_IZHOD_TP_CLENI);
		TIM2->CCR3 = pwm;
		if(foward == 1){GPIOE->ODR |= (1<<8); GPIOE->ODR &= ~(1<<7);}
		else if(foward < 0){GPIOE->ODR &= ~(1<<8); GPIOE->ODR &= ~(1<<7);}
		else{GPIOE->ODR &= ~(1<<8); GPIOE->ODR |= (1<<7);}
		break;
	case LB:
		//TIM2->CCR2 = izracunajPovprecjeInt16(&M4,pwm,MOTORJI_IZHOD_TP_CLENI);
		TIM2->CCR2 = pwm;
		if(foward == 1){GPIOE->ODR |= (1<<9); GPIOE->ODR &= ~(1<<11);}
		else if(foward < 0){GPIOE->ODR &= ~(1<<9); GPIOE->ODR &= ~(1<<11);}
		else{GPIOE->ODR &= ~(1<<9); GPIOE->ODR |= (1<<11);}
		break;
	}
}

void ifButtonPresed(void(*f)(),struct button* b){
	  if(HAL_GPIO_ReadPin (b->port,b->pin)){
		b->presedConf++;
  	    b->relesedConf = 0;
  	    if (b->presedConf > b->debaunceCycles){
  	    	if(~b->flags & 1){ //ce je zastavica na prvem bitu 0 jo nastavi na 1 in zastavico na drugem bitu na 0
  	    		b->flags |= 1; b->flags &= ~(1<<1);
  	    		 //on press funkcija
  	    	}
  	    }
  	}
  	else{
  		b->presedConf = 0;
  	    b->relesedConf++;
  	    if (b->relesedConf > b->debaunceCycles){
  	    	if(~b->flags & (1<<1)){ //ce je zastavica na drugem bitu 0 jo nastavi na 1 in zastavico na prvem bitu na 0
  	    		b->flags |= (1<<1); b->flags &= ~1;
  	    		(*f)();
  	    		b->presses++;
  	    	}
  	    }
  	}
}

