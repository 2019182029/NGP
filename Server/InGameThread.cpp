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

	std::cout << "게임 시작" << std::endl;
}

DWORD __stdcall InGameThread(LPVOID arg) {
	std::array<Packet, 4> ClientInfo;

	auto beforeTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	auto elapsedTime = std::chrono::microseconds(0);
	auto totalElapsedTime = std::chrono::microseconds(0);

	Init(&ClientInfo, ((ThreadArg*)arg)->GetClientInfoArray(), ((ThreadArg*)arg)->GetServerClientArray(), ((ThreadArg*)arg)->GetServerClientArrayCS(), ((ThreadArg*)arg)->GetGameStartOrNot());

	// 게임 루프
	while (1) {
		// ClientServerQueue가 비어있지 않다면 ClientInfo 갱신
		EnterCriticalSection(((ThreadArg*)arg)->GetClientServerQueueCS());

		if (!((ThreadArg*)arg)->GetClientServerQueue()->empty()) {  
			Packet p = ((ThreadArg*)arg)->GetClientServerQueue()->front();  // 큐의 첫 번째 요소 가져오기

			if (p.GetKeyState() & 0001) {  // C 키가 눌린 거라면
				if (ClientInfo[p.GetPlayerNumber()].GetItemBit()) {  // 해당 플레이어가 아이템을 가지고 있다면
					for (int i = 0; i < 4; ++i) {
						if (i != p.GetPlayerNumber()) {  // 해당 플레이어를 제외한 나머지 모든 플레이어에게
							ClientInfo[i].SetAplliedBit(true);  // 아이템 적용
						}
					}
				}
			}
			ClientInfo[p.GetPlayerNumber()].SetKeyState(p.GetKeyState());  // ClientInfo 갱신

			((ThreadArg*)arg)->GetClientServerQueue()->pop();  // 큐의 첫 번째 요소 삭제
		}

		LeaveCriticalSection(((ThreadArg*)arg)->GetClientServerQueueCS());

		// 모든 플레이어가 사망했다면 프로그램 종료
		/*if (!std::count_if(ClientInfo.begin(), ClientInfo.end(), [](const auto& packet) { return packet.GetSurvivingBit() == 0; })) {  
			((ThreadArg*)arg)->SetGameStartOrNot(false);
			break;
		}*/

		currentTime = std::chrono::high_resolution_clock::now();
		elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - beforeTime);
		totalElapsedTime += std::chrono::duration_cast<std::chrono::microseconds>(currentTime - beforeTime);
		beforeTime = std::chrono::high_resolution_clock::now();

		// 클라이언트 이동, 충돌 검사
		for (auto& packet : ClientInfo) {
			// 클리아인트 이동 : Position, CurrentSurface
			// 충돌 검사 : Item, Applied, Surviving
		}

		// totalElapsedTime이 60분의 1초를 경과했을 시 ServerClientArray 갱신
		if (elapsedTime.count() >= 16'667) { 

			totalElapsedTime = std::chrono::microseconds(0);  // totalElapsedTime를 0초로 갱신
		}
	}

	return 0;
}
