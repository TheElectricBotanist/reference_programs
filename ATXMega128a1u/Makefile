main:  main.c
	rm -f *.bin *.hex
	avr-gcc -mmcu=atxmega64a1u -Ofast -g -Wall -o main.bin main.c
	avr-objcopy -j .text -j .data -O ihex main.bin main.hex
