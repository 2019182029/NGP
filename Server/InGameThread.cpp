#include "Common.h"
#include "InGameThread.h"

void Init(std::array<Packet, 4>* ClientInfo, std::array<Packet, 4>* ClientInfoArray, std::array<Packet, 4>* ServerClientArray, CRITICAL_SECTION* ServerClientArray_CS, bool* isGameStart) {
	EnterCriticalSection(ServerClientArray_CS);
	
	for (int i = 0; i < 4; ++i) {
		// ServerClientArray �ʱ�ȭ
		if ((*ClientInfoArray)[i].GetValidBit()) {
			(*ClientInfo)[i].SetValidBit(1);  (*ServerClientArray)[i].SetValidBit(1);
			(*ServerClientArray)[i].SetPlayerNumber(i);
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

DWORD __stdcall InGameThread(LPVOID arg) {
	std::array<Packet, 4> ClientInfo;

	auto beforeTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	auto elapsedTime = std::chrono::microseconds(0);
	auto totalElapsedTime = std::chrono::microseconds(0);

	Init(&ClientInfo, ((ThreadArg*)arg)->GetClientInfoArray(), ((ThreadArg*)arg)->GetServerClientArray(), ((ThreadArg*)arg)->GetServerClientArrayCS(), ((ThreadArg*)arg)->GetGameStartOrNot());

	// ���� ����
	while (1) {
		// ClientServerQueue�� ������� �ʴٸ� ClientInfo ����
		EnterCriticalSection(((ThreadArg*)arg)->GetClientServerQueueCS());

		if (!((ThreadArg*)arg)->GetClientServerQueue()->empty()) {  
			Packet p = ((ThreadArg*)arg)->GetClientServerQueue()->front();  // ť�� ù ��° ��� ��������

			if (p.GetKeyState() & 0001) {  // C Ű�� ���� �Ŷ��
				if (ClientInfo[p.GetPlayerNumber()].GetItemBit()) {  // �ش� �÷��̾ �������� ������ �ִٸ�
					for (int i = 0; i < 4; ++i) {
						if (i != p.GetPlayerNumber()) {  // �ش� �÷��̾ ������ ������ ��� �÷��̾��
							ClientInfo[i].SetAplliedBit(true);  // ������ ����
						}
					}
				}
			}
			ClientInfo[p.GetPlayerNumber()].SetKeyState(p.GetKeyState());  // ClientInfo ����

			((ThreadArg*)arg)->GetClientServerQueue()->pop();  // ť�� ù ��° ��� ����
		}

		LeaveCriticalSection(((ThreadArg*)arg)->GetClientServerQueueCS());

		// ��� �÷��̾ ����ߴٸ� ���α׷� ����
		/*if (!std::count_if(ClientInfo.begin(), ClientInfo.end(), [](const auto& packet) { return packet.GetSurvivingBit() == 0; })) {  
			((ThreadArg*)arg)->SetGameStartOrNot(false);
			break;
		}*/

		currentTime = std::chrono::high_resolution_clock::now();
		elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - beforeTime);
		totalElapsedTime += std::chrono::duration_cast<std::chrono::microseconds>(currentTime - beforeTime);
		beforeTime = std::chrono::high_resolution_clock::now();

		// Ŭ���̾�Ʈ �̵�, �浹 �˻�
		for (auto& packet : ClientInfo) {
			// Ŭ������Ʈ �̵� : Position, CurrentSurface
			// �浹 �˻� : Item, Applied, Surviving
		}

		// totalElapsedTime�� 60���� 1�ʸ� ������� �� ServerClientArray ����
		if (elapsedTime.count() >= 16'667) { 

			totalElapsedTime = std::chrono::microseconds(0);  // totalElapsedTime�� 0�ʷ� ����
		}
	}

	return 0;
}
