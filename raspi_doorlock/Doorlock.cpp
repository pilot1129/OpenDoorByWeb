#include "Doorlock.h"

Doorlock::Doorlock() {
	network = Network::getInstance();
	door_status = CLOSE;
}

void Doorlock::run() {

}
