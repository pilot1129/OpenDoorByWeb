#pragma once
#include "stdafx.h"

#define SERVER_ADDRESS "163.180.118.201"
#define PORT 32765

typedef struct NetData {
	char* data;
	int len;
} NetData;

class Network {
public:
	static Network* getInstance();
	bool sendMsg(char* data, int len);
	bool recvMsg(char* buf, int* bufLen);

private:
	Network();
	~Network();
	static Network* Instance;
	static void delInstance();

	bool connectCheck();
	void networkInit();
	void networkConnect();
	
	static void* sockSend_t(void*);
	static void* sockRecv_t(void*);
private:
	pthread_t network_thread[2];
	static pthread_mutex_t recv_mutex, send_mutex;
	static sem_t recv_num, send_num;
	static std::queue<NetData> recv_buf, send_buf;

	int sockfd;
	struct hostent* host;
	BIO* errBIO;
	static SSL_CTX* ctx;
	static SSL* ssl;
};

