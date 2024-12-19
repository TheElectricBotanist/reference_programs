/*
 * PCA9685_driver.c
 * 
 * Copyright 2021 Lukas Fischer <lukefischer@lukefischers-PC>
 * 
 * I think this was for an Adafruit servo driver?  Not sure anymore.
 * Still, I'm keeping it.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */


#include <stdio.h>
#include <unistd.h>
#include "include/I2C.h"

#define DEV_I2C 0x40
#define PRESCALE 0x7A  // Sets PWM prescale value to 50Hz

volatile int PCA9685_fh;
volatile uint16_t lowtime;

void setHighTime(int servo, uint16_t cycles_high);

int main(int argc, char **argv)
{
	// Initialize the PCA9685
	PCA9685_fh = wiringPiI2CSetup(1, DEV_I2C);
	if(PCA9685_fh == -1)
	{
		puts("Initializing PCA9685 failed \n");
		return -1;
	}
	
	// Set PWM to 50Hz
	int i = 0;
	i |= wiringPiI2CWriteReg8(PCA9685_fh, 0x00, 00110000);  // Puts in sleep mode
	usleep(100000);
	i |= wiringPiI2CWriteReg8(PCA9685_fh, 0xFE, (uint8_t) PRESCALE);  // Writes prescale value
	usleep(100000);
	i |= wiringPiI2CWriteReg8(PCA9685_fh, 0x01, 00001100);  // Sets chip to update duty cycle at end of an I2C write as appose to a separate update command
	usleep(100000);
	i |= wiringPiI2CWriteReg8(PCA9685_fh, 0x00, 00100000);  // Wakes up device
	usleep(100000);
	if(i != 0)
	{
		puts("Something went wrong during configuration \n");
		return -1;
	}
	
	// Example:  set to 50% duty cycle
	setHighTime(0, 4096/3);
	puts("Theoretically PWM has been set \n");
	
	return 0;
}

void setHighTime(int servo, uint16_t cycles_high)
{
	if(cycles_high > 4096) cycles_high = 4096;
	lowtime = 4096 - cycles_high;
	// wiringPiI2CWriteReg16(PCA9685_fh, 0x06 + (servo * 4), (cycles_high >> 8) | (cycles_high << 8));
	// wiringPiI2CWriteReg16(PCA9685_fh, 0x08 + (servo * 4), (lowtime >> 8) | (lowtime << 8));
	wiringPiI2CWriteReg8(PCA9685_fh, 0x06 + (servo * 4), (uint8_t) (cycles_high >> 8));
	wiringPiI2CWriteReg8(PCA9685_fh, 0x07 + (servo * 4), (uint8_t) (cycles_high & 0000000011111111));
	wiringPiI2CWriteReg8(PCA9685_fh, 0x08 + (servo * 4), (uint8_t) (lowtime >> 8));
	wiringPiI2CWriteReg8(PCA9685_fh, 0x09 + (servo * 4), (uint8_t) (lowtime & 0000000011111111));
}

