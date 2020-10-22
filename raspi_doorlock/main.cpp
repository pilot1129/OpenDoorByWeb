#include "stdafx.h"
#include "Doorlock.h"

int main() {
	if (wiringPiSetupGpio() < 0) {
		printf("wiringpi setup error\n");
		return -1;
	}

	Doorlock doorlock;
	doorlock.run();
	return 0;
}