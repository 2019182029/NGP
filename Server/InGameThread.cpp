#include "Common.h"
#include "InGameThread.h"

std::default_random_engine dre(0);
std::uniform_int_distribution<int> uid(-15, 15);
std::uniform_int_distribution<int> uidZDir(1, 10);

void Init(std::array<Packet, 4>* ClientInfoArray, std::array<Packet, 4>* ServerClientArray, std::array<Object, 4>* ClientInfo, std::array<Object, 10>* Obstacles, CRITICAL_SECTION* ServerClientArray_CS, volatile bool* isGameStart) {
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

	// 장애물 초기화
	for (auto& obstacle : *Obstacles) {
		// 위치 
		obstacle.SetPosition(uid(dre) / 10.0f, uid(dre) / 10.0f);
		obstacle.SetZPosition(-100.0f);

		// 방향
		obstacle.SetDir(uid(dre) / 10.0f, uid(dre) / 10.0f, uidZDir(dre) / 10.0f);
	}

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
	switch (packet.GetKeyState() ^ (*ClientInfo)[packet.GetPlayerNumber()].GetKeyState()) {  // 어떤 키가 눌리거나 떼어졌는가?
	case 0b0100:  // A 키
		if (packet.GetKeyState() & 0b0100) {  // A 키가 눌렸다면
			(*ClientInfo)[packet.GetPlayerNumber()].SetDir(-1.0f, 0.0f); 
		}
		else {  // A 키가 떼어졌다면
			(*ClientInfo)[packet.GetPlayerNumber()].SetDir(1.0f, 0.0f); 
		}
		break;

	case 0b0010:  // D 키
		if (packet.GetKeyState() & 0b0010) {  // D 키가 눌렸다면
			(*ClientInfo)[packet.GetPlayerNumber()].SetDir(1.0f, 0.0f); 
		}
		else {  // D 키가 떼어졌다면
			(*ClientInfo)[packet.GetPlayerNumber()].SetDir(-1.0f, 0.0f); 
		}
		break;

	case 0b0001:  // C 키
		if (packet.GetKeyState() & 0b0001) {  // C 키가 눌렸다면
			if ((*ClientInfo)[packet.GetPlayerNumber()].GetItemBit()) {  // 해당 플레이어가 아이템을 가지고 있다면
				for (int i = 0; i < 4; ++i) {
					if (!(*ClientInfo)[i].GetValidBit()) { continue; }
					if (!(*ClientInfo)[i].GetSurvivingBit()) { continue; }

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

void MovePlayer(std::array<Object, 4>* ClientInfo, double elapsedTime) {
	for (auto& player : *ClientInfo) {
		if (!player.GetValidBit()) { continue; }
		if (!player.GetSurvivingBit()) { continue; }

		switch (player.GetCurrentSurface()) {
		case 0:  // 아랫면이 밑면일 때
			player.SetPosition(player.GetXPosition() + player.GetXDir() * (float)elapsedTime, player.GetYPosition());
			if (player.GetXPosition() < -2.0f + 0.25f) {  // 왼쪽 벽에 닿았다면
				player.SetPosition(-2.0f, 0.25f);
				player.SetCurrentSurface(3);
			}
			else if (player.GetXPosition() > 2.0f - 0.25f) {  // 오른쪽 벽에 닿았다면
				player.SetPosition(2.0f, 0.25f);
				player.SetCurrentSurface(1);
			}
			break;

		case 1:  // 오른면이 밑면일 때
			player.SetPosition(player.GetXPosition(), player.GetYPosition() + player.GetXDir() * (float)elapsedTime);
			if (player.GetYPosition() < 0.0f + 0.25f) {  // 아래쪽 벽에 닿았다면
				player.SetPosition(2.0f - 0.25f, 0.0f);
				player.SetCurrentSurface(0);
			}
			else if (player.GetYPosition() > 4.0f - 0.25f) {  // 위쪽 벽에 닿았다면
				player.SetPosition(2.0f - 0.25f, 4.0f);
				player.SetCurrentSurface(2);
			}
			break;

		case 2:  // 윗면이 밑면일 때
			player.SetPosition(player.GetXPosition() - player.GetXDir() * (float)elapsedTime, player.GetYPosition());
			if (player.GetXPosition() < -2.0f + 0.25f) {  // 왼쪽 벽에 닿았다면
				player.SetPosition(-2.0f, 4.0f - 0.25f);
				player.SetCurrentSurface(3);
			}
			else if (player.GetXPosition() > 2.0f - 0.25f) {  // 오른쪽 벽에 닿았다면
				player.SetPosition(2.0f, 4.0f - 0.25f);
				player.SetCurrentSurface(1);
			}
			break;

		case 3:  // 왼면이 밑면일 때
			player.SetPosition(player.GetXPosition(), player.GetYPosition() - player.GetXDir() * (float)elapsedTime);
			if (player.GetYPosition() < 0.0f + 0.25f) {  // 아래쪽 벽에 닿았다면
				player.SetPosition(-2.0f + 0.25f, 0.0f);
				player.SetCurrentSurface(0);
			}
			else if (player.GetYPosition() > 4.0f - 0.25f) {  // 위쪽 벽에 닿았다면
				player.SetPosition(-2.0f + 0.25f, 4.0f);
				player.SetCurrentSurface(2);
			}
			break;

		default:
			break;
		}
	}
}

void MoveObstacle(std::array<Object, 10>* Obstacles, double elapsedTime) {
	for (auto& obstacle : *Obstacles) {
		if (obstacle.GetXPosition() + obstacle.GetXDir() * elapsedTime < -2.0f + 0.25f || obstacle.GetXPosition() + obstacle.GetXDir() * elapsedTime > 2.0f - 0.25f) {  // 왼쪽, 오른쪽 벽에 닿았다면
			obstacle.SetDir(-2.0f * obstacle.GetXDir(), 0.0f, obstacle.GetZDir());  // yz 평면에 대해 반사
		}

		if (obstacle.GetYPosition() + obstacle.GetYDir() * elapsedTime < -2.0f + 0.25f || obstacle.GetYPosition() + obstacle.GetYDir() * elapsedTime > 2.0f - 0.25f) {  // 위쪽, 아래쪽 벽에 닿았다면
			obstacle.SetDir(0.0f, -2.0f * obstacle.GetYDir(), obstacle.GetZDir());  // xz 평면에 대해 반사
		}

		// 장애물 이동
		obstacle.SetPosition(obstacle.GetXPosition() + obstacle.GetXDir() * (float)elapsedTime, obstacle.GetYPosition() + obstacle.GetYDir() * (float)elapsedTime);
		obstacle.SetZPosition(obstacle.GetZPosition() + obstacle.GetZDir() * (float)elapsedTime);

		if (obstacle.GetZPosition() > 0.0f) {  // 플레이어를 지나갔다면
			obstacle.SetZPosition(-100.0f + obstacle.GetZPosition());  // 재배치
		}
	}
}

void CheckCollision(std::array<Object, 4>* ClientInfo, std::array<Object, 10>* Obstacles) {
	for (auto& player : *ClientInfo) {
		if (!player.GetValidBit()) { continue; }
		if (!player.GetSurvivingBit()) { continue; }

		// 현재 바닥에 따른 플레이어 위치 보정
		float playerX, playerY;

		switch (player.GetCurrentSurface()) {
		case 0:  // 아랫면이 밑면일 때
			playerX = player.GetXPosition();
			playerY = player.GetYPosition() + 0.25f;
			break;

		case 1:  // 오른면이 밑면일 때
			playerX = player.GetXPosition() - 0.25f;
			playerY = player.GetYPosition();
			break;

		case 2:  // 윗면이 밑면일 때
			playerX = player.GetXPosition();
			playerY = player.GetYPosition() - 0.25f;
			break;

		case 3:  // 왼면이 밑면일 때
			playerX = player.GetXPosition() + 0.25f;
			playerY = player.GetYPosition();
			break;

		default:
			break;
		}

		for (auto& obstacle : *Obstacles) {
			// 플레이어와 장애물의 거리 계산을 통한 충돌 검사
			if (sqrt((playerX - obstacle.GetXPosition()) * (playerX - obstacle.GetXPosition()) + 
				     (playerY - obstacle.GetYPosition()) * (playerY - obstacle.GetYPosition()) + 
				     (-1.0f - obstacle.GetZPosition()) * (-1.0f - obstacle.GetZPosition())) < 0.5f) {  // 플레이어와 장애물 간의 거리가 0.5f 미만이라면
				player.SetSurvivingBit(0);
			}
		}
	}
}

void RenewalServerClientArray(std::array<Packet, 4>* ServerClientArray, std::array<Vertex, 10>* ObstacleArray, std::array<Object, 4>* ClientInfo, std::array<Object, 10>* Obstacles, CRITICAL_SECTION* ServerClientArray_CS) {
	EnterCriticalSection(ServerClientArray_CS);

	// 플레이어 갱신
	for (int i = 0; i < 4; ++i) {
		if ((*ClientInfo)[i].GetValidBit()) {
			(*ServerClientArray)[i].SetItemBit((*ClientInfo)[i].GetItemBit());
			(*ServerClientArray)[i].SetAplliedBit((*ClientInfo)[i].GetAppliedBit());
			(*ServerClientArray)[i].SetSurvivingBit((*ClientInfo)[i].GetSurvivingBit());
			(*ServerClientArray)[i].SetPosition((*ClientInfo)[i].GetXPosition(), (*ClientInfo)[i].GetYPosition());
			(*ServerClientArray)[i].SetCurrentSurface((*ClientInfo)[i].GetCurrentSurface());
		}
	}

	// 장애물 갱신
	for (int i = 0; i < 10; ++i) {
		(*ObstacleArray)[i].SetPosition((*Obstacles)[i].GetXPosition(), (*Obstacles)[i].GetYPosition(), (*Obstacles)[i].GetZPosition());
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

	Init(((ThreadArg*)arg)->GetClientInfoArray(), ((ThreadArg*)arg)->GetServerClientArray(), &Players, &Obstacles, ((ThreadArg*)arg)->GetServerClientArrayCS(), ((ThreadArg*)arg)->GetGameStartOrNot());

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

		// 클리아인트, 장애물 이동 
		MovePlayer(&Players, elapsedTime);
		MoveObstacle(&Obstacles, elapsedTime);

		// 충돌 검사
		CheckCollision(&Players, &Obstacles);

		// totalElapsedTime이 60분의 1초를 경과했을 시 ServerClientArray 갱신
		if (totalElapsedTime.count() >= 16'667) {
			RenewalServerClientArray(((ThreadArg*)arg)->GetServerClientArray(), ((ThreadArg*)arg)->GetObstacleArray(), &Players, &Obstacles, ((ThreadArg*)arg)->GetServerClientArrayCS());

			totalElapsedTime = std::chrono::microseconds(0);  // totalElapsedTime를 0초로 갱신
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // CPU 사용량 감소
	}

	return 0;
}
