#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <WinSock2.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <sstream>
#include <WS2tcpip.h>
#include <thread>

using namespace std;

#pragma comment(lib, "ws2_32.lib")

#define PORT 27047
#define BUFFER_LEN 2048
#define MAX_SOCK 3

typedef struct SOCKETS {
	SOCKET serverSocket;
	SOCKET placeHolderSocket;
} SOCKETS, *LPSOCKETS;

typedef class PORT_NO {
private:
	unsigned int no;
public:
	PORT_NO() : no(0) {};
	PORT_NO(unsigned int port) {
		no = port;
	}
	void setPortNo(unsigned int port) {
		no = port;
	}
	unsigned int getPortNo() {
		return no;
	}
	string getPortNoAsChar() {
		stringstream ss;
		ss << no;
		return ss.str();
	}
} PORT_NO, *LPPORT_NO;

void serverService(SOCKET s, char* buf) {
	int iRes;
	do {
		iRes = recv(s, buf, BUFFER_LEN, 0);
		if (iRes > 0) {
			printf("%s\n", buf);
			printf("Bytes received: %d\n\n", iRes);
			send(s, buf, BUFFER_LEN, 0);
			memset(buf, 0, sizeof(buf));
		}
	} while (iRes > 0);	
}

int main(int argc, char** argv) {

	int iRes;

	char buffer[BUFFER_LEN];

	PORT_NO port(PORT);

	WSAData wsaData;
	WORD version = MAKEWORD(2, 1);

	addrinfo* result = nullptr, hints;
	memset(&hints, 0, sizeof(hints));
	memset(&result, 0, sizeof(result));

	WSAPOLLFD pollFds[MAX_SOCK+1];

	LPSOCKETS sockets;
	sockets = (LPSOCKETS)GlobalAlloc(GPTR, sizeof(sockets));

	iRes = WSAStartup(version, &wsaData);
	if (iRes != 0) {
		printf("Error initializing WSA\n");
		return 1;
	}

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	iRes = getaddrinfo(NULL, port.getPortNoAsChar().c_str(), &hints, &result);
	if (iRes != 0) {
		printf("Cos nie tak z getaddrinfo\n");
		return 1;
	}

	sockets->serverSocket = WSASocket(result->ai_family, result->ai_socktype, result->ai_protocol,
		NULL, 0, WSA_FLAG_OVERLAPPED);
	if (sockets->serverSocket == INVALID_SOCKET) {
		printf("Error creating socket\n");
		return 1;
	}

	iRes = bind(sockets->serverSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iRes != 0) {
		printf("Bind failed\n");
		freeaddrinfo(result);
		closesocket(sockets->serverSocket);
		return 1;
	}

	freeaddrinfo(result);

	pollFds[0].fd = sockets->serverSocket;
	pollFds[0].events = POLLRDNORM;
	for (int i = 1; i < 4; i++) {
		pollFds[i].fd = INVALID_SOCKET;
		pollFds[i].events = POLLIN;
	}

	iRes = listen(sockets->serverSocket, SOMAXCONN);
	if (iRes != 0) {
		printf("Problem listening");
		closesocket(sockets->serverSocket);
		return 1;
	}

	printf("Waiting for connection...\n");

	while (1) {
		
		WSAPoll(pollFds, 4, -1);

		for (int i = 0; i < 4; i++) {
			if (i == 0 && pollFds[i].revents == POLLRDNORM) {
				sockets->placeHolderSocket = accept(sockets->serverSocket, NULL, NULL);
				if (sockets->placeHolderSocket == INVALID_SOCKET) {
					printf("Accept failed\n");
					closesocket(sockets->serverSocket);
					return 1;
				}

				printf("Connection accepted...");

				for (int i = 1; i < 4; i++) {
					if (pollFds[i].fd == INVALID_SOCKET) {
						pollFds[i].fd = sockets->placeHolderSocket;
						printf(" and passed to the new client socket\n");
						send(pollFds[i].fd, "Hi", 2, 0);
						break;
					}
				}
			}
			else if (pollFds[i].revents == POLLRDNORM) {
				memset(&buffer, 0, BUFFER_LEN);
				iRes = recv(pollFds[i].fd, buffer, BUFFER_LEN, 0);
				if (iRes > 0) {
					printf("Msg from client no. %d : %s\n",i , buffer);
				}
				else if (iRes == 0) {
					printf("Connection closed\n");
				}
				send(pollFds[i].fd, "OK", 2, 0);
			}
			else if (pollFds[i].revents == POLLHUP) {
					printf("Msg from client no. %d : Connection closed\n", i);
					pollFds[i].fd = INVALID_SOCKET;
			}
			
		}

	}
	
	closesocket(sockets->serverSocket);
	WSACleanup();

	return 0;
}