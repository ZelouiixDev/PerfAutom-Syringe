#include <stdio.h>
#include <stdlib.h>

#include <wiringPi.h>

int main(){
	int pin = 0;
	if (wiringPiSetup() == -1) return 0;
	while (1){
		system("clear");
		printf("\nQuel GPIO allumer ?\n");
		scanf("%d", &pin);
		printf("Allumage du pin: %d\n", pin);
		pinMode(pin, OUTPUT);
		digitalWrite(pin, 1);
		delay(2000);
		printf("Eteignage du pin %d\n", pin);
		digitalWrite(pin, 0);
		delay(500);
	}

	return 0;

}
