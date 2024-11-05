#include "Common.h"
#include "InfoCheckThread.h"

#define SERVERPORT 9000

// ���� ��� �Լ�
void err_quit(const char* msg);
void err_display(const char* msg);
void err_display(int errcode);

// ���� ���� ����
std::array<Packet, 4> ClientInfoArray;
std::queue<Packet> ClientServerQueue;
std::array<Packet, 4> ServerClientArray;

// ����ȭ ��ü
HANDLE WriteEvent;
CRITICAL_SECTION CS_CSQ;
CRITICAL_SECTION CS_SCA;

int main(int argc, char* argv[]) {
	// ������ ��ſ� ����� ����
	int retval;
	SOCKET client_sock;
	struct sockaddr_in clientaddr;
	int addrlen;

	// ����ȭ ��ü ����
	WriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	InitializeCriticalSection(&CS_CSQ);
	InitializeCriticalSection(&CS_SCA);

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

	// ��� ���� ����
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	// ���� Ȯ�� ������ ����
	HANDLE hInfoCheckThread;
	InfoCheckThreadArg InfoCheckThreadArg;
	InfoCheckThreadArg.SetArg(&ClientInfoArray, &ClientServerQueue, &ServerClientArray, &WriteEvent, &CS_CSQ, &CS_SCA);
	hInfoCheckThread = CreateThread(NULL, 0, InfoCheckThread, (LPVOID)&InfoCheckThreadArg, 0, NULL);
	if (hInfoCheckThread != NULL) CloseHandle(hInfoCheckThread);

	while (1) {
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}

		// Ŭ���̾�Ʈ ���� ������ ����
	}

	// ���� �ݱ�
	closesocket(listen_sock);

	// ���� ����
	WSACleanup();

	// ����ȭ ��ü ����
	DeleteCriticalSection(&CS_CSQ);
	DeleteCriticalSection(&CS_SCA);
	CloseHandle(WriteEvent);
	
	return 0;
}

// ���� �Լ� ���� ��� �� ����
void err_quit(const char* msg) {
	LPVOID lpMsgBuf;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(char*)&lpMsgBuf, 0, NULL);
	MessageBoxA(NULL, (const char*)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// ���� �Լ� ���� ���
void err_display(const char* msg) {
	LPVOID lpMsgBuf;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(char*)&lpMsgBuf, 0, NULL);
	printf("[%s] %s\n", msg, (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

// ���� �Լ� ���� ���
void err_display(int errcode) {
	LPVOID lpMsgBuf;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, errcode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(char*)&lpMsgBuf, 0, NULL);
	printf("[����] %s\n", (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}
