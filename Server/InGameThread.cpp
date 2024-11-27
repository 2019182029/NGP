#include "Common.h"
#include "InGameThread.h"

void Init(std::array<Packet, 4>* ClientInfoArray, std::array<Packet, 4>* ServerClientArray, std::array<Object, 4>* ClientInfo, CRITICAL_SECTION* ServerClientArray_CS, volatile bool* isGameStart) {
	EnterCriticalSection(ServerClientArray_CS);
	
	for (int i = 0; i < 4; ++i) {
		// ServerClientArray �ʱ�ȭ
		if ((*ClientInfoArray)[i].GetValidBit()) {
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
	}

	LeaveCriticalSection(ServerClientArray_CS);

	*isGameStart = true;

	std::cout << "���� ����" << std::endl;
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
	switch (packet.GetKeyState() ^ (*ClientInfo)[packet.GetPlayerNumber()].GetKeyState()) {
	case 0b0100:  // A Ű
		if (packet.GetKeyState() & 0100) { (*ClientInfo)[packet.GetPlayerNumber()].SetDir(-1.0f, 0.0f); }
		else { (*ClientInfo)[packet.GetPlayerNumber()].SetDir(1.0f, 0.0f); }
		break;

	case 0b0010:  // D Ű
		if (packet.GetKeyState() & 0010) { (*ClientInfo)[packet.GetPlayerNumber()].SetDir(1.0f, 0.0f); }
		else { (*ClientInfo)[packet.GetPlayerNumber()].SetDir(-1.0f, 0.0f); }
		break;

	case 0b0001:  // C Ű
		if (packet.GetKeyState() & 0001) {  
			if ((*ClientInfo)[packet.GetPlayerNumber()].GetItemBit()) {  // �ش� �÷��̾ �������� ������ �ִٸ�
				for (int i = 0; i < 4; ++i) {
					if (i != packet.GetPlayerNumber()) {  // �ش� �÷��̾ ������ ������ ��� �÷��̾��
						(*ClientInfo)[i].SetAplliedBit(true);  // ������ ȿ�� ���� ��
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

	// ���� ����
	while (1) {
		// ClientServerQueue�� ������� �ʴٸ� ClientInfo ����
		PopFromClientServerQueue(((ThreadArg*)arg)->GetClientServerQueue(), &Players, ((ThreadArg*)arg)->GetClientServerQueueCS());

		// ��� �÷��̾ ����ߴٸ� ���α׷� ����
		if (!std::count_if(Players.begin(), Players.end(), [](auto& packet) { return packet.GetSurvivingBit() == 0; })) {
			((ThreadArg*)arg)->SetGameStartOrNot(false);
			break;
		}

		currentTime = std::chrono::high_resolution_clock::now();
		elapsedTime = ((std::chrono::duration<double>)std::chrono::duration_cast<std::chrono::microseconds>(currentTime - beforeTime)).count();
		totalElapsedTime += std::chrono::duration_cast<std::chrono::microseconds>(currentTime - beforeTime);
		beforeTime = std::chrono::high_resolution_clock::now();

		// Ŭ���̾�Ʈ �̵�, �浹 �˻�
		for (auto& ObjectInfo : Players) {
			// Ŭ������Ʈ �̵� 


			// �浹 �˻� : Item, Surviving
		}

		// totalElapsedTime�� 60���� 1�ʸ� ������� �� ServerClientArray ����
		if (totalElapsedTime.count() >= 16'667) {
			RenewalServerClientArray(((ThreadArg*)arg)->GetServerClientArray(), &Players, ((ThreadArg*)arg)->GetServerClientArrayCS());

			totalElapsedTime = std::chrono::microseconds(0);  // totalElapsedTime�� 0�ʷ� ����
		}
	}

	return 0;
}
