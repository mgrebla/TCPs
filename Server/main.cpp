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
	stringstream ss;
	ss << PORT;
	string portNumStr = ss.str();
	char* portNumChar = (char*)portNumStr.c_str();

	WSAData wsaData;
	WORD version = MAKEWORD(2, 1);

	addrinfo* result = nullptr, hints;
	memset(&hints, 0, sizeof(hints));
	memset(&result, 0, sizeof(result));

	SOCKET serverSocket = INVALID_SOCKET;
	SOCKET clientSocket = INVALID_SOCKET;
	SOCKET clientSockets[] = { INVALID_SOCKET , INVALID_SOCKET , INVALID_SOCKET };

	WSAPOLLFD pollFds[3];

	string answer;

	char buffer[BUFFER_LEN];

	iRes = WSAStartup(version, &wsaData);
	if (iRes != 0) {
		printf("Error initializing WSA\n");
		return 1;
	}

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	iRes = getaddrinfo(NULL, portNumChar, &hints, &result);
	if (iRes != 0) {
		printf("Cos nie tak z getaddrinfo\n");
		return 1;
	}

	serverSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (serverSocket == INVALID_SOCKET) {
		printf("Error creating socket\n");
		return 1;
	}

	iRes = bind(serverSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iRes != 0) {
		printf("Bind failed\n");
		freeaddrinfo(result);
		closesocket(serverSocket);
		return 1;
	}

	freeaddrinfo(result);

	for (int i = 0; i < 4; i++) {
		pollFds[i].fd = serverSocket;
	}

	iRes = listen(serverSocket, SOMAXCONN);
	if (iRes != 0) {
		printf("Problem listening");
		closesocket(serverSocket);
		return 1;
	}

	printf("Waiting for connection...\n");

	WSAPoll(pollFds, 2, -1);

	do {
		clientSocket = accept(serverSocket, NULL, NULL);
		if (clientSocket == INVALID_SOCKET) {
			printf("Accept failed\n");
			closesocket(serverSocket);
			return 1;
		}

		printf("Connection accepted, closing serverSocket\n");

		iRes = send(clientSocket, "No siema", (int)strlen("No siema"), 0);

		memset(&buffer, 0, sizeof(buffer));
		serverService(clientSocket, buffer);

		cout << "\n\nSluchac dalej ? \n";
		cin >> answer;

	} while (!answer.compare("tak"));
	
	closesocket(serverSocket);

	iRes = shutdown(clientSocket, SD_BOTH);
	closesocket(clientSocket);
	WSACleanup();

	return 0;
}