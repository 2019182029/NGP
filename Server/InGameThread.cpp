#include "Common.h"
#include "InGameThread.h"

std::default_random_engine dre(std::random_device{}());
std::uniform_int_distribution<int> uid(-15, 15);
std::uniform_int_distribution<int> uidZDir(10, 15);
std::uniform_int_distribution<int> uidItem(0, 9);

void Init(std::array<Packet, 4>* ClientInfoArray, std::array<Packet, 4>* ServerClientArray, std::array<Object, 4>* ClientInfo, std::array<Object, 10>* Obstacles, std::array<Position, 10>* ObstacleArray, CRITICAL_SECTION* ServerClientArray_CS, volatile bool* isGameStart) {
	EnterCriticalSection(ServerClientArray_CS);
	
	for (int i = 0; i < 4; ++i) {
		// ServerClientArray 초기화
		if (!(*ClientInfoArray)[i].GetValidBit()) { continue; }

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

	LeaveCriticalSection(ServerClientArray_CS);

	// 장애물 초기화
	for (int i = 0; i < 10; ++i) {
		// 위치 
		(*Obstacles)[i].SetPosition(0.0f, 2.0f);
		(*Obstacles)[i].SetZPosition(-100.0f - 10.0f * i);

		// 방향
		(*Obstacles)[i].SetDir(uid(dre) / 10.0f, uid(dre) / 10.0f, (float)uidZDir(dre));
	}

	// 아이템
	(*ObstacleArray)[uidItem(dre)].SetItem(true);

	*isGameStart = true;

	std::cout << "게임 시작" << std::endl << std::endl;
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
	case 0b1000:  // W 키
		if (packet.GetKeyState() & 0b1000) {  // W 키가 눌렸다면
			switch ((*ClientInfo)[packet.GetPlayerNumber()].GetCurrentSurface()) {  // 점프할 수 있다면
			case 0:
				if ((*ClientInfo)[packet.GetPlayerNumber()].GetYPosition() == 0.0f) { (*ClientInfo)[packet.GetPlayerNumber()].SetDir(0.0f, 2.0f); }
				break;

			case 1:
				if ((*ClientInfo)[packet.GetPlayerNumber()].GetXPosition() == 2.0f) { (*ClientInfo)[packet.GetPlayerNumber()].SetDir(0.0f, 2.0f); }
				break;

			case 2:
				if ((*ClientInfo)[packet.GetPlayerNumber()].GetYPosition() == 4.0f) { (*ClientInfo)[packet.GetPlayerNumber()].SetDir(0.0f, 2.0f); }
				break;

			case 3:
				if ((*ClientInfo)[packet.GetPlayerNumber()].GetXPosition() == -2.0f) { (*ClientInfo)[packet.GetPlayerNumber()].SetDir(0.0f, 2.0f); }
				break;

			default:
				break;
			}
		}
		break;

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
						(*ClientInfo)[i].SetAplliedBit(true);  // 아이템 효과 적용
						(*ClientInfo)[i].SetElapsedTime(std::chrono::high_resolution_clock::now());  // 아이템 효과 적용 시간 갱신
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

Position ModifyPlayerPosition(Object& player) {
	Position PlayerPosition;

	switch (player.GetCurrentSurface()) {
	case 0:  // 아랫면이 밑면일 때
		PlayerPosition.SetPosition(player.GetXPosition(), player.GetYPosition() + 0.25f, 0.0f);
		break;

	case 1:  // 오른면이 밑면일 때
		PlayerPosition.SetPosition(player.GetXPosition() - 0.25f, player.GetYPosition(), 0.0f);
		break;

	case 2:  // 윗면이 밑면일 때
		PlayerPosition.SetPosition(player.GetXPosition(), player.GetYPosition() - 0.25f, 0.0f);
		break;

	case 3:  // 왼면이 밑면일 때
		PlayerPosition.SetPosition(player.GetXPosition() + 0.25f, player.GetYPosition(), 0.0f);
		break;

	default:
		break;
	}

	return PlayerPosition;
}

void MovePlayer(std::array<Object, 4>* ClientInfo, double elapsedTime) {
	Position currentPosition, nextPosition;

	for (auto& player : *ClientInfo) {
		if (!player.GetValidBit()) { continue; }
		if (!player.GetSurvivingBit()) { continue; }

		// 플레이어 위치, 속도 보정
		currentPosition = ModifyPlayerPosition(player);
		player.SetDir(0.0f, -2.0f * (float)elapsedTime);

		// 플레이어 다음 위치
		switch (player.GetCurrentSurface()) {
		case 0:  // 아랫면이 밑면일 때
			nextPosition.SetPosition(currentPosition.GetXPosition() + player.GetXDir() * (float)elapsedTime, currentPosition.GetYPosition() + player.GetYDir() * (float)elapsedTime, 0.0f);
			if (nextPosition.GetYPosition() < 0.0f + 0.25f) {
				nextPosition.SetPosition(nextPosition.GetXPosition(), 0.0f + 0.25f, 0.0f);
				player.SetDir(0.0f, -1.0f * player.GetYDir());
			}
			break;

		case 1:  // 오른면이 밑면일 때
			nextPosition.SetPosition(currentPosition.GetXPosition() - player.GetYDir() * (float)elapsedTime, currentPosition.GetYPosition() + player.GetXDir() * (float)elapsedTime, 0.0f);
			if (nextPosition.GetXPosition() > 2.0f - 0.25f) {
				nextPosition.SetPosition(2.0f - 0.25f, nextPosition.GetYPosition(), 0.0f);
				player.SetDir(0.0f, -1.0f * player.GetYDir());
			}break;

		case 2:  // 윗면이 밑면일 때
			nextPosition.SetPosition(currentPosition.GetXPosition() - player.GetXDir() * (float)elapsedTime, currentPosition.GetYPosition() - player.GetYDir() * (float)elapsedTime, 0.0f);
			if (nextPosition.GetYPosition() > 4.0f - 0.25f) {
				nextPosition.SetPosition(nextPosition.GetXPosition(), 4.0f - 0.25f, 0.0f);
				player.SetDir(0.0f, -1.0f * player.GetYDir());
			}break;

		case 3:  // 왼면이 밑면일 때
			nextPosition.SetPosition(currentPosition.GetXPosition() + player.GetYDir() * (float)elapsedTime, currentPosition.GetYPosition() - player.GetXDir() * (float)elapsedTime, 0.0f);
			if (nextPosition.GetXPosition() < -2.0f + 0.25f) {
				nextPosition.SetPosition(-2.0f + 0.25f, nextPosition.GetYPosition(), 0.0f);
				player.SetDir(0.0f, -1.0f * player.GetYDir());
			}break;

		default:
			break;
		}

		// 플레이어 간의 충돌 검사
		for (int i = 0; i < 4; ++i) {
			if (i == player.GetPlayerNumber()) { continue; }

			if (!(*ClientInfo)[i].GetValidBit()) { continue; }
			if (!(*ClientInfo)[i].GetSurvivingBit()) { continue; }

			// AABB 충돌 검사
			if ((nextPosition.GetXPosition() + 0.25f > ModifyPlayerPosition((*ClientInfo)[i]).GetXPosition() - 0.25f &&
				 nextPosition.GetXPosition() - 0.25f < ModifyPlayerPosition((*ClientInfo)[i]).GetXPosition() + 0.25f) 
				&& (nextPosition.GetYPosition() + 0.25f > ModifyPlayerPosition((*ClientInfo)[i]).GetYPosition() - 0.25f &&
				    nextPosition.GetYPosition() - 0.25f < ModifyPlayerPosition((*ClientInfo)[i]).GetYPosition() + 0.25f)) {

					// x값이 변화하여 충돌이 발생한 거라면
					if (currentPosition.GetYPosition() + 0.25f > ModifyPlayerPosition((*ClientInfo)[i]).GetYPosition() - 0.25f &&
						currentPosition.GetYPosition() - 0.25f < ModifyPlayerPosition((*ClientInfo)[i]).GetYPosition() + 0.25f) { 

						switch (player.GetCurrentSurface()) {
						case 1:
							if (currentPosition.GetXPosition() + 0.25f < ModifyPlayerPosition((*ClientInfo)[i]).GetXPosition() - 0.25f) {  // 다른 플레이어를 밟았다면 
								(*ClientInfo)[i].SetSurvivingBit(0);
								std::cout << "플레이어 " << i << "번 사망" << std::endl;
							}
							else {
								nextPosition.SetPosition(currentPosition.GetXPosition(), nextPosition.GetYPosition(), 0.0f);
							}
							break;

						case 3:
							if (currentPosition.GetXPosition() - 0.25f > ModifyPlayerPosition((*ClientInfo)[i]).GetXPosition() + 0.25f) {  // 다른 플레이어를 밟았다면 
								(*ClientInfo)[i].SetSurvivingBit(0);
								std::cout << "플레이어 " << i << "번 사망" << std::endl;
							}
							else {
								nextPosition.SetPosition(currentPosition.GetXPosition(), nextPosition.GetYPosition(), 0.0f);
							}
							break;

						default:
							nextPosition.SetPosition(currentPosition.GetXPosition(), nextPosition.GetYPosition(), 0.0f);
							break;
						}
					}

					// y값이 변화하여 충돌이 발생한 거라면
					if (currentPosition.GetXPosition() + 0.25f > ModifyPlayerPosition((*ClientInfo)[i]).GetXPosition() - 0.25f &&
						currentPosition.GetXPosition() - 0.25f < ModifyPlayerPosition((*ClientInfo)[i]).GetXPosition() + 0.25f) {

						switch (player.GetCurrentSurface()) {
						case 0:
							if (currentPosition.GetYPosition() - 0.25f > ModifyPlayerPosition((*ClientInfo)[i]).GetYPosition() + 0.25f) {  // 다른 플레이어를 밟았다면 
								(*ClientInfo)[i].SetSurvivingBit(0);
								std::cout << "플레이어 " << i << "번 사망" << std::endl;
							}
							else {
								nextPosition.SetPosition(currentPosition.GetXPosition(), nextPosition.GetYPosition(), 0.0f);
							}
							break;

						case 3:
							if (currentPosition.GetYPosition() + 0.25f < ModifyPlayerPosition((*ClientInfo)[i]).GetYPosition() - 0.25f) {  // 다른 플레이어를 밟았다면 
								(*ClientInfo)[i].SetSurvivingBit(0);
								std::cout << "플레이어 " << i << "번 사망" << std::endl;
							}
							else {
								nextPosition.SetPosition(currentPosition.GetXPosition(), nextPosition.GetYPosition(), 0.0f);
							}
							break;

						default:
							nextPosition.SetPosition(nextPosition.GetXPosition(), currentPosition.GetYPosition(), 0.0f);
							break;
						}
					}
			}
		}

		// 플레이어와 벽 간의 충돌 검사
		switch (player.GetCurrentSurface()) {
		case 0:  // 아랫면이 밑면일 때
			player.SetPosition(nextPosition.GetXPosition(), nextPosition.GetYPosition() - 0.25f);
			if (player.GetXPosition() < -2.0f + 0.25f) {  // 왼쪽 벽에 닿았다면
				player.SetPosition(-2.0f, nextPosition.GetYPosition());
				player.SetCurrentSurface(3);
				player.SetDir(0.0f, -1.0f * player.GetYDir());
			}
			else if (player.GetXPosition() > 2.0f - 0.25f) {  // 오른쪽 벽에 닿았다면
				player.SetPosition(2.0f, nextPosition.GetYPosition());
				player.SetCurrentSurface(1);
				player.SetDir(0.0f, -1.0f * player.GetYDir());
			}
			break;

		case 1:  // 오른면이 밑면일 때
			player.SetPosition(nextPosition.GetXPosition() + 0.25f, nextPosition.GetYPosition());
			if (player.GetYPosition() < 0.0f + 0.25f) {  // 아래쪽 벽에 닿았다면
				player.SetPosition(nextPosition.GetXPosition(), 0.0f);
				player.SetCurrentSurface(0);
				player.SetDir(0.0f, -1.0f * player.GetYDir());
			}
			else if (player.GetYPosition() > 4.0f - 0.25f) {  // 위쪽 벽에 닿았다면
				player.SetPosition(nextPosition.GetXPosition(), 4.0f);
				player.SetCurrentSurface(2);
				player.SetDir(0.0f, -1.0f * player.GetYDir());
			}
			break;

		case 2:  // 윗면이 밑면일 때
			player.SetPosition(nextPosition.GetXPosition(), nextPosition.GetYPosition() + 0.25f);
			if (player.GetXPosition() < -2.0f + 0.25f) {  // 왼쪽 벽에 닿았다면
				player.SetPosition(-2.0f, nextPosition.GetYPosition());
				player.SetCurrentSurface(3);
				player.SetDir(0.0f, -1.0f * player.GetYDir());
			}
			else if (player.GetXPosition() > 2.0f - 0.25f) {  // 오른쪽 벽에 닿았다면
				player.SetPosition(2.0f, nextPosition.GetYPosition());
				player.SetCurrentSurface(1);
				player.SetDir(0.0f, -1.0f * player.GetYDir());
			}
			break;

		case 3:  // 왼면이 밑면일 때
			player.SetPosition(nextPosition.GetXPosition() - 0.25f, nextPosition.GetYPosition());
			if (player.GetYPosition() < 0.0f + 0.25f) {  // 아래쪽 벽에 닿았다면
				player.SetPosition(nextPosition.GetXPosition(), 0.0f);
				player.SetCurrentSurface(0);
				player.SetDir(0.0f, -1.0f * player.GetYDir());
			}
			else if (player.GetYPosition() > 4.0f - 0.25f) {  // 위쪽 벽에 닿았다면
				player.SetPosition(nextPosition.GetXPosition(), 4.0f);
				player.SetCurrentSurface(2);
				player.SetDir(0.0f, -1.0f * player.GetYDir());
			}
			break;

		default:
			break;
		}

		if (player.GetAppliedBit()) {  // 아이템 효과가 적용된 플레이어일 경우
			if (((std::chrono::duration<double>)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - player.GetElapsedTime())).count() > 2.5) {  // 아이템 효과 적용 이후 2.5초 경과 시
				player.SetAplliedBit(0);  // 아이템 효과 제거
			}
		}
	}
}

void MoveObstacle(std::array<Object, 10>* Obstacles, double elapsedTime) {
	for (auto& obstacle : *Obstacles) {
		if (obstacle.GetXPosition() + obstacle.GetXDir() * elapsedTime < -2.0f + 0.25f || obstacle.GetXPosition() + obstacle.GetXDir() * elapsedTime > 2.0f - 0.25f) {  // 왼쪽, 오른쪽 벽에 닿았다면
			obstacle.SetDir(-2.0f * obstacle.GetXDir(), 0.0f, obstacle.GetZDir());  // yz 평면에 대해 반사
		}

		if (obstacle.GetYPosition() + obstacle.GetYDir() * elapsedTime < 0.0f + 0.25f || obstacle.GetYPosition() + obstacle.GetYDir() * elapsedTime > 4.0f - 0.25f) {  // 위쪽, 아래쪽 벽에 닿았다면
			obstacle.SetDir(0.0f, -2.0f * obstacle.GetYDir(), obstacle.GetZDir());  // xz 평면에 대해 반사
		}

		// 장애물 이동
		obstacle.SetPosition(obstacle.GetXPosition() + obstacle.GetXDir() * (float)elapsedTime, obstacle.GetYPosition() + obstacle.GetYDir() * (float)elapsedTime);
		obstacle.SetZPosition(obstacle.GetZPosition() + obstacle.GetZDir() * (float)elapsedTime);

		if (obstacle.GetZPosition() > 10.0f) {  // 플레이어를 지나갔다면
			obstacle.SetZPosition(-100.0f);  // 재배치
		}
	}
}

void CheckPlayerObjectCollision(std::array<Object, 4>* ClientInfo, std::array<Object, 10>* Obstacles, std::array<Position, 10>* ObstacleArray) {
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

		for (int i = 0; i < 10; ++i) {
			// 플레이어와 장애물 간의 거리 계산을 통한 충돌 검사
			if (sqrt((playerX - (*Obstacles)[i].GetXPosition()) * (playerX - (*Obstacles)[i].GetXPosition()) +
				     (playerY - (*Obstacles)[i].GetYPosition()) * (playerY - (*Obstacles)[i].GetYPosition()) +
				     (-1.0f - (*Obstacles)[i].GetZPosition()) * (-1.0f - (*Obstacles)[i].GetZPosition())) < ((sqrt(2) + 1.0f) / 4.0f)) {  // 플레이어와 장애물 간의 거리가 (sqrt(2) + 1.0f) / 4.0f 미만(최단 거리와 최장 거리의 중간값)이라면
				if ((*ObstacleArray)[i].GetItem()) {  // 장애물이 아이템이라면
					player.SetItemBit(1);
				}
				else {
					player.SetSurvivingBit(0);

					std::cout << "플레이어 " << player.GetPlayerNumber() << "번 사망" << std::endl;
				}
			}
		}
	}
}

void RenewalServerClientArray(std::array<Packet, 4>* ServerClientArray, std::array<Position, 10>* ObstacleArray, std::array<Object, 4>* ClientInfo, std::array<Object, 10>* Obstacles, CRITICAL_SECTION* ServerClientArray_CS) {
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
	auto endTime = std::chrono::high_resolution_clock::now();

	Init(((ThreadArg*)arg)->GetClientInfoArray(), ((ThreadArg*)arg)->GetServerClientArray(), &Players, &Obstacles, ((ThreadArg*)arg)->GetObstacleArray(), ((ThreadArg*)arg)->GetServerClientArrayCS(), ((ThreadArg*)arg)->GetGameStartOrNot());

	// 게임 루프
	while (1) {
		// ClientServerQueue가 비어있지 않다면 ClientInfo 갱신
		PopFromClientServerQueue(((ThreadArg*)arg)->GetClientServerQueue(), &Players, ((ThreadArg*)arg)->GetClientServerQueueCS());
		
		// 모든 플레이어가 사망했다면 프로그램 종료
		if (!std::count_if(Players.begin(), Players.end(), [](auto& packet) { return packet.GetSurvivingBit() == 1; })) {
			RenewalServerClientArray(((ThreadArg*)arg)->GetServerClientArray(), ((ThreadArg*)arg)->GetObstacleArray(), &Players, &Obstacles, ((ThreadArg*)arg)->GetServerClientArrayCS());
			endTime = std::chrono::high_resolution_clock::now();
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
		CheckPlayerObjectCollision(&Players, &Obstacles, ((ThreadArg*)arg)->GetObstacleArray());

		// totalElapsedTime이 60분의 1초를 경과했을 시 ServerClientArray 갱신
		if (totalElapsedTime.count() >= 16'667) {
			RenewalServerClientArray(((ThreadArg*)arg)->GetServerClientArray(), ((ThreadArg*)arg)->GetObstacleArray(), &Players, &Obstacles, ((ThreadArg*)arg)->GetServerClientArrayCS());

			totalElapsedTime = std::chrono::microseconds(0);  // totalElapsedTime를 0초로 갱신
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // CPU 사용량 감소
	}

	std::cout << std::endl << "전원 사망, 게임 종료" << std::endl;

	while (1) {
		if (((std::chrono::duration<double>)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - endTime)).count() > 5.0) {
			((ThreadArg*)arg)->SetGameStartOrNot(false);
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	return 0;
}
