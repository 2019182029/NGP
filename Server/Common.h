#define _CRT_SECURE_NO_WARNINGS // ���� C �Լ� ��� �� ��� ����
#define _WINSOCK_DEPRECATED_NO_WARNINGS // ���� ���� API ��� �� ��� ����

#include <winsock2.h> // ����2 ���� ���
#include <ws2tcpip.h> // ����2 Ȯ�� ���

#include <tchar.h> // _T(), ...
#include <stdio.h> // printf(), ...
#include <stdlib.h> // exit(), ...
#include <string.h> // strncpy(), ...

#include <array>
#include <queue>

#pragma comment(lib, "ws2_32") // ws2_32.lib ��ũ

class Packet {
private:
	float x, y;
	BYTE state[2];

public:
	void setValid(bool isValid) { state[0] = (state[0] & 0b01111111) | (isValid << 7); }
	void setPlayerNumber(int num) { state[0] = (state[0] & 0b10011111) | ((num & 0b11) << 5); }
	BYTE getValid() const { return (state[0] >> 7) & 0b1; }
	int getPlayerNumber() const { return (state[0] >> 5) & 0b11; }
};

class InfoCheckThreadArg {
private:
	std::array<Packet, 4>* m_CIA;
	std::queue<Packet>* m_CSQ;
	std::array<Packet, 4>* m_SCA;

	HANDLE* m_WriteEvent;
	CRITICAL_SECTION* m_CS_CSQ;
	CRITICAL_SECTION* m_CS_SCA;

public:
	void SetArg(std::array<Packet, 4>* CIA, std::queue<Packet>* CSQ, std::array<Packet, 4>* SCA, HANDLE* WriteEvent, CRITICAL_SECTION* CS_CSQ, CRITICAL_SECTION* CS_SCA) {
		m_CIA = CIA; m_CSQ = CSQ; m_SCA = SCA;
		m_WriteEvent = WriteEvent; m_CS_CSQ = CS_CSQ; m_CS_SCA = CS_SCA;
	};
};
