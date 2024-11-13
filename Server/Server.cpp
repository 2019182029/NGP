#include "Common.h"
#include "InfoCheckThread.h"

#define SERVERPORT 9000

// 서버 전역 변수
std::array<Packet, 4> ClientInfoArray;
std::queue<Packet> ClientServerQueue;
std::array<Packet, 4> ServerClientArray;

// 동기화 객체
HANDLE ClientInfoArray_Event;
CRITICAL_SECTION ClientServerQueue_CS;
CRITICAL_SECTION ServerClientArray_CS;

int main(int argc, char* argv[]) {
	// 데이터 통신에 사용할 변수
	int retval;
	SOCKET client_sock;
	struct sockaddr_in clientaddr;
	int addrlen;

	// 동기화 객체 생성
	ClientInfoArray_Event = CreateEvent(NULL, FALSE, FALSE, NULL);
	InitializeCriticalSection(&ClientServerQueue_CS);
	InitializeCriticalSection(&ServerClientArray_CS);

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

	// 대기 소켓 생성
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

	// 스레드 인자 
	ThreadArg tArg;
	tArg.SetSocket(NULL);
	tArg.SetClientInfoArray(&ClientInfoArray);
	tArg.SetClientServerQueue(&ClientServerQueue);
	tArg.SetServerClientArray(&ServerClientArray);
	tArg.SetClientInfoArrayEvent(&ClientInfoArray_Event);
	tArg.SetClientServerQueueCS(&ClientServerQueue_CS);
	tArg.SetServerClientArrayCS(&ServerClientArray_CS);

	// 정보 확인 스레드 생성
	HANDLE hThread;
	hThread = CreateThread(NULL, 0, InfoCheckThread, (LPVOID)&tArg, 0, NULL);
	if (hThread != NULL) CloseHandle(hThread);

	while (1) {
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}

		// 클라이언트 전용 스레드 생성
		tArg.SetSocket(client_sock);
		hThread = CreateThread(NULL, 0, ClientServerThread, (LPVOID)&tArg, 0, NULL);
		if (hThread == NULL) { closesocket(client_sock); }
		else { CloseHandle(hThread); }
	}

	// 소켓 닫기
	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();

	// 동기화 객체 제거
	DeleteCriticalSection(&ServerClientArray_CS);
	DeleteCriticalSection(&ClientServerQueue_CS);
	CloseHandle(ClientInfoArray_Event);
	
	return 0;
}
