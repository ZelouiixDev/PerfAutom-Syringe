all:
	gcc seringue.c -o seringue -lpthread -lwiringPi
	gcc gpioTest.c -o gpioTest -lwiringPi
