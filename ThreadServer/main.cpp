#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <stdio.h>
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <sstream>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

#define errNo WSAGetLastError()
#define PORT 27047
#define BUFFER_LEN 128

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
	string getPortNoAsStr() {
		stringstream ss;
		ss << no;
		return ss.str();
	}
} PORT_NO, * LPPORT_NO;

typedef struct SOCKETS {
	SOCKET serverSocket;
	SOCKET clientSocket;
} SOCKETS;

typedef class ClientContext {
private:
	OVERLAPPED* op;
	WSABUF* buf;
	SOCKET socket;
	char msg_buf[BUFFER_LEN];
public:
	void setSocket(SOCKET s) {
		socket = s;
	}
	SOCKET getSocket() {
		return socket;
	}
	WSABUF* getWSABUFPtr() {
		return buf;
	}
	OVERLAPPED* getOverlappedPtr() {
		return op;
	}
	ClientContext() {
		op = new OVERLAPPED;
		buf = new WSABUF;
		socket = INVALID_SOCKET;
		memset(op, 0, sizeof(OVERLAPPED));
		memset(msg_buf, 0, sizeof(msg_buf));
		buf->buf = msg_buf;
		buf->len = BUFFER_LEN;
	}
	~ClientContext(){
		closesocket(socket);
		delete op;
		delete buf;
	}
} ClientContext, *lpClientContext;

void acceptConnection(SOCKET listenerSocket);
DWORD WINAPI workerThread(void* lpParam);

HANDLE hIoCP;

int main(int argc, char** argv) {

	int iRes;
	WSAData wsaData;
	WORD ver = MAKEWORD(2, 1);

	PORT_NO port(PORT);

	SOCKETS sockets;
	sockets.clientSocket = INVALID_SOCKET;

	addrinfo hints, *result;
	memset(&hints, 0, sizeof(hints));
	memset(&result, 0, sizeof(result));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	iRes = WSAStartup(ver, &wsaData);
	if (iRes != 0) {
		printf("Error at WSAStartup no.%d", errNo);
		return 1;
	}

	iRes = getaddrinfo(NULL, port.getPortNoAsStr().c_str(), &hints, &result);
	if (iRes != 0) {
		printf("Error at getaddrinfo");
		WSACleanup();
		return 1;
	}

	sockets.serverSocket = INVALID_SOCKET;
	sockets.serverSocket = WSASocket(result->ai_family, result->ai_socktype, result->ai_protocol, 
		NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == sockets.serverSocket) {
		printf("Error creating socket");
		WSACleanup();
		return 1;
	}

	iRes = bind(sockets.serverSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iRes != 0) {
		printf("Error binding socket");
		freeaddrinfo(result);
		closesocket(sockets.serverSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	hIoCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	if (NULL == hIoCP) {
		printf("Error creating Completion Port");
		return 1;
	}

	DWORD threadId;
	for (int i = 0; i < 4; i++) {
		CreateThread(NULL, 0, workerThread, (void*)(i + 1), 0, &threadId);
	}

	WSAEVENT acceptEvent = WSACreateEvent();
	WSAEventSelect(sockets.serverSocket, acceptEvent, FD_ACCEPT);

	iRes = listen(sockets.serverSocket, SOMAXCONN);
	if (iRes != 0) {
		printf("Error listening");
		closesocket(sockets.serverSocket);
		WSACleanup();
		return 1;
	}

	printf("Listening for incoming conns...\n");

	WSANETWORKEVENTS wsaNetEvent;
	while (1) {
		if (WAIT_OBJECT_0 == WaitForSingleObject(acceptEvent, INFINITE)) {
			WSAEnumNetworkEvents(sockets.serverSocket, acceptEvent, &wsaNetEvent);
			bool temp1 = wsaNetEvent.lNetworkEvents & FD_ACCEPT;
			bool temp2 = wsaNetEvent.iErrorCode[FD_ACCEPT_BIT] == 0;
			if (temp1 && temp2) {
				printf("Accept request incoming\n");
				acceptConnection(sockets.serverSocket);
			}
		}
		
	}	

	return 0;
}
void acceptConnection(SOCKET listenerSocket) {

	SOCKET clientSocket = accept(listenerSocket, NULL, NULL);

	send(clientSocket, "Hello", 5, 0);

	ClientContext* p_client_context = new ClientContext;
	p_client_context->setSocket(clientSocket);

	hIoCP = CreateIoCompletionPort((HANDLE)p_client_context->getSocket(), hIoCP, (DWORD)p_client_context, 0);

	WSABUF* p_wbuf = p_client_context->getWSABUFPtr();
	OVERLAPPED* p_ovp = p_client_context->getOverlappedPtr();

	DWORD dwBytes = 0;
	DWORD dwFlags = 0;

	printf("Receiving initial data\n");
	int init_bytes_rcv = WSARecv(p_client_context->getSocket(), p_wbuf, 1, &dwBytes, &dwFlags, p_ovp, NULL);
	if (SOCKET_ERROR == init_bytes_rcv) {
		printf("No bytes rcvd");
	}
	printf("Initial data received\n");
}
DWORD WINAPI workerThread(void* lpParam) {
	
	int n_thread_no = (int)lpParam;

	void* lpContext = NULL;
	DWORD dw_bytes_transf = 0;
	OVERLAPPED* ov_temp = NULL;
	ClientContext* p_client_context = new ClientContext;
	DWORD dw_bytes = 0, dw_flags = 0;

	while (1) {
		
		bool ret = GetQueuedCompletionStatus( hIoCP, &dw_bytes_transf, (LPDWORD)&p_client_context, &ov_temp, INFINITE);

		WSABUF* wsa_buf = p_client_context->getWSABUFPtr();
		OVERLAPPED* ov_ov = p_client_context->getOverlappedPtr();

		int bytes_recv = WSARecv(p_client_context->getSocket(), wsa_buf, 1, &dw_bytes, &dw_flags, ov_ov, NULL);
		send(p_client_context->getSocket(), "OK", 2, NULL);
		printf("%s\n", wsa_buf->buf);

	}

}
