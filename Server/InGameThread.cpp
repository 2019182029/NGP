#include "Common.h"
#include "InGameThread.h"

std::default_random_engine dre(std::random_device{}());
std::uniform_int_distribution<int> uid(-15, 15);
std::uniform_int_distribution<int> uidZDir(10, 15);
std::uniform_int_distribution<int> uidItem(0, 9);

void Init(std::array<Packet, 4>* ClientInfoArray, std::array<Packet, 4>* ServerClientArray, std::array<Object, 4>* ClientInfo, std::array<Object, 10>* Obstacles, std::array<Position, 10>* ObstacleArray, CRITICAL_SECTION* ServerClientArray_CS, volatile bool* isGameStart) {
	EnterCriticalSection(ServerClientArray_CS);
	
	for (int i = 0; i < 4; ++i) {
		// ServerClientArray �ʱ�ȭ
		if (!(*ClientInfoArray)[i].GetValidBit()) { continue; }

		(*ClientInfo)[i].SetValidBit(1);  (*ServerClientArray)[i].SetValidBit(1);
		(*ClientInfo)[i].SetPlayerNumber(i);  (*ServerClientArray)[i].SetPlayerNumber(i);
		(*ClientInfo)[i].SetItemBit(0);  (*ServerClientArray)[i].SetItemBit(0);
		(*ClientInfo)[i].SetAplliedBit(0);  (*ServerClientArray)[i].SetAplliedBit(0);
		(*ClientInfo)[i].SetSurvivingBit(1);  (*ServerClientArray)[i].SetSurvivingBit(1);
		(*ClientInfo)[i].SetKeyState(0b0000); 
		(*ServerClientArray)[i].SetSeed(0);

		switch (i) {
		case 0:  // 0�� �÷��̾�
			(*ClientInfo)[i].SetPosition(0.0f, 0.0f);  (*ServerClientArray)[i].SetPosition(0.0f, 0.0f);
			(*ClientInfo)[i].SetCurrentSurface(0);  (*ServerClientArray)[i].SetCurrentSurface(0);  // �Ʒ���
			break;

		case 1:  // 1�� �÷��̾�
			(*ClientInfo)[i].SetPosition(2.0f, 2.0f);  (*ServerClientArray)[i].SetPosition(2.0f, 2.0f);
			(*ClientInfo)[i].SetCurrentSurface(1);  (*ServerClientArray)[i].SetCurrentSurface(1);  // ������
			break;

		case 2:  // 2�� �÷��̾�
			(*ClientInfo)[i].SetPosition(0.0f, 4.0f);  (*ServerClientArray)[i].SetPosition(0.0f, 4.0f);
			(*ClientInfo)[i].SetCurrentSurface(2);  (*ServerClientArray)[i].SetCurrentSurface(2);  // ����
			break;

		case 3:  // 3�� �÷��̾�
			(*ClientInfo)[i].SetPosition(-2.0f, 2.0f);  (*ServerClientArray)[i].SetPosition(-2.0f, 2.0f);
			(*ClientInfo)[i].SetCurrentSurface(3);  (*ServerClientArray)[i].SetCurrentSurface(3);  // �Ʒ���
			break;

		default:
			break;
		}
	}

	LeaveCriticalSection(ServerClientArray_CS);

	// ��ֹ� �ʱ�ȭ
	for (int i = 0; i < 10; ++i) {
		// ��ġ 
		(*Obstacles)[i].SetPosition(0.0f, 2.0f);
		(*Obstacles)[i].SetZPosition(-100.0f - 10.0f * i);

		// ����
		(*Obstacles)[i].SetDir(uid(dre) / 10.0f, uid(dre) / 10.0f, (float)uidZDir(dre));
	}

	// ������
	(*ObstacleArray)[uidItem(dre)].SetItem(true);

	*isGameStart = true;

	std::cout << "���� ����" << std::endl << std::endl;
}

void PopFromClientServerQueue(std::queue<Packet>* ClientServerQueue, std::array<Object, 4>* ClientInfo, CRITICAL_SECTION* ClientServerQueue_CS) {
	if (!ClientServerQueue->empty()) {
		EnterCriticalSection(ClientServerQueue_CS);

		Update(ClientServerQueue->front(), ClientInfo);  // ť�� ù ��° ��Ҹ� �����ͼ� �÷��̾� ���� ����
		ClientServerQueue->pop();  // ť�� ù ��° ��� ����

		LeaveCriticalSection(ClientServerQueue_CS);
	}
}

void Update(Packet packet, std::array<Object, 4>* ClientInfo) {
	switch (packet.GetKeyState() ^ (*ClientInfo)[packet.GetPlayerNumber()].GetKeyState()) {  // � Ű�� �����ų� �������°�?
	case 0b1000:  // W Ű
		if (packet.GetKeyState() & 0b1000) {  // W Ű�� ���ȴٸ�
			switch ((*ClientInfo)[packet.GetPlayerNumber()].GetCurrentSurface()) {  // ������ �� �ִٸ�
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

	case 0b0100:  // A Ű
		if (packet.GetKeyState() & 0b0100) {  // A Ű�� ���ȴٸ�
			(*ClientInfo)[packet.GetPlayerNumber()].SetDir(-1.0f, 0.0f); 
		}
		else {  // A Ű�� �������ٸ�
			(*ClientInfo)[packet.GetPlayerNumber()].SetDir(1.0f, 0.0f); 
		}
		break;

	case 0b0010:  // D Ű
		if (packet.GetKeyState() & 0b0010) {  // D Ű�� ���ȴٸ�
			(*ClientInfo)[packet.GetPlayerNumber()].SetDir(1.0f, 0.0f); 
		}
		else {  // D Ű�� �������ٸ�
			(*ClientInfo)[packet.GetPlayerNumber()].SetDir(-1.0f, 0.0f); 
		}
		break;

	case 0b0001:  // C Ű
		if (packet.GetKeyState() & 0b0001) {  // C Ű�� ���ȴٸ�
			if ((*ClientInfo)[packet.GetPlayerNumber()].GetItemBit()) {  // �ش� �÷��̾ �������� ������ �ִٸ�
				for (int i = 0; i < 4; ++i) {
					if (!(*ClientInfo)[i].GetValidBit()) { continue; }
					if (!(*ClientInfo)[i].GetSurvivingBit()) { continue; }

					if (i != packet.GetPlayerNumber()) {  // �ش� �÷��̾ ������ ������ ��� �÷��̾��
						(*ClientInfo)[i].SetAplliedBit(true);  // ������ ȿ�� ����
						(*ClientInfo)[i].SetElapsedTime(std::chrono::high_resolution_clock::now());  // ������ ȿ�� ���� �ð� ����
					}
				}
				(*ClientInfo)[packet.GetPlayerNumber()].SetItemBit(0);  // �ش� �÷��̾��� ������ ���� ���θ� false�� ����
			}
		}
		break;

	default: 
		break;
	}

	(*ClientInfo)[packet.GetPlayerNumber()].SetKeyState(packet.GetKeyState());  // ClientInfo�� KeyState ����
}

Position ModifyPlayerPosition(Object& player) {
	Position PlayerPosition;

	switch (player.GetCurrentSurface()) {
	case 0:  // �Ʒ����� �ظ��� ��
		PlayerPosition.SetPosition(player.GetXPosition(), player.GetYPosition() + 0.25f, 0.0f);
		break;

	case 1:  // �������� �ظ��� ��
		PlayerPosition.SetPosition(player.GetXPosition() - 0.25f, player.GetYPosition(), 0.0f);
		break;

	case 2:  // ������ �ظ��� ��
		PlayerPosition.SetPosition(player.GetXPosition(), player.GetYPosition() - 0.25f, 0.0f);
		break;

	case 3:  // �޸��� �ظ��� ��
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

		// �÷��̾� ��ġ, �ӵ� ����
		currentPosition = ModifyPlayerPosition(player);
		player.SetDir(0.0f, -2.0f * (float)elapsedTime);

		// �÷��̾� ���� ��ġ
		switch (player.GetCurrentSurface()) {
		case 0:  // �Ʒ����� �ظ��� ��
			nextPosition.SetPosition(currentPosition.GetXPosition() + player.GetXDir() * (float)elapsedTime, currentPosition.GetYPosition() + player.GetYDir() * (float)elapsedTime, 0.0f);
			if (nextPosition.GetYPosition() < 0.0f + 0.25f) {
				nextPosition.SetPosition(nextPosition.GetXPosition(), 0.0f + 0.25f, 0.0f);
				player.SetDir(0.0f, -1.0f * player.GetYDir());
			}
			break;

		case 1:  // �������� �ظ��� ��
			nextPosition.SetPosition(currentPosition.GetXPosition() - player.GetYDir() * (float)elapsedTime, currentPosition.GetYPosition() + player.GetXDir() * (float)elapsedTime, 0.0f);
			if (nextPosition.GetXPosition() > 2.0f - 0.25f) {
				nextPosition.SetPosition(2.0f - 0.25f, nextPosition.GetYPosition(), 0.0f);
				player.SetDir(0.0f, -1.0f * player.GetYDir());
			}break;

		case 2:  // ������ �ظ��� ��
			nextPosition.SetPosition(currentPosition.GetXPosition() - player.GetXDir() * (float)elapsedTime, currentPosition.GetYPosition() - player.GetYDir() * (float)elapsedTime, 0.0f);
			if (nextPosition.GetYPosition() > 4.0f - 0.25f) {
				nextPosition.SetPosition(nextPosition.GetXPosition(), 4.0f - 0.25f, 0.0f);
				player.SetDir(0.0f, -1.0f * player.GetYDir());
			}break;

		case 3:  // �޸��� �ظ��� ��
			nextPosition.SetPosition(currentPosition.GetXPosition() + player.GetYDir() * (float)elapsedTime, currentPosition.GetYPosition() - player.GetXDir() * (float)elapsedTime, 0.0f);
			if (nextPosition.GetXPosition() < -2.0f + 0.25f) {
				nextPosition.SetPosition(-2.0f + 0.25f, nextPosition.GetYPosition(), 0.0f);
				player.SetDir(0.0f, -1.0f * player.GetYDir());
			}break;

		default:
			break;
		}

		// �÷��̾� ���� �浹 �˻�
		for (int i = 0; i < 4; ++i) {
			if (i == player.GetPlayerNumber()) { continue; }

			if (!(*ClientInfo)[i].GetValidBit()) { continue; }
			if (!(*ClientInfo)[i].GetSurvivingBit()) { continue; }

			// AABB �浹 �˻�
			if ((nextPosition.GetXPosition() + 0.25f > ModifyPlayerPosition((*ClientInfo)[i]).GetXPosition() - 0.25f &&
				 nextPosition.GetXPosition() - 0.25f < ModifyPlayerPosition((*ClientInfo)[i]).GetXPosition() + 0.25f) 
				&& (nextPosition.GetYPosition() + 0.25f > ModifyPlayerPosition((*ClientInfo)[i]).GetYPosition() - 0.25f &&
				    nextPosition.GetYPosition() - 0.25f < ModifyPlayerPosition((*ClientInfo)[i]).GetYPosition() + 0.25f)) {

					// x���� ��ȭ�Ͽ� �浹�� �߻��� �Ŷ��
					if (currentPosition.GetYPosition() + 0.25f > ModifyPlayerPosition((*ClientInfo)[i]).GetYPosition() - 0.25f &&
						currentPosition.GetYPosition() - 0.25f < ModifyPlayerPosition((*ClientInfo)[i]).GetYPosition() + 0.25f) { 

						switch (player.GetCurrentSurface()) {
						case 1:
							if (currentPosition.GetXPosition() + 0.25f < ModifyPlayerPosition((*ClientInfo)[i]).GetXPosition() - 0.25f) {  // �ٸ� �÷��̾ ��Ҵٸ� 
								(*ClientInfo)[i].SetSurvivingBit(0);
								std::cout << "�÷��̾� " << i << "�� ���" << std::endl;
							}
							else {
								nextPosition.SetPosition(currentPosition.GetXPosition(), nextPosition.GetYPosition(), 0.0f);
							}
							break;

						case 3:
							if (currentPosition.GetXPosition() - 0.25f > ModifyPlayerPosition((*ClientInfo)[i]).GetXPosition() + 0.25f) {  // �ٸ� �÷��̾ ��Ҵٸ� 
								(*ClientInfo)[i].SetSurvivingBit(0);
								std::cout << "�÷��̾� " << i << "�� ���" << std::endl;
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

					// y���� ��ȭ�Ͽ� �浹�� �߻��� �Ŷ��
					if (currentPosition.GetXPosition() + 0.25f > ModifyPlayerPosition((*ClientInfo)[i]).GetXPosition() - 0.25f &&
						currentPosition.GetXPosition() - 0.25f < ModifyPlayerPosition((*ClientInfo)[i]).GetXPosition() + 0.25f) {

						switch (player.GetCurrentSurface()) {
						case 0:
							if (currentPosition.GetYPosition() - 0.25f > ModifyPlayerPosition((*ClientInfo)[i]).GetYPosition() + 0.25f) {  // �ٸ� �÷��̾ ��Ҵٸ� 
								(*ClientInfo)[i].SetSurvivingBit(0);
								std::cout << "�÷��̾� " << i << "�� ���" << std::endl;
							}
							else {
								nextPosition.SetPosition(currentPosition.GetXPosition(), nextPosition.GetYPosition(), 0.0f);
							}
							break;

						case 3:
							if (currentPosition.GetYPosition() + 0.25f < ModifyPlayerPosition((*ClientInfo)[i]).GetYPosition() - 0.25f) {  // �ٸ� �÷��̾ ��Ҵٸ� 
								(*ClientInfo)[i].SetSurvivingBit(0);
								std::cout << "�÷��̾� " << i << "�� ���" << std::endl;
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

		// �÷��̾�� �� ���� �浹 �˻�
		switch (player.GetCurrentSurface()) {
		case 0:  // �Ʒ����� �ظ��� ��
			player.SetPosition(nextPosition.GetXPosition(), nextPosition.GetYPosition() - 0.25f);
			if (player.GetXPosition() < -2.0f + 0.25f) {  // ���� ���� ��Ҵٸ�
				player.SetPosition(-2.0f, nextPosition.GetYPosition());
				player.SetCurrentSurface(3);
				player.SetDir(0.0f, -1.0f * player.GetYDir());
			}
			else if (player.GetXPosition() > 2.0f - 0.25f) {  // ������ ���� ��Ҵٸ�
				player.SetPosition(2.0f, nextPosition.GetYPosition());
				player.SetCurrentSurface(1);
				player.SetDir(0.0f, -1.0f * player.GetYDir());
			}
			break;

		case 1:  // �������� �ظ��� ��
			player.SetPosition(nextPosition.GetXPosition() + 0.25f, nextPosition.GetYPosition());
			if (player.GetYPosition() < 0.0f + 0.25f) {  // �Ʒ��� ���� ��Ҵٸ�
				player.SetPosition(nextPosition.GetXPosition(), 0.0f);
				player.SetCurrentSurface(0);
				player.SetDir(0.0f, -1.0f * player.GetYDir());
			}
			else if (player.GetYPosition() > 4.0f - 0.25f) {  // ���� ���� ��Ҵٸ�
				player.SetPosition(nextPosition.GetXPosition(), 4.0f);
				player.SetCurrentSurface(2);
				player.SetDir(0.0f, -1.0f * player.GetYDir());
			}
			break;

		case 2:  // ������ �ظ��� ��
			player.SetPosition(nextPosition.GetXPosition(), nextPosition.GetYPosition() + 0.25f);
			if (player.GetXPosition() < -2.0f + 0.25f) {  // ���� ���� ��Ҵٸ�
				player.SetPosition(-2.0f, nextPosition.GetYPosition());
				player.SetCurrentSurface(3);
				player.SetDir(0.0f, -1.0f * player.GetYDir());
			}
			else if (player.GetXPosition() > 2.0f - 0.25f) {  // ������ ���� ��Ҵٸ�
				player.SetPosition(2.0f, nextPosition.GetYPosition());
				player.SetCurrentSurface(1);
				player.SetDir(0.0f, -1.0f * player.GetYDir());
			}
			break;

		case 3:  // �޸��� �ظ��� ��
			player.SetPosition(nextPosition.GetXPosition() - 0.25f, nextPosition.GetYPosition());
			if (player.GetYPosition() < 0.0f + 0.25f) {  // �Ʒ��� ���� ��Ҵٸ�
				player.SetPosition(nextPosition.GetXPosition(), 0.0f);
				player.SetCurrentSurface(0);
				player.SetDir(0.0f, -1.0f * player.GetYDir());
			}
			else if (player.GetYPosition() > 4.0f - 0.25f) {  // ���� ���� ��Ҵٸ�
				player.SetPosition(nextPosition.GetXPosition(), 4.0f);
				player.SetCurrentSurface(2);
				player.SetDir(0.0f, -1.0f * player.GetYDir());
			}
			break;

		default:
			break;
		}

		if (player.GetAppliedBit()) {  // ������ ȿ���� ����� �÷��̾��� ���
			if (((std::chrono::duration<double>)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - player.GetElapsedTime())).count() > 2.5) {  // ������ ȿ�� ���� ���� 2.5�� ��� ��
				player.SetAplliedBit(0);  // ������ ȿ�� ����
			}
		}
	}
}

void MoveObstacle(std::array<Object, 10>* Obstacles, double elapsedTime) {
	for (auto& obstacle : *Obstacles) {
		if (obstacle.GetXPosition() + obstacle.GetXDir() * elapsedTime < -2.0f + 0.25f || obstacle.GetXPosition() + obstacle.GetXDir() * elapsedTime > 2.0f - 0.25f) {  // ����, ������ ���� ��Ҵٸ�
			obstacle.SetDir(-2.0f * obstacle.GetXDir(), 0.0f, obstacle.GetZDir());  // yz ��鿡 ���� �ݻ�
		}

		if (obstacle.GetYPosition() + obstacle.GetYDir() * elapsedTime < 0.0f + 0.25f || obstacle.GetYPosition() + obstacle.GetYDir() * elapsedTime > 4.0f - 0.25f) {  // ����, �Ʒ��� ���� ��Ҵٸ�
			obstacle.SetDir(0.0f, -2.0f * obstacle.GetYDir(), obstacle.GetZDir());  // xz ��鿡 ���� �ݻ�
		}

		// ��ֹ� �̵�
		obstacle.SetPosition(obstacle.GetXPosition() + obstacle.GetXDir() * (float)elapsedTime, obstacle.GetYPosition() + obstacle.GetYDir() * (float)elapsedTime);
		obstacle.SetZPosition(obstacle.GetZPosition() + obstacle.GetZDir() * (float)elapsedTime);

		if (obstacle.GetZPosition() > 10.0f) {  // �÷��̾ �������ٸ�
			obstacle.SetZPosition(-100.0f);  // ���ġ
		}
	}
}

void CheckPlayerObjectCollision(std::array<Object, 4>* ClientInfo, std::array<Object, 10>* Obstacles, std::array<Position, 10>* ObstacleArray) {
	for (auto& player : *ClientInfo) {
		if (!player.GetValidBit()) { continue; }
		if (!player.GetSurvivingBit()) { continue; }

		// ���� �ٴڿ� ���� �÷��̾� ��ġ ����
		float playerX, playerY;

		switch (player.GetCurrentSurface()) {
		case 0:  // �Ʒ����� �ظ��� ��
			playerX = player.GetXPosition();
			playerY = player.GetYPosition() + 0.25f;
			break;

		case 1:  // �������� �ظ��� ��
			playerX = player.GetXPosition() - 0.25f;
			playerY = player.GetYPosition();
			break;

		case 2:  // ������ �ظ��� ��
			playerX = player.GetXPosition();
			playerY = player.GetYPosition() - 0.25f;
			break;

		case 3:  // �޸��� �ظ��� ��
			playerX = player.GetXPosition() + 0.25f;
			playerY = player.GetYPosition();
			break;

		default:
			break;
		}

		for (int i = 0; i < 10; ++i) {
			// �÷��̾�� ��ֹ� ���� �Ÿ� ����� ���� �浹 �˻�
			if (sqrt((playerX - (*Obstacles)[i].GetXPosition()) * (playerX - (*Obstacles)[i].GetXPosition()) +
				     (playerY - (*Obstacles)[i].GetYPosition()) * (playerY - (*Obstacles)[i].GetYPosition()) +
				     (-1.0f - (*Obstacles)[i].GetZPosition()) * (-1.0f - (*Obstacles)[i].GetZPosition())) < ((sqrt(2) + 1.0f) / 4.0f)) {  // �÷��̾�� ��ֹ� ���� �Ÿ��� (sqrt(2) + 1.0f) / 4.0f �̸�(�ִ� �Ÿ��� ���� �Ÿ��� �߰���)�̶��
				if ((*ObstacleArray)[i].GetItem()) {  // ��ֹ��� �������̶��
					player.SetItemBit(1);
				}
				else {
					player.SetSurvivingBit(0);

					std::cout << "�÷��̾� " << player.GetPlayerNumber() << "�� ���" << std::endl;
				}
			}
		}
	}
}

void RenewalServerClientArray(std::array<Packet, 4>* ServerClientArray, std::array<Position, 10>* ObstacleArray, std::array<Object, 4>* ClientInfo, std::array<Object, 10>* Obstacles, CRITICAL_SECTION* ServerClientArray_CS) {
	EnterCriticalSection(ServerClientArray_CS);

	// �÷��̾� ����
	for (int i = 0; i < 4; ++i) {
		if ((*ClientInfo)[i].GetValidBit()) {
			(*ServerClientArray)[i].SetItemBit((*ClientInfo)[i].GetItemBit());
			(*ServerClientArray)[i].SetAplliedBit((*ClientInfo)[i].GetAppliedBit());
			(*ServerClientArray)[i].SetSurvivingBit((*ClientInfo)[i].GetSurvivingBit());
			(*ServerClientArray)[i].SetPosition((*ClientInfo)[i].GetXPosition(), (*ClientInfo)[i].GetYPosition());
			(*ServerClientArray)[i].SetCurrentSurface((*ClientInfo)[i].GetCurrentSurface());
		}
	}

	// ��ֹ� ����
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

	// ���� ����
	while (1) {
		// ClientServerQueue�� ������� �ʴٸ� ClientInfo ����
		PopFromClientServerQueue(((ThreadArg*)arg)->GetClientServerQueue(), &Players, ((ThreadArg*)arg)->GetClientServerQueueCS());
		
		// ��� �÷��̾ ����ߴٸ� ���α׷� ����
		if (!std::count_if(Players.begin(), Players.end(), [](auto& packet) { return packet.GetSurvivingBit() == 1; })) {
			RenewalServerClientArray(((ThreadArg*)arg)->GetServerClientArray(), ((ThreadArg*)arg)->GetObstacleArray(), &Players, &Obstacles, ((ThreadArg*)arg)->GetServerClientArrayCS());
			endTime = std::chrono::high_resolution_clock::now();
			break;
		}

		currentTime = std::chrono::high_resolution_clock::now();
		elapsedTime = ((std::chrono::duration<double>)std::chrono::duration_cast<std::chrono::microseconds>(currentTime - beforeTime)).count();
		totalElapsedTime += std::chrono::duration_cast<std::chrono::microseconds>(currentTime - beforeTime);
		beforeTime = std::chrono::high_resolution_clock::now();

		// Ŭ������Ʈ, ��ֹ� �̵� 
		MovePlayer(&Players, elapsedTime);
		MoveObstacle(&Obstacles, elapsedTime);

		// �浹 �˻�
		CheckPlayerObjectCollision(&Players, &Obstacles, ((ThreadArg*)arg)->GetObstacleArray());

		// totalElapsedTime�� 60���� 1�ʸ� ������� �� ServerClientArray ����
		if (totalElapsedTime.count() >= 16'667) {
			RenewalServerClientArray(((ThreadArg*)arg)->GetServerClientArray(), ((ThreadArg*)arg)->GetObstacleArray(), &Players, &Obstacles, ((ThreadArg*)arg)->GetServerClientArrayCS());

			totalElapsedTime = std::chrono::microseconds(0);  // totalElapsedTime�� 0�ʷ� ����
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // CPU ��뷮 ����
	}

	std::cout << std::endl << "���� ���, ���� ����" << std::endl;

	while (1) {
		if (((std::chrono::duration<double>)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - endTime)).count() > 5.0) {
			((ThreadArg*)arg)->SetGameStartOrNot(false);
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	return 0;
}
