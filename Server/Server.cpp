#include "Common.h"
#include "InfoCheckThread.h"

#define SERVERPORT 9000

// ���� ���� ����
std::array<Packet, 4> ClientInfoArray;
std::queue<Packet> ClientServerQueue;
std::array<Packet, 4> ServerClientArray;

// ����ȭ ��ü
HANDLE ClientInfoArray_Event;
CRITICAL_SECTION ClientServerQueue_CS;
CRITICAL_SECTION ServerClientArray_CS;

// ���� ���� ����
bool isGameStart = false;

int main(int argc, char* argv[]) {
	// ������ ��ſ� ����� ����
	int retval;
	SOCKET client_sock;
	struct sockaddr_in clientaddr;
	int addrlen;

	// ����ȭ ��ü ����
	ClientInfoArray_Event = CreateEvent(NULL, FALSE, FALSE, NULL);
	InitializeCriticalSection(&ClientServerQueue_CS);
	InitializeCriticalSection(&ServerClientArray_CS);

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
	ThreadArg InfoCheckThreadArg;
	InfoCheckThreadArg.SetSocket(NULL);
	InfoCheckThreadArg.SetClientInfoArray(&ClientInfoArray);
	InfoCheckThreadArg.SetClientServerQueue(&ClientServerQueue);
	InfoCheckThreadArg.SetServerClientArray(&ServerClientArray);
	InfoCheckThreadArg.SetClientInfoArrayEvent(&ClientInfoArray_Event);
	InfoCheckThreadArg.SetClientServerQueueCS(&ClientServerQueue_CS);
	InfoCheckThreadArg.SetServerClientArrayCS(&ServerClientArray_CS);
	InfoCheckThreadArg.SetGameStartOrNot(&isGameStart);

	// ���� Ȯ�� ������ ����
	HANDLE hThread;
	hThread = CreateThread(NULL, 0, InfoCheckThread, (LPVOID)&InfoCheckThreadArg, 0, NULL);
	if (hThread != NULL) CloseHandle(hThread);

	while (1) {
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}

		// Ŭ���̾�Ʈ ���� ������ ���� 
		ThreadArg* ClientServerThreadArg = new ThreadArg();
		ClientServerThreadArg->SetSocket(client_sock);
		ClientServerThreadArg->SetClientInfoArray(&ClientInfoArray);
		ClientServerThreadArg->SetClientServerQueue(&ClientServerQueue);
		ClientServerThreadArg->SetServerClientArray(&ServerClientArray);
		ClientServerThreadArg->SetClientInfoArrayEvent(&ClientInfoArray_Event);
		ClientServerThreadArg->SetClientServerQueueCS(&ClientServerQueue_CS);
		ClientServerThreadArg->SetServerClientArrayCS(&ServerClientArray_CS);
		ClientServerThreadArg->SetGameStartOrNot(&isGameStart);

		// Ŭ���̾�Ʈ ���� ������ ����
		hThread = CreateThread(NULL, 0, ClientServerThread, (LPVOID)ClientServerThreadArg, 0, NULL);
		if (hThread == NULL) { 
			delete ClientServerThreadArg;
			closesocket(client_sock); 
		}
		else { CloseHandle(hThread); }
	}

	// ���� �ݱ�
	closesocket(listen_sock);

	// ���� ����
	WSACleanup();

	// ����ȭ ��ü ����
	DeleteCriticalSection(&ServerClientArray_CS);
	DeleteCriticalSection(&ClientServerQueue_CS);
	CloseHandle(ClientInfoArray_Event);
	
	return 0;
}
