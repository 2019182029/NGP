#define _CRT_SECURE_NO_WARNINGS // 구형 C 함수 사용 시 경고 끄기
#define _WINSOCK_DEPRECATED_NO_WARNINGS // 구형 소켓 API 사용 시 경고 끄기

#include <winsock2.h> // 윈속2 메인 헤더
#include <ws2tcpip.h> // 윈속2 확장 헤더

#include <tchar.h> // _T(), ...
#include <stdio.h> // printf(), ...
#include <stdlib.h> // exit(), ...
#include <string.h> // strncpy(), ...

#include <array>  // ClientInfoArray, ServerClientArray
#include <queue>  // ClientServerQueue

#pragma comment(lib, "ws2_32") // ws2_32.lib 링크

class Packet {
private:
	float x, y;
	BYTE state[2];
};

class ThreadArg {
private:
	std::array<Packet, 4>* m_CIA;
	std::queue<Packet>* m_CSQ;
	std::array<Packet, 4>* m_SCA;

	HANDLE* m_WriteEvent;
	CRITICAL_SECTION* m_CS_CSQ;
	CRITICAL_SECTION* m_CS_SCA;

public:
	ThreadArg(std::array<Packet, 4>* CIA, std::queue<Packet>* CSQ, std::array<Packet, 4>* SCA, HANDLE* WriteEvent, CRITICAL_SECTION* CS_CSQ, CRITICAL_SECTION* CS_SCA) : m_CIA(CIA), m_CSQ(CSQ), m_SCA(SCA), m_WriteEvent(WriteEvent), m_CS_CSQ(CS_CSQ), m_CS_SCA(CS_SCA) {};
};

// 소켓 함수 오류 출력 후 종료
inline void err_quit(const char* msg) {
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
inline void err_display(const char* msg) {
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
inline void err_display(int errcode) {
	LPVOID lpMsgBuf;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, errcode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(char*)&lpMsgBuf, 0, NULL);
	printf("[오류] %s\n", (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}
