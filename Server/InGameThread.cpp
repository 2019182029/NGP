#include "Common.h"
#include "InGameThread.h"

void Init(std::array<Packet, 4>* ClientInfoArray, std::array<Packet, 4>* ServerClientArray, bool* isGameStart) {
	for (int i = 0; i < 4; ++i) {
		// ServerClientArray 초기화
		if ((*ClientInfoArray)[i].GetValidBit()) {
			(*ServerClientArray)[i].SetValidBit((*ClientInfoArray)[i].GetValidBit());
			(*ServerClientArray)[i].SetPlayerNumber(i);
			(*ServerClientArray)[i].SetItemBit(0);
			(*ServerClientArray)[i].SetAplliedBit(0);
			(*ServerClientArray)[i].SetSurvivingBit(1);
			(*ServerClientArray)[i].SetKeyState(0b0000);
			(*ServerClientArray)[i].SetSeed(0);

			switch (i) {
			case 0:  // 0번 플레이어
				(*ServerClientArray)[i].SetPosition(0.0f, 0.0f);
				(*ServerClientArray)[i].SetCurrentSurface(0);  // 아랫면
				break;

			case 1:  // 1번 플레이어
				(*ServerClientArray)[i].SetPosition(2.0f, 2.0f);
				(*ServerClientArray)[i].SetCurrentSurface(1);  // 오른면
				break;

			case 2:  // 2번 플레이어
				(*ServerClientArray)[i].SetPosition(0.0f, 4.0f);
				(*ServerClientArray)[i].SetCurrentSurface(2);  // 윗면
				break;

			case 3:  // 3번 플레이어
				(*ServerClientArray)[i].SetPosition(-2.0f, 2.0f);
				(*ServerClientArray)[i].SetCurrentSurface(3);  // 왼면
				break;

			default:
				break;
			}
		}
	}

	*isGameStart = true;
}

DWORD __stdcall InGameThread(LPVOID arg) {
	Init(((ThreadArg*)arg)->GetClientInfoArray(), ((ThreadArg*)arg)->GetServerClientArray(), ((ThreadArg*)arg)->GetGameStartOrNot());

	// 게임 루프
	while (1) {

	}

	return 0;
}
