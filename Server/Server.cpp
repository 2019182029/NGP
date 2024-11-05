#include "Common.h"
#include "InfoCheckThread.h"

#define SERVERPORT 9000

// 오류 출력 함수
void err_quit(const char* msg);
void err_display(const char* msg);
void err_display(int errcode);

// 서버 전역 변수
std::array<Packet, 4> ClientInfoArray;
std::queue<Packet> ClientServerQueue;
std::array<Packet, 4> ServerClientArray;

// 동기화 객체
HANDLE WriteEvent;
CRITICAL_SECTION CS_CSQ;
CRITICAL_SECTION CS_SCA;

int main(int argc, char* argv[]) {
	// 데이터 통신에 사용할 변수
	int retval;
	SOCKET client_sock;
	struct sockaddr_in clientaddr;
	int addrlen;

	// 동기화 객체 생성
	WriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	InitializeCriticalSection(&CS_CSQ);
	InitializeCriticalSection(&CS_SCA);

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

	// 정보 확인 스레드 생성
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

		// 클라이언트 전용 스레드 생성
	}

	// 소켓 닫기
	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();

	// 동기화 객체 제거
	DeleteCriticalSection(&CS_CSQ);
	DeleteCriticalSection(&CS_SCA);
	CloseHandle(WriteEvent);
	
	return 0;
}

// 소켓 함수 오류 출력 후 종료
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

// 소켓 함수 오류 출력
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

// 소켓 함수 오류 출력
void err_display(int errcode) {
	LPVOID lpMsgBuf;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, errcode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(char*)&lpMsgBuf, 0, NULL);
	printf("[오류] %s\n", (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}
