#include "Common.h"
#include "InGameThread.h"

void Init(std::array<Packet, 4>* ClientInfo, std::array<Packet, 4>* ClientInfoArray, std::array<Packet, 4>* ServerClientArray, CRITICAL_SECTION* ServerClientArray_CS, bool* isGameStart) {
	EnterCriticalSection(ServerClientArray_CS);
	
	for (int i = 0; i < 4; ++i) {
		// ServerClientArray 초기화
		if ((*ClientInfoArray)[i].GetValidBit()) {
			(*ClientInfo)[i].SetValidBit(1);  (*ServerClientArray)[i].SetValidBit(1);
			(*ServerClientArray)[i].SetPlayerNumber(i);
			(*ClientInfo)[i].SetItemBit(0);  (*ServerClientArray)[i].SetItemBit(0);
			(*ClientInfo)[i].SetAplliedBit(0);  (*ServerClientArray)[i].SetAplliedBit(0);
			(*ClientInfo)[i].SetSurvivingBit(1);  (*ServerClientArray)[i].SetSurvivingBit(1);
			(*ClientInfo)[i].SetKeyState(0b0000); 
			(*ServerClientArray)[i].SetSeed(0);

			switch (i) {
			case 0:  // 0번 플레이어
				(*ClientInfo)[i].SetPosition(0.0f, 0.0f);  (*ServerClientArray)[i].SetPosition(0.0f, 0.0f);
				(*ClientInfo)[i].SetCurrentSurface(0);  (*ServerClientArray)[i].SetCurrentSurface(0);  // 아랫면
				break;

			case 1:  // 1번 플레이어
				(*ClientInfo)[i].SetPosition(2.0f, 2.0f);  (*ServerClientArray)[i].SetPosition(2.0f, 2.0f);
				(*ClientInfo)[i].SetCurrentSurface(1);  (*ServerClientArray)[i].SetCurrentSurface(1);  // 오른면
				break;

			case 2:  // 2번 플레이어
				(*ClientInfo)[i].SetPosition(0.0f, 4.0f);  (*ServerClientArray)[i].SetPosition(0.0f, 4.0f);
				(*ClientInfo)[i].SetCurrentSurface(2);  (*ServerClientArray)[i].SetCurrentSurface(2);  // 윗면
				break;

			case 3:  // 3번 플레이어
				(*ClientInfo)[i].SetPosition(-2.0f, 2.0f);  (*ServerClientArray)[i].SetPosition(-2.0f, 2.0f);
				(*ClientInfo)[i].SetCurrentSurface(3);  (*ServerClientArray)[i].SetCurrentSurface(3);  // 아랫면
				break;

			default:
				break;
			}
		}
	}

	LeaveCriticalSection(ServerClientArray_CS);

	*isGameStart = true;
}

DWORD __stdcall InGameThread(LPVOID arg) {
	std::array<Packet, 4> ClientInfo;

	auto beforeTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	auto elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - beforeTime);

	Init(&ClientInfo, ((ThreadArg*)arg)->GetClientInfoArray(), ((ThreadArg*)arg)->GetServerClientArray(), ((ThreadArg*)arg)->GetServerClientArrayCS(), ((ThreadArg*)arg)->GetGameStartOrNot());

	// 게임 루프
	while (1) {
		// ClientServerQueue가 비어있지 않다면 ClientInfo 갱신
		if (!((ThreadArg*)arg)->GetClientServerQueue()->empty()) {  

		}

		// 모든 플레이어가 사망했다면 프로그램 종료
		if (!std::count_if(ClientInfo.begin(), ClientInfo.end(), [](const auto& packet) { return packet.GetSurvivingBit() == 0; })) {  

		}

		// 클라이언트 이동, 충돌 검사
		for (auto& packet : ClientInfo) {
			// 클리아인트 이동 : Position, CurrentSurface
			// 충돌 검사 : Item, Applied, Surviving
		}

		currentTime = std::chrono::high_resolution_clock::now();
		elapsedTime += std::chrono::duration_cast<std::chrono::microseconds>(currentTime - beforeTime);

		// 60분의 1초 경과 시 ServerClientArray 갱신
		if (elapsedTime.count() >= 16'667) { 

		}
	}

	return 0;
}
