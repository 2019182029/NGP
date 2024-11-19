#include <iostream>
#include "Common.h"

using namespace std;

// 전역 변수
std::array<Packet, 4> ClientInfoArray;
std::queue<Packet> ClientServerQueue;
std::array<Packet, 4> ServerClientArray;
bool isGameStart = false;

// 클라이언트 스레드에 전달할 데이터 구조체
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

			// ClientServerThread 스레드 생성
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
			// 클라이언트 정보를 초기화
			Packet pkt;
			pkt.setValid(true);
			pkt.setPlayerNumber(i);
			(*CIA)[i] = pkt;

			// 클라이언트에게 응답
			send(s, reinterpret_cast<const char*>(&pkt), sizeof(pkt), 0);

			// Recv 스레드 생성
			ThreadParam* param = new ThreadParam{ s, CIA, CSQ };
			CreateThread(NULL, 0, RecvThread, param, 0, NULL);
			assigned = true;
			break;
		}
	}
	if (!assigned) {
		// 빈 자리가 없는 경우
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
				// 게임 시작 전 상태 업데이트
				int playerNum = receivedPkt.getPlayerNumber();
				if (playerNum >= 0 && playerNum < CIA->size()) {
					(*CIA)[playerNum] = receivedPkt;
				}
			}
			else {
				// 게임 중 데이터 큐에 추가
				CSQ->push(receivedPkt);
			}
		}
	}

	delete recvParam; // 동적 메모리 해제
	return 0;
}

DWORD WINAPI ClientServerThread(LPVOID param) {
	/*init(s, CIA*, CSQ*);

	while (1) {
		if (isGameStart) {
			if (elapsedTime > Timeout) {
				ServerClientArray의 정보를 Read하여 클라이언트에게 send();
			}
		}
	}*/
}