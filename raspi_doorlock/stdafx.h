#pragma once
#include <iostream>
#include <wiringPi.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <queue>

#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>


#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>