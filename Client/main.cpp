#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

using namespace std;

#pragma comment(lib, "ws2_32.lib")

#define PORT 27047
#define BUFFER_LEN 2048

void communicateToServer(SOCKET s, char* recvBuf, string sendBuf) {
	int iRes;
	iRes = send(s, sendBuf.c_str(), (int)sendBuf.size(), 0);
	if (iRes == SOCKET_ERROR) {
		printf("sending fail\n");
		closesocket(s);
		WSACleanup();
	}
}
void receiveFromServer(SOCKET s, char* recvBuf) {
	int iRes;
	
	iRes = recv(s, recvBuf, BUFFER_LEN, 0);
	printf("Recv from server: %s\n", recvBuf);
	
}

int main(int argc, char** argv) {

	stringstream ss;
	ss << PORT;
	string portCharStr = ss.str();
	char* portNumChar = (char*)portCharStr.c_str();

	string sendBuf;
	char recvBuf[BUFFER_LEN];
	memset(&recvBuf, 0, sizeof(recvBuf));

	WSAData wsaData;
	WORD version = MAKEWORD(2, 1);

	int iRes;

	addrinfo* result = nullptr, hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	SOCKET clientSocket = INVALID_SOCKET;

	iRes = WSAStartup(version, &wsaData);
	if (iRes != 0) {
		printf("Error at WSAStartup\n");
		return 1;
	}

	iRes = getaddrinfo(NULL, portNumChar, &hints, &result);
	if (iRes != 0) {
		printf("Problem getaddrinfo \n");
		WSACleanup();
		return 1;
	}

	clientSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (clientSocket == INVALID_SOCKET) {
		printf("Problem creating socket\n");
		WSACleanup();
		return 1;
	}

	iRes = connect(clientSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iRes == SOCKET_ERROR) {
		closesocket(clientSocket);
		clientSocket = INVALID_SOCKET;
		return 1;
	}

	freeaddrinfo(result);

	if (clientSocket == INVALID_SOCKET) {
		printf("Connect to server failed");
		WSACleanup();
		return 1;
	}

	receiveFromServer(clientSocket, recvBuf);

	do {
		cout << "Write sth: ";
		getline(cin, sendBuf);
		cout << endl;
		communicateToServer(clientSocket, recvBuf, sendBuf);
		receiveFromServer(clientSocket, recvBuf);
	} while (sendBuf.compare(".koniec"));
		
	iRes = shutdown(clientSocket, SD_SEND);
	if (iRes == SOCKET_ERROR) {
		printf("shutdown fail\n");
		closesocket(clientSocket);
		WSACleanup();
		return 1;
	}

	closesocket(clientSocket);
	WSACleanup();

	return 0;
}