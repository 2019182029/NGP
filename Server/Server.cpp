#include "Common.h"
#include "InfoCheckThread.h"
#include "ClientServerThread.h"

#define SERVERPORT 9000

// ���� ���� ����
std::array<Packet, 4> ClientInfoArray;
std::queue<Packet> ClientServerQueue;
std::array<Packet, 4> ServerClientArray;

// ����ȭ ��ü
HANDLE ClientInfoArray_WriteEvent;
HANDLE ClientInfoArray_ReadEvent;
CRITICAL_SECTION ClientServerQueue_CS;
CRITICAL_SECTION ServerClientArray_CS;

// ���� ���� ����
volatile bool isGameStart = false;

int main(int argc, char* argv[]) {
	// ������ ��ſ� ����� ����
	int retval;
	SOCKET client_sock;
	struct sockaddr_in clientaddr;
	int addrlen;

	// ����ȭ ��ü ����
	ClientInfoArray_WriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	ClientInfoArray_ReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
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
	InfoCheckThreadArg.SetClientInfoArrayWriteEvent(&ClientInfoArray_WriteEvent);
	InfoCheckThreadArg.SetClientInfoArrayReadEvent(&ClientInfoArray_ReadEvent);
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

		// ������ Ŭ���̾�Ʈ ���� ���
		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
		std::cout << "Ŭ���̾�Ʈ ���� : IP �ּ� = " << addr << ", ��Ʈ ��ȣ = " << ntohs(clientaddr.sin_port) << std::endl;

		// Ŭ���̾�Ʈ ���� ������ ���� 
		ThreadArg* ClientServerThreadArg = new ThreadArg();
		ClientServerThreadArg->SetSocket(client_sock);
		ClientServerThreadArg->SetClientInfoArray(&ClientInfoArray);
		ClientServerThreadArg->SetClientServerQueue(&ClientServerQueue);
		ClientServerThreadArg->SetServerClientArray(&ServerClientArray);
		ClientServerThreadArg->SetClientInfoArrayWriteEvent(&ClientInfoArray_WriteEvent);
		ClientServerThreadArg->SetClientInfoArrayReadEvent(&ClientInfoArray_ReadEvent);
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
	CloseHandle(ClientInfoArray_WriteEvent);
	
	return 0;
}
