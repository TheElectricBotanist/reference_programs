/*
 * main.c
 * 
 * Copyright 2022 Lukas Fischer <lukefischer@lukefischers-PC>
 * 
 * This program is designed to be executed on an ATXMega128a1u.
 * It is responsible for performing 3 tasks:  recieving instuctions
 * over I2C (that give a collective thrust level, cyclic angle, and
 * cyclic thrust), monitoring motor position, and sending motor speed
 * commands to the ESC via the DShot600 protocol.
 * 
 * Of course, it was never finished; but nonetheless demonstrates the code
 * needed to set up an ATXMega128a1u in the event future projects use one.
 * 
 */

#define F_CPU 32000000UL
#define COMPARE_VAL 0x7A12  // Change this value to overclock chip
// #define I2C_ADDR 0b01010011  // One address above the DRV10983SQPWPRQ1 address
// #define COLLECTIVE_THRUST 0x01  // Collective thrust register
// #define CYCLIC_ANGLE 0x03  // Cyclic angle register
// #define CYCLIC_THRUST 0x04  // Cyclic thrust register


#define NUM_EM 24

#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
// #include <avr/wdt.h>
// #include <util/twi.h>

int main(void)
{
	// This first part sets the CPU clock to (hopefully) 32 Megacycles
	OSC.CTRL = 0b00000111;  // Enable 32MHz and 32.768kHz oscillators
	for(;;)  // Poll loop that breaks when oscillators are ready
	{
		if((OSC.STATUS | 0b11111001) == 0b11111111) break;
	}
	OSC.DFLLCTRL = 0b00000000;  // Make sure internal oscillator is set
	DFLLRC32M.COMP1 = COMPARE_VAL & 0xFF;
	DFLLRC32M.COMP2 = (COMPARE_VAL >> 8) & 0xFF;
	DFLLRC32M.CTRL = 0b00000001;  // Enable DFLL
	for(;;)  // Redundant poll loop to make sure 32MHz oscillator is working
	{
		if((OSC.STATUS | 0b11111101) == 0b11111111) break;
	}
	CCP = 0x9D;  // Unlock SPM/LPM registers
	CLK.CTRL = 0b00000001;  // Set clock to 32MHz
	OSC.CTRL = 0b00000110;  // Turn off 2MHz oscillator (MAY NOT WORK IF IMMEDATELY CALLED AFTER SETTING CLOCK)
	
	return 0;
}
