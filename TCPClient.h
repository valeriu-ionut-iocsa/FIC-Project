#ifndef TCPCLIENT_H_
#define TCPCLIENT_H_

#include<iostream>
#include<netdb.h>

using namespace std;

class TCPClient
{
private:
	int sock;
	string address;
	int port;
	struct sockaddr_in server;

public:
	TCPClient();
	bool conn(string, int);
	bool send_data(string data);
};

#endif /* TCPCLIENT_H_ */
