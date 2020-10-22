#pragma once
#include "stdafx.h"
#include "Network.h"
#include "SE/SArduino.h"

enum DoorStat {CLOSE, OPEN};
class Doorlock {
public:
	Doorlock();
	void run();

private:
	void doorControl(int stat);

	Network* network;
	int door_status;

};

