#include "Common.h"
#include "InGameThread.h"

void Init(std::array<Packet, 4>* ClientInfoArray, std::array<Packet, 4>* ServerClientArray, std::array<Object, 4>* ClientInfo, CRITICAL_SECTION* ServerClientArray_CS, volatile bool* isGameStart) {
	EnterCriticalSection(ServerClientArray_CS);
	
	for (int i = 0; i < 4; ++i) {
		// ServerClientArray 초기화
		if ((*ClientInfoArray)[i].GetValidBit()) {
			(*ClientInfo)[i].SetValidBit(1);  (*ServerClientArray)[i].SetValidBit(1);
			(*ClientInfo)[i].SetPlayerNumber(i);  (*ServerClientArray)[i].SetPlayerNumber(i);
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

void PopFromClientServerQueue(std::queue<Packet>* ClientServerQueue, std::array<Object, 4>* ClientInfo, CRITICAL_SECTION* ClientServerQueue_CS) {
	if (!ClientServerQueue->empty()) {
		EnterCriticalSection(ClientServerQueue_CS);

		Update(ClientServerQueue->front(), ClientInfo);  // 큐의 첫 번째 요소를 가져와서 플레이어 정보 갱신
		ClientServerQueue->pop();  // 큐의 첫 번째 요소 삭제

		LeaveCriticalSection(ClientServerQueue_CS);
	}
}

void Update(Packet packet, std::array<Object, 4>* ClientInfo) {
	switch (packet.GetKeyState() ^ (*ClientInfo)[packet.GetPlayerNumber()].GetKeyState()) {
	case 0b0100:  // A 키
		if (packet.GetKeyState() & 0100) { (*ClientInfo)[packet.GetPlayerNumber()].SetDir(-1.0f, 0.0f); }
		else { (*ClientInfo)[packet.GetPlayerNumber()].SetDir(1.0f, 0.0f); }
		break;

	case 0b0010:  // D 키
		if (packet.GetKeyState() & 0010) { (*ClientInfo)[packet.GetPlayerNumber()].SetDir(1.0f, 0.0f); }
		else { (*ClientInfo)[packet.GetPlayerNumber()].SetDir(-1.0f, 0.0f); }
		break;

	case 0b0001:  // C 키
		if (packet.GetKeyState() & 0001) {  
			if ((*ClientInfo)[packet.GetPlayerNumber()].GetItemBit()) {  // 해당 플레이어가 아이템을 가지고 있다면
				for (int i = 0; i < 4; ++i) {
					if (i != packet.GetPlayerNumber()) {  // 해당 플레이어를 제외한 나머지 모든 플레이어에게
						(*ClientInfo)[i].SetAplliedBit(true);  // 아이템 효과 적용 후
					}
				}
				(*ClientInfo)[packet.GetPlayerNumber()].SetItemBit(0);  // 해당 플레이어의 아이템 보유 여부를 false로 갱신
			}
		}
		break;

	default: 
		break;
	}

	(*ClientInfo)[packet.GetPlayerNumber()].SetKeyState(packet.GetKeyState());  // ClientInfo의 KeyState 갱신
}

void RenewalServerClientArray(std::array<Packet, 4>* ServerClientArray, std::array<Object, 4>* ClientInfo, CRITICAL_SECTION* ServerClientArray_CS) {
	EnterCriticalSection(ServerClientArray_CS);

	for (int i = 0; i < 4; ++i) {
		if ((*ClientInfo)[i].GetValidBit()) {
			(*ServerClientArray)[i].SetItemBit((*ClientInfo)[i].GetItemBit());
			(*ServerClientArray)[i].SetAplliedBit((*ClientInfo)[i].GetAppliedBit());
			(*ServerClientArray)[i].SetSurvivingBit((*ClientInfo)[i].GetSurvivingBit());
			(*ServerClientArray)[i].SetPosition((*ClientInfo)[i].GetXPosition(), (*ClientInfo)[i].GetYPosition());
			(*ServerClientArray)[i].SetCurrentSurface((*ClientInfo)[i].GetCurrentSurface());
		}
	}

	LeaveCriticalSection(ServerClientArray_CS);
}

DWORD __stdcall InGameThread(LPVOID arg) {
	std::array<Object, 4> Players;
	std::array<Object, 10> Obstacles;

	auto beforeTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	double elapsedTime;
	auto totalElapsedTime = std::chrono::microseconds(0);

	Init(((ThreadArg*)arg)->GetClientInfoArray(), ((ThreadArg*)arg)->GetServerClientArray(), &Players, ((ThreadArg*)arg)->GetServerClientArrayCS(), ((ThreadArg*)arg)->GetGameStartOrNot());

	// 게임 루프
	while (1) {
		// ClientServerQueue가 비어있지 않다면 ClientInfo 갱신
		PopFromClientServerQueue(((ThreadArg*)arg)->GetClientServerQueue(), &Players, ((ThreadArg*)arg)->GetClientServerQueueCS());

		// 모든 플레이어가 사망했다면 프로그램 종료
		if (!std::count_if(Players.begin(), Players.end(), [](auto& packet) { return packet.GetSurvivingBit() == 0; })) {
			((ThreadArg*)arg)->SetGameStartOrNot(false);
			break;
		}

		currentTime = std::chrono::high_resolution_clock::now();
		elapsedTime = ((std::chrono::duration<double>)std::chrono::duration_cast<std::chrono::microseconds>(currentTime - beforeTime)).count();
		totalElapsedTime += std::chrono::duration_cast<std::chrono::microseconds>(currentTime - beforeTime);
		beforeTime = std::chrono::high_resolution_clock::now();

		// 클라이언트 이동, 충돌 검사
		for (auto& ObjectInfo : Players) {
			// 클리아인트 이동 


			// 충돌 검사 : Item, Surviving
		}

		// totalElapsedTime이 60분의 1초를 경과했을 시 ServerClientArray 갱신
		if (totalElapsedTime.count() >= 16'667) {
			RenewalServerClientArray(((ThreadArg*)arg)->GetServerClientArray(), &Players, ((ThreadArg*)arg)->GetServerClientArrayCS());

			totalElapsedTime = std::chrono::microseconds(0);  // totalElapsedTime를 0초로 갱신
		}
	}

	return 0;
}
