/*
 * polyphase_pwm.c
 * 
 * Copyright 2025 Luke Fischer <lukefischer@nanopineoplus2>
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
 ***********************************************************************
 *
 * This program is designed to generate a software-controlled, polyphase
 * PWM signal through multiple GPIO pins.  Since it is a soft PWM some
 * noise, jitter, and frequency instability should be expected.  Nonetheless,
 * the current parameters were found to be quite adequate at producing a
 * three-phase sine wave at 300 Hz with proper filtering (and with an
 * Allwinner H5 no less!).  For porting the program as-is to a different
 * SBC, line_offsets will need to reflect the used GPIO, and chip_path
 * will need to point to the proper path used by gpiod.
 * 
 * While many ways exist to optimize a software polyphase PWM GPIO
 * generator, this one was built to utilize a single thread.  This may allow
 * such a method to be ported to embedded or single-core processors in
 * the future.
 * 
 */


#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <gpiod.h>
#include <math.h>
#include <stdbool.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>


// PWM_FREQ is really what sets the sleep function.  So, a 25KC setting
// runs the sleep function at 40,000ns.  With PHASE_RESOLUTION the true
// PWM frequency is dynamically assigned, and with TEMPORAL_RESOLUTION
// the true PWM frequency is PWM_FREQ/TEMPORAL_RESOLUTION.
#define PWM_FREQ 30000 // Stability decreases when above about 25Kc
#define TEMPORAL_RESOLUTION 4  // Granularity of PWM cycle - 4 was found to be good

// Define number and output of GPIO here
#define NUM_LINES 3  // Number of PWM channels
static const unsigned int line_offsets[NUM_LINES] = {203, 11, 12};  // Linux GPIO numbers
static const char * const chip_path = "/dev/gpiochip1";  // GPIO chip

// Starting output target frequency.  Actual output frequency may be slightly
// different depending on the values of PWM_FREQ and TEMPORAL_RESOLUTION
volatile float drvFreq = 300.0f; 

/*
 ***********************************************************************
*/

// Other global data objects
struct timespec time1;  // Used by absSleep()
long sleeptime = (long)round(1000000000.0 / (double)PWM_FREQ);  // Sleep duration defined from PWM_FREQ
struct gpiod_line_request * request;  // Used to configure GPIO

// Descriptions of functions are found at the definitions, but many are
// self-explainitory
int absSleep(long nanoseconds);
void generateTruthTable(float inputFunctionTable[360], bool outputTruthTable[][TEMPORAL_RESOLUTION], int phaseResolution);
int getPhaseResolution(float targetFrequency);
float getTrueFreq(int phaseResolution);
int evaluateLine(uint_fast16_t divNum, uint_fast16_t sweepNum, bool truthTable[][TEMPORAL_RESOLUTION], uint_fast16_t offsetNo, volatile bool isHigh[]);
static struct gpiod_line_request * request_output_line(const char *chip_path, const unsigned int offsets[], int numOffsets, enum gpiod_line_value value, const char *consumer);


int main(int argc, char **argv)
{
	// Open GPIO(s)
	enum gpiod_line_value value = GPIOD_LINE_VALUE_ACTIVE;
	request = request_output_line(chip_path, line_offsets, NUM_LINES, value, "toggle-line-value");
	if(!request)
	{
		fprintf(stderr, "failed to request line: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}
	
	// Create some sine lookup tables.  It is likely any function can be
	// used that repeats between 0 and 360 degrees
	float sinLookup[360];
	float sin120Lookup[360];
	float sin240Lookup[360];
	int i;
	for(i = 0; i < 360; ++i)
	{
		sinLookup[i] = (float)(sin((double)i * (3.14159f/180.0f)) * 0.5f) + 0.5f;
		sin120Lookup[i] = (float)(sin(((double)i + 120.0f) * (3.14159f/180.0f)) * 0.5f) + 0.5f;
		sin240Lookup[i] = (float)(sin(((double)i + 240.0f) * (3.14159f/180.0f)) * 0.5f) + 0.5f;
	}
	
	// Define your phase truth tables here.  Each table need not utilize
	// the same frequency, but with the current setup a different phase
	// resolution will need to be calculated for each, and each would require
	// its own divNum in the PWM loop.
	uint_fast16_t phaseRes = getPhaseResolution(drvFreq);
	bool phase1TruthTable[phaseRes][TEMPORAL_RESOLUTION];
	bool phase2TruthTable[phaseRes][TEMPORAL_RESOLUTION];
	bool phase3TruthTable[phaseRes][TEMPORAL_RESOLUTION];
	generateTruthTable(sinLookup, phase1TruthTable, phaseRes);
	generateTruthTable(sin120Lookup, phase2TruthTable, phaseRes);
	generateTruthTable(sin240Lookup, phase3TruthTable, phaseRes);
	printf("Target Frequency:  %f Hz;  True Frequency:  %f Hz \n", drvFreq, getTrueFreq(phaseRes));
	
	// The PWM Loop
	uint_fast16_t divNum = 0;
	uint_fast16_t sweepNum = 0;
	uint_fast16_t tempRes = TEMPORAL_RESOLUTION;
	bool isHigh[NUM_LINES] = {false};
	clock_gettime(CLOCK_REALTIME, &time1);  // Needed to start the sleep function
	for(;;)
	{
		// Evaluate the lines here
		evaluateLine(divNum, sweepNum, phase1TruthTable, 0, isHigh);
		evaluateLine(divNum, sweepNum, phase2TruthTable, 1, isHigh);
		evaluateLine(divNum, sweepNum, phase3TruthTable, 2, isHigh);
		++sweepNum;
		if(sweepNum == tempRes)
		{
			sweepNum = 0;
			++divNum;
			if(divNum == phaseRes)
			{
				divNum = 0;
			}
		}
		
		absSleep(sleeptime);
	}
	
	gpiod_line_request_release(request);
	return 0;
}


// Nanoseconds of sleep since last time function was called or time1 was
// updated.  Stabalizes frequency a bit.
int absSleep(long nanoseconds)
{
	long newTime = time1.tv_nsec + nanoseconds;
	if (newTime > 999999999)
	{
		++time1.tv_sec;
		time1.tv_nsec = newTime - 1000000000;
	}
	else
	{
		time1.tv_nsec = newTime;
	}
	return clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &time1, NULL);
}


// Gets the phase resolution for a target frequency
int getPhaseResolution(float targetFrequency)
{
	float sweepsPerCycle = (float)((double)PWM_FREQ/(double)targetFrequency);
	return (int)round(sweepsPerCycle / (float)TEMPORAL_RESOLUTION);  //Divs per output cycle
}


// Helpful tool to get the true frequency from the phase resolution
float getTrueFreq(int phaseResolution)
{
	return (float)PWM_FREQ / (phaseResolution * (float)TEMPORAL_RESOLUTION);
}


// Generates truth table for PWM timing for a single phase over 360 degrees.
// While the second parameter for the output table is generally
// TEMPORAL_RESOLUTION, getPhaseResolution() should be run to get the first parameter.
void generateTruthTable(float inputFunctionTable[360], bool outputTruthTable[][TEMPORAL_RESOLUTION], int phaseResolution)
{
	int i;
	// Create angle lookup table
	int angleLookup[phaseResolution];
	for(i = 0; i < phaseResolution; ++i)
	{
		angleLookup[i] = (int)round((float)i * (360.0f / (float)phaseResolution));
	}
	
	// Create sweep duration table
	int highSweepsTbl[phaseResolution];
	for(i = 0; i < phaseResolution; ++i)
	{
		highSweepsTbl[i] = (int)round(inputFunctionTable[angleLookup[i]] * (float)TEMPORAL_RESOLUTION);
	}
	
	for(i = 0; i < phaseResolution; ++i)
	{
		int j;
		for(j = 0; j < TEMPORAL_RESOLUTION; ++j)
		{
			if(j < highSweepsTbl[i])
			{
				outputTruthTable[i][j] = true;
			}
			else
			{
				outputTruthTable[i][j] = false;
			}
		}
	}
}

// Determines if the line should be set high or low for a given sweep.
// Returns -1 on failure, 0 on successfull line write, and 1 on no action.
int evaluateLine(uint_fast16_t divNum, uint_fast16_t sweepNum, bool truthTable[][TEMPORAL_RESOLUTION], uint_fast16_t offsetNo, volatile bool isHigh[])
{
	int ret = 1;
	if(truthTable[divNum][sweepNum])
	{
		if(!isHigh[offsetNo])
		{
			ret = gpiod_line_request_set_value(request, line_offsets[offsetNo], GPIOD_LINE_VALUE_ACTIVE);
			isHigh[offsetNo] = true;
			return ret;
		}
		return ret;
	}
	else
	{
		if(isHigh[offsetNo])
		{
			ret = gpiod_line_request_set_value(request, line_offsets[offsetNo], GPIOD_LINE_VALUE_INACTIVE);
			isHigh[offsetNo] = false;
			return ret;
		}
		return ret;
	}
}


// Slightly modified GPIO configuration function from the libgpiod example
// found here:  https://github.com/brgl/libgpiod/blob/master/examples/toggle_line_value.c
// Changed to allow an arbitrary number of output lines to be configured.
static struct gpiod_line_request * request_output_line(const char *chip_path, const unsigned int offsets[], int numOffsets, enum gpiod_line_value value, const char *consumer)
{
	struct gpiod_request_config *req_cfg = NULL;
	struct gpiod_line_request *request = NULL;
	struct gpiod_line_settings *settings;
	struct gpiod_line_config *line_cfg;
	struct gpiod_chip *chip;
	int ret;

	chip = gpiod_chip_open(chip_path);
	if (!chip) return NULL;

	settings = gpiod_line_settings_new();
	if (!settings) goto close_chip;

	gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
	gpiod_line_settings_set_output_value(settings, value);

	line_cfg = gpiod_line_config_new();
	if (!line_cfg) goto free_settings;

	ret = gpiod_line_config_add_line_settings(line_cfg, offsets, numOffsets, settings);
	if (ret) goto free_line_config;

	if (consumer)
	{
		req_cfg = gpiod_request_config_new();
		if (!req_cfg) goto free_line_config;
		gpiod_request_config_set_consumer(req_cfg, consumer);
	}

	request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);

	gpiod_request_config_free(req_cfg);

free_line_config:
	gpiod_line_config_free(line_cfg);

free_settings:
	gpiod_line_settings_free(settings);

close_chip:
	gpiod_chip_close(chip);

	return request;
}
