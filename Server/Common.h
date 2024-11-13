#define _CRT_SECURE_NO_WARNINGS // ���� C �Լ� ��� �� ��� ����
#define _WINSOCK_DEPRECATED_NO_WARNINGS // ���� ���� API ��� �� ��� ����

#include <winsock2.h> // ����2 ���� ���
#include <ws2tcpip.h> // ����2 Ȯ�� ���

#include <tchar.h> // _T(), ...
#include <stdio.h> // printf(), ...
#include <stdlib.h> // exit(), ...
#include <string.h> // strncpy(), ...

#include <array>  // ClientInfoArray, ServerClientArray
#include <queue>  // ClientServerQueue

#pragma comment(lib, "ws2_32") // ws2_32.lib ��ũ

class Packet {
private:
	float x, y;
	BYTE state[2];
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

public:
	void SetSocket(SOCKET sock) { m_sock = sock; }
	void SetClientInfoArray(std::array<Packet, 4>* ClientInfoArray) { m_ClientInfoArray = ClientInfoArray; }
	void SetClientServerQueue(std::queue<Packet>* ClientServerQueue) { m_ClientServerQueue = ClientServerQueue; }
	void SetServerClientArray(std::array<Packet, 4>* ServerClientArray) { m_ServerClientArray = ServerClientArray; }
	void SetClientInfoArrayEvent(HANDLE* ClientInfoArray_Event) { m_ClientInfoArray_Event = ClientInfoArray_Event; }
	void SetClientServerQueueCS(CRITICAL_SECTION* ClientServerQueue_CS) { m_ClientServerQueue_CS = ClientServerQueue_CS; }
	void SetServerClientArrayCS(CRITICAL_SECTION* ServerClientArray_CS) { m_ServerClientArray_CS = ServerClientArray_CS; }

	SOCKET GetSocket() { return m_sock; }
	std::array<Packet, 4>* GetClientInfoArray() { return m_ClientInfoArray; }
	std::queue<Packet>* GetClientServerQueue() { return m_ClientServerQueue; }
	std::array<Packet, 4>* GetServerClientArray() { return m_ServerClientArray; }
	HANDLE* GetClientInfoArrayEvent() { return m_ClientInfoArray_Event; }
	CRITICAL_SECTION* GetClientServerQueueCS() { return m_ClientServerQueue_CS; }
	CRITICAL_SECTION* GetServerClientArrayCS() { return m_ServerClientArray_CS; }
};

// ���� �Լ� ���� ��� �� ����
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

// ���� �Լ� ���� ���
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

// ���� �Լ� ���� ���
inline void err_display(int errcode) {
	LPVOID lpMsgBuf;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, errcode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(char*)&lpMsgBuf, 0, NULL);
	printf("[����] %s\n", (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}
