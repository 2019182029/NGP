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
#include <chrono>
#include <iostream>
#include <random>

#pragma comment(lib, "ws2_32") // ws2_32.lib 링크

class Packet {
private:
	float x, y;
	BYTE state[2];

public:
	void SetPosition(float fx, float fy) { x = fx; y = fy; }
	void SetReadyBit(bool ready) { state[0] = ready ? (state[0] | 0b10000000) : (state[0] & 0b01111111); }
	void SetStartBit(bool start) { state[0] = start ? (state[0] | 0b01000000) : (state[0] & 0b10111111); }
	void SetValidBit(bool valid) { state[0] = valid ? (state[0] | 0b00100000) : (state[0] & 0b11011111); }
	void SetPlayerNumber(int number) { state[0] = ((state[0] & 0b11100111) | (number << 3)); }
	void SetItemBit(bool item) { state[0] = item ? (state[0] | 0b00000100) : (state[0] & 0b11111011); }
	void SetAplliedBit(bool applied) { state[0] = applied ? (state[0] | 0b00000010) : (state[0] & 0b11111101); }
	void SetSurvivingBit(bool surviving) { state[0] = surviving ? (state[0] | 0b00000001) : (state[0] & 0b11111110); }
	void SetCurrentSurface(int surface) { state[1] = ((state[1] & 0b00111111) | (surface << 6)); }
	void SetKeyState(int keyState) { state[1] = ((state[1] & 0b11000011) | (keyState << 4)); }
	void SetSeed(int seed) { state[1] = ((state[1] & 0b11111100) | seed); }

	float GetXPosition() { return x; }
	float GetYPosition() { return y; }
	bool GetReadyBit() { return (state[0] & 0b10000000) >> 7; }
	bool GetStartBit() { return (state[0] & 0b01000000) >> 6; }
	bool GetValidBit() { return (state[0] & 0b00100000) >> 5; }
	int GetPlayerNumber() { return (state[0] & 0b00011000) >> 3; }
	bool GetItemBit() { return (state[0] & 0b00000100) >> 2; }
	bool GetAppliedBit() { return (state[0] & 0b00000010) >> 1; }
	bool GetSurvivingBit() { return state[0] & 0b00000001; }
	int GetCurrentSurface() { return (state[1] & 0b11000000) >> 6; }
	int GetKeyState() { return (state[1] & 0b00111100) >> 2; }
	int GetSeed() { return state[1] & 0b00000011; }
};

class ThreadArg {
private:
	SOCKET m_sock;

	std::array<Packet, 4>* m_ClientInfoArray;
	std::queue<Packet>* m_ClientServerQueue;
	std::array<Packet, 4>* m_ServerClientArray;

	HANDLE* m_ClientInfoArray_WriteEvent;
	HANDLE* m_ClientInfoArray_ReadEvent;
	CRITICAL_SECTION* m_ClientServerQueue_CS;
	CRITICAL_SECTION* m_ServerClientArray_CS;

	volatile bool* m_isGameStarted;

public:
	void SetSocket(SOCKET sock) { m_sock = sock; }
	void SetClientInfoArray(std::array<Packet, 4>* ClientInfoArray) { m_ClientInfoArray = ClientInfoArray; }
	void SetClientServerQueue(std::queue<Packet>* ClientServerQueue) { m_ClientServerQueue = ClientServerQueue; }
	void SetServerClientArray(std::array<Packet, 4>* ServerClientArray) { m_ServerClientArray = ServerClientArray; }
	void SetClientInfoArrayWriteEvent(HANDLE* ClientInfoArray_WriteEvent) { m_ClientInfoArray_WriteEvent = ClientInfoArray_WriteEvent; }
	void SetClientInfoArrayReadEvent(HANDLE* ClientInfoArray_ReadEvent) { m_ClientInfoArray_ReadEvent = ClientInfoArray_ReadEvent; }
	void SetClientServerQueueCS(CRITICAL_SECTION* ClientServerQueue_CS) { m_ClientServerQueue_CS = ClientServerQueue_CS; }
	void SetServerClientArrayCS(CRITICAL_SECTION* ServerClientArray_CS) { m_ServerClientArray_CS = ServerClientArray_CS; }
	void SetGameStartOrNot(volatile bool* isGameStarted) { m_isGameStarted = isGameStarted; }

	SOCKET GetSocket() { return m_sock; }
	std::array<Packet, 4>* GetClientInfoArray() { return m_ClientInfoArray; }
	std::queue<Packet>* GetClientServerQueue() { return m_ClientServerQueue; }
	std::array<Packet, 4>* GetServerClientArray() { return m_ServerClientArray; }
	HANDLE* GetClientInfoArrayWriteEvent() { return m_ClientInfoArray_WriteEvent; }
	HANDLE* GetClientInfoArrayReadEvent() { return m_ClientInfoArray_ReadEvent; }
	CRITICAL_SECTION* GetClientServerQueueCS() { return m_ClientServerQueue_CS; }
	CRITICAL_SECTION* GetServerClientArrayCS() { return m_ServerClientArray_CS; }
	volatile bool* GetGameStartOrNot() { return m_isGameStarted; }
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
