/*
 * fastest_switching.c
 * 
 * Copyright 2021 Lukas Fischer <lukefischer@lukefischers-PC>
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

#include <sys/time.h>
#include <sys/resource.h>


#include <stdio.h>
#include <gpiod.h>
#include <unistd.h>
#include <time.h>

int main(int argc, char **argv)
{
	setpriority(PRIO_PROCESS, 0, -20);
	
	struct timespec tim1;
	tim1.tv_sec = 0;
	tim1.tv_nsec = 10000000L;
	
	const char * chipname = "gpiochip1";  // Either use gpiochip0 or gpiochip1
	struct gpiod_chip * chip;
	struct gpiod_line * pwmLine;
	
	chip = gpiod_chip_open_by_name(chipname);
	pwmLine = gpiod_chip_get_line(chip, 12);  // Places PWM on SDA0 line
	
	gpiod_line_request_output(pwmLine, "Soft PWM", 0);
	
	
	// For loop should generate a square wave with a cycle of 50 Hz
	// For loop using nanosleep instead of usleep
	/* for(;;)
	{
		gpiod_line_set_value(pwmLine, 1);
		nanosleep(&tim1, NULL);
		gpiod_line_set_value(pwmLine, 0);
		nanosleep(&tim1, NULL);
	}
	*/
	
	
	// For loop for testing fastest possible switching (~458 KHz)
	for(;;)
	{
		gpiod_line_set_value(pwmLine, 1);
		gpiod_line_set_value(pwmLine, 0);
	}
	
	
	gpiod_line_release(pwmLine);
	gpiod_chip_close(chip);
	
	return 0;
}

