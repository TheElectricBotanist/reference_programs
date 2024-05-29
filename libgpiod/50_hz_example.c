/*
 * 50_hz_example.c
 * 
 * Copyright 2021 Lukas Fischer
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
#include <gpiod.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	const char * chipname = "gpiochip0"  // Either use gpiochip0 or gpiochip1
	struct gpiod_chip * chip;
	struct gpiod_line * pwmLine;
	
	chip = gpiod_chip_open_by_name(chipname);
	pwmLine = gpiod_chip_get_line(chip, 15);
	
	gpiod_line_request_output(pwmLine, "Soft PWM", 0);
	
	// For loop should generate a square wave with a cycle of 50 Hz
	for(;;)
	{
		gpiod_line_set_value(pwmLine, 1);
		usleep(10000);
		gpiod_line_set_value(pwmLine, 0);
		usleep(10000);
	}
	
	gpiod_line_release(pwmLine);
	gpiod_chip_close(chip);
	
	return 0;
}

