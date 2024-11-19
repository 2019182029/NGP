#include <iostream>
#include "Common.h"

using namespace std;

// ���� ����
std::array<Packet, 4> ClientInfoArray;
std::queue<Packet> ClientServerQueue;
std::array<Packet, 4> ServerClientArray;
bool isGameStart = false;

// Ŭ���̾�Ʈ �����忡 ������ ������ ����ü
struct ThreadParam {
	SOCKET s;
	std::array<Packet, 4>* CIA;
	std::queue<Packet>* CSQ;
};

int main() {
	cout << "Server Starting..." << endl;
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);

	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in serverAddr = {};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(12345);
	serverAddr.sin_addr.s_addr = INADDR_ANY;

	bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
	listen(serverSocket, 4);

	while (true) {
		sockaddr_in clientAddr;
		int clientSize = sizeof(clientAddr);
		SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientSize);
		if (clientSocket != INVALID_SOCKET) {
			cout << "Client Connected" << endl;

			// ClientServerThread ������ ����
			ThreadParam* param = new ThreadParam{ clientSocket, &ClientInfoArray, &ClientServerQueue };
			CreateThread(NULL, 0, ClientServerThread, param, 0, NULL);
		}
	}

	closesocket(serverSocket);
	WSACleanup();
	return 0;

}

void init(SOCKET s, std::array<Packet, 4>* CIA, std::queue<Packet>* CSQ) {
	bool assigned = false;
	for (int i = 0; i < CIA->size(); i++) {
		if ((*CIA)[i].getValid() == 0) {
			// Ŭ���̾�Ʈ ������ �ʱ�ȭ
			Packet pkt;
			pkt.setValid(true);
			pkt.setPlayerNumber(i);
			(*CIA)[i] = pkt;

			// Ŭ���̾�Ʈ���� ����
			send(s, reinterpret_cast<const char*>(&pkt), sizeof(pkt), 0);

			// Recv ������ ����
			ThreadParam* param = new ThreadParam{ s, CIA, CSQ };
			CreateThread(NULL, 0, RecvThread, param, 0, NULL);
			assigned = true;
			break;
		}
	}
	if (!assigned) {
		// �� �ڸ��� ���� ���
		Packet invalidPkt;
		invalidPkt.setValid(false);
		send(s, reinterpret_cast<const char*>(&invalidPkt), sizeof(invalidPkt), 0);
	}
}

DWORD WINAPI RecvThread(LPVOID param) {
	ThreadParam* recvParam = reinterpret_cast<ThreadParam*>(param);
	SOCKET s = recvParam->s;
	std::array<Packet, 4>* CIA = recvParam->CIA;
	std::queue<Packet>* CSQ = recvParam->CSQ;

	while (true) {
		Packet receivedPkt;
		int bytesReceived = recv(s, reinterpret_cast<char*>(&receivedPkt), sizeof(receivedPkt), 0);
		if (bytesReceived > 0) {
			if (!isGameStart) {
				// ���� ���� �� ���� ������Ʈ
				int playerNum = receivedPkt.getPlayerNumber();
				if (playerNum >= 0 && playerNum < CIA->size()) {
					(*CIA)[playerNum] = receivedPkt;
				}
			}
			else {
				// ���� �� ������ ť�� �߰�
				CSQ->push(receivedPkt);
			}
		}
	}

	delete recvParam; // ���� �޸� ����
	return 0;
}

DWORD WINAPI ClientServerThread(LPVOID param) {
	/*init(s, CIA*, CSQ*);

	while (1) {
		if (isGameStart) {
			if (elapsedTime > Timeout) {
				ServerClientArray�� ������ Read�Ͽ� Ŭ���̾�Ʈ���� send();
			}
		}
	}*/
}