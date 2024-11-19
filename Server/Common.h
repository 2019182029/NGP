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

public:
	void SetPosition(float fx, float fy) { x = fx; y = fy; }
	void SetReadyBit(bool ready) { state[1] = ready ? (state[1] | 0b10000000) : (state[1] & 0b01111111); }
	void SetStartBit(bool start) { state[1] = start ? (state[1] | 0b01000000) : (state[1] & 0b10111111); }
	void SetValidBit(bool valid) { state[1] = valid ? (state[1] | 0b00100000) : (state[1] & 0b11011111); }
	void SetPlayerNumber(int number) { state[1] = ((state[1] & 0b11100111) | (number << 3)); }
	void SetItemBit(bool item) { state[1] = item ? (state[1] | 0b00000100) : (state[1] & 0b11111011); }
	void SetAplliedBit(bool applied) { state[1] = applied ? (state[1] | 0b00000010) : (state[1] & 0b11111101); }
	void SetSurvivingBit(bool surviving) { state[1] = surviving ? (state[1] | 0b00000001) : (state[1] & 0b11111110); }
	void SetCurrentSurface(int surface) { state[2] = ((state[2] & 0b00111111) | (surface << 6)); }
	void SetKeyState(int keyState) { state[2] = ((state[2] & 0b11000011) | (keyState << 4)); }
	void SetSeed(int seed) { state[2] = ((state[2] & 0b11111100) | seed); }

	bool GetReadyBit() { return (state[1] & 0b10000000) >> 7; }
	bool GetStartBit() { return (state[1] & 0b01000000) >> 6; }
	bool GetValidBit() { return (state[1] & 0b00100000) >> 5; }
	int GetPlayerNumber() { return (state[1] & 0b00011000) >> 3; }
	bool GetItemBit() { return (state[1] & 0b00000100) >> 2; }
	bool GetAppliedBit() { return (state[1] & 0b00000010) >> 1; }
	bool GetSurvivingBit() { return state[1] & 0b00000001; }
	int GetCurrentSurface() { return (state[2] & 0b11000000) >> 6; }
	int GetKeyState() { return (state[2] & 0b00111100) >> 2; }
	int GetSeed() { return state[2] & 0b00000011; }
};

class ThreadArg {
private:
	SOCKET m_sock;

	std::array<Packet, 4>* m_ClientInfoArray;
	std::queue<Packet>* m_ClientServerQueue;
	std::array<Packet, 4>* m_ServerClientArray;

	HANDLE* m_ClientInfoArray_Event;
	CRITICAL_SECTION* m_ClientServerQueue_CS;
	CRITICAL_SECTION* m_ServerClientArray_CS;

	bool* m_isGameStarted;

public:
	void SetSocket(SOCKET sock) { m_sock = sock; }
	void SetClientInfoArray(std::array<Packet, 4>* ClientInfoArray) { m_ClientInfoArray = ClientInfoArray; }
	void SetClientServerQueue(std::queue<Packet>* ClientServerQueue) { m_ClientServerQueue = ClientServerQueue; }
	void SetServerClientArray(std::array<Packet, 4>* ServerClientArray) { m_ServerClientArray = ServerClientArray; }
	void SetClientInfoArrayEvent(HANDLE* ClientInfoArray_Event) { m_ClientInfoArray_Event = ClientInfoArray_Event; }
	void SetClientServerQueueCS(CRITICAL_SECTION* ClientServerQueue_CS) { m_ClientServerQueue_CS = ClientServerQueue_CS; }
	void SetServerClientArrayCS(CRITICAL_SECTION* ServerClientArray_CS) { m_ServerClientArray_CS = ServerClientArray_CS; }
	void SetGameStartOrNot(bool* isGameStarted) { m_isGameStarted = isGameStarted; }

	SOCKET GetSocket() { return m_sock; }
	std::array<Packet, 4>* GetClientInfoArray() { return m_ClientInfoArray; }
	std::queue<Packet>* GetClientServerQueue() { return m_ClientServerQueue; }
	std::array<Packet, 4>* GetServerClientArray() { return m_ServerClientArray; }
	HANDLE* GetClientInfoArrayEvent() { return m_ClientInfoArray_Event; }
	CRITICAL_SECTION* GetClientServerQueueCS() { return m_ClientServerQueue_CS; }
	CRITICAL_SECTION* GetServerClientArrayCS() { return m_ServerClientArray_CS; }
	bool* GetGameStartOrNot() { return m_isGameStarted; }
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
