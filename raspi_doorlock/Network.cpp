#include "Network.h"

using namespace std;

Network* Network::Instance = nullptr;
SSL_CTX* Network::ctx = nullptr;
SSL* Network::ssl = nullptr;

pthread_mutex_t Network::recv_mutex, Network::send_mutex;
sem_t Network::recv_num, Network::send_num;
std::queue<NetData> Network::recv_buf, Network::send_buf;

bool Network::sendMsg(char* data, int len) {
	if (send_buf.size() > 100)
		return false;

	NetData msg;
	msg.data = data;
	msg.len = len;
	pthread_mutex_lock(&send_mutex);
	send_buf.push(msg);
	sem_post(&send_num);
	pthread_mutex_unlock(&send_mutex);

	return true;
}

bool Network::recvMsg(char* buf, int* bufLen) {
	if(sem_trywait(&recv_num)) {
		if (*bufLen < recv_buf.front().len) {
			pthread_mutex_lock(&recv_mutex);
			*bufLen = recv_buf.front().len;
			sem_post(&recv_num);
			pthread_mutex_unlock(&recv_mutex);
			return false;
		}

		pthread_mutex_lock(&recv_mutex);
		*bufLen = recv_buf.front().len;
		memcpy(buf, recv_buf.front().data, *bufLen);

		delete recv_buf.front().data;
		recv_buf.pop();
		pthread_mutex_unlock(&recv_mutex);
		return true;
	}
	return false;
}

Network::Network() {
	networkInit();
	do {
		networkConnect();
	} while (connectCheck());


	pthread_create(&this->network_thread[0], NULL, sockRecv_t, NULL);
	pthread_create(&this->network_thread[0], NULL, sockSend_t, NULL);
}

Network::~Network() {
	SSL_shutdown(ssl);

	close(sockfd);
	SSL_free(ssl);
	SSL_CTX_free(ctx);
	for (int i = 0; i < 2; i++) 
		pthread_join(this->network_thread[i], NULL);
}

Network* Network::getInstance() {
	if (Instance == nullptr) {
		pthread_mutex_init(&recv_mutex, NULL);
		pthread_mutex_init(&send_mutex, NULL);
		sem_init(&recv_num, 0, 0);
		sem_init(&send_num, 0, 0);

		Instance = new Network();
	}
	return Instance;
}

void Network::delInstance() {
	if (Instance != nullptr) {
		pthread_mutex_destroy(&recv_mutex);
		pthread_mutex_destroy(&send_mutex);
		sem_destroy(&recv_num);
		sem_destroy(&send_num);

		delete Instance;
		Instance = nullptr;
	}
}

bool Network::connectCheck() {
	int error = 0;
	socklen_t len = sizeof(error);
	int retval = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len);
	if (retval != 0)
		return false;
	return true;
}

void Network::networkInit() {
	unsigned int addr;

	const SSL_METHOD* meth;

	if ((errBIO = BIO_new(BIO_s_file())) != NULL)
		BIO_set_fp(errBIO, stderr, BIO_NOCLOSE | BIO_FP_TEXT);

	meth = SSLv23_method();
	ctx = SSL_CTX_new(meth);

	if (ctx == NULL) {
		cout << "SSL_CTX generation error" << endl;
		ERR_print_errors(errBIO);
	}

	if (isalpha(SERVER_ADDRESS[0]))
		host = gethostbyname(SERVER_ADDRESS);
	else {
		addr = inet_addr(SERVER_ADDRESS);
		host = new hostent;
		host->h_addr_list = new char* [1];
		host->h_addr = (char*)&addr;
		host->h_length = sizeof(addr);
		host->h_addrtype = AF_INET;
	}

	if (host == NULL) {
		fprintf(stderr, "알 수 없는 주소 [%s] 입니다! \n", SERVER_ADDRESS);
		exit(1);
	}
}

void Network::networkConnect() {

	struct sockaddr_in server_add;

	int socket_type = SOCK_STREAM;

	memset(&server_add, 0, sizeof(server_add));
	memcpy(&(server_add.sin_addr), host->h_addr, host->h_length);
	server_add.sin_family = host->h_addrtype;
	server_add.sin_port = htons(PORT);

	sockfd = socket(AF_INET, socket_type, 0);
	if (sockfd < 0) {
		fprintf(stderr, "socket generation error!\n");
	}

	printf("[%s] connecting to server…\n", SERVER_ADDRESS);
	if (connect(sockfd, (struct sockaddr*) & server_add, sizeof(server_add)) == -1) {
		fprintf(stderr, "connect error!\n");
	}

	ssl = SSL_new(ctx);
	SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
	if (ssl == NULL) {
		BIO_printf(errBIO, "SSL generation error!\n");
		ERR_print_errors(errBIO);
	}

	SSL_set_fd(ssl, sockfd);
	if (SSL_connect(ssl) == -1) {
		BIO_printf(errBIO, "SSL connect error!\n");
		ERR_print_errors(errBIO);
	}
}

void* Network::sockSend_t(void*) {
	while (1) {
		sem_wait(&send_num);
		pthread_mutex_lock(&send_mutex);
		if (SSL_write(ssl, send_buf.front().data, send_buf.front().len) < 0)
			sem_post(&send_num);
		else
			send_buf.pop();
		pthread_mutex_unlock(&send_mutex);
	}
}

void* Network::sockRecv_t(void*) {
	while (1) {
		NetData msg;
		SSL_read(ssl, &msg.len, sizeof(msg.len));
		msg.data = new char[msg.len];
		SSL_read(ssl, msg.data, msg.len);

		pthread_mutex_lock(&recv_mutex);
		recv_buf.push(msg);
		sem_post(&recv_num);
		pthread_mutex_unlock(&recv_mutex);
	}
}