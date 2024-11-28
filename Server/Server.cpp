#include "Common.h"
#include "InfoCheckThread.h"
#include "ClientServerThread.h"

#define SERVERPORT 9000

// 서버 전역 변수
std::array<Packet, 4> ClientInfoArray;
std::queue<Packet> ClientServerQueue;
std::array<Packet, 4> ServerClientArray;

// 동기화 객체
HANDLE ClientInfoArray_WriteEvent;
HANDLE ClientInfoArray_ReadEvent;
CRITICAL_SECTION ClientServerQueue_CS;
CRITICAL_SECTION ServerClientArray_CS;

// 게임 시작 여부
volatile bool isGameStart = false;

int main(int argc, char* argv[]) {
	// 데이터 통신에 사용할 변수
	int retval;
	SOCKET client_sock;
	struct sockaddr_in clientaddr;
	int addrlen;

	// 동기화 객체 생성
	ClientInfoArray_WriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	ClientInfoArray_ReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
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

	// 정보 확인 스레드 인자 
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

	// 정보 확인 스레드 생성
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

		// 접속한 클라이언트 정보 출력
		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
		std::cout << "클라이언트 접속 : IP 주소 = " << addr << ", 포트 번호 = " << ntohs(clientaddr.sin_port) << std::endl;

		// 클라이언트 전용 스레드 인자 
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

		// 클라이언트 전용 스레드 생성
		hThread = CreateThread(NULL, 0, ClientServerThread, (LPVOID)ClientServerThreadArg, 0, NULL);
		if (hThread == NULL) { 
			delete ClientServerThreadArg;
			closesocket(client_sock); 
		}
		else { CloseHandle(hThread); }
	}

	// 소켓 닫기
	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();

	// 동기화 객체 제거
	DeleteCriticalSection(&ServerClientArray_CS);
	DeleteCriticalSection(&ClientServerQueue_CS);
	CloseHandle(ClientInfoArray_WriteEvent);
	
	return 0;
}
