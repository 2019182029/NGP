#include <iostream>
#include <queue>
#include "Common.h"

void init(SOCKET s, std::array<Packet, 4>* CIA, std::queue<Packet>* CSQ, CRITICAL_SECTION* CSQ_CS, CRITICAL_SECTION* SCQ_CS) {
    bool slotFound = false;
    for (int i = 0; i < 4; ++i) {
        if (!CIA->at(i).GetValidBit()) {
            slotFound = true;

            EnterCriticalSection(CSQ_CS);
            CIA->at(i).SetValidBit(true);
            CIA->at(i).SetPlayerNumber(i);
            LeaveCriticalSection(CSQ_CS);

            Packet response;
            response.SetValidBit(true);
            response.SetPlayerNumber(i);
            send(s, (char*)&response, sizeof(response), 0);

            HANDLE hRecvThread = CreateThread(NULL, 0, RecvThread, CIA, 0, NULL);
            if (hRecvThread == NULL) {
                err_display("RecvThread 생성 실패");
                closesocket(s);
                return;
            }
            CloseHandle(hRecvThread);
            return;
        }
    }

    if (!slotFound) {
        Packet response;
        response.SetValidBit(false);
        send(s, (char*)&response, sizeof(response), 0);
    }
}

DWORD WINAPI RecvThread(LPVOID arg) {
    auto* args = reinterpret_cast<ThreadArg*>(arg);
    SOCKET s = args->GetSocket();
    auto* CIA = args->GetClientInfoArray();
    auto* CSQ = args->GetClientServerQueue();
    bool* isGameStarted = args->GetGameStartOrNot();
    HANDLE* CIA_Event = args->GetClientInfoArrayEvent();
    CRITICAL_SECTION* CSQ_CS = args->GetClientServerQueueCS();
    CRITICAL_SECTION* SCA_CS = args->GetServerClientArrayCS();

    while (true) {
        Packet receivedPacket;
        int retval = recv(s, (char*)&receivedPacket, sizeof(receivedPacket), 0);
        if (retval <= 0) {
            err_display("recv()");
            break;
        }

        if (!*isGameStarted) {
            EnterCriticalSection(CSQ_CS);
            int playerNumber = receivedPacket.GetPlayerNumber();
            if (playerNumber >= 0 && playerNumber < 4) {
                CIA->at(playerNumber) = receivedPacket;
            }
            LeaveCriticalSection(CSQ_CS);
        }
        else {
            EnterCriticalSection(CSQ_CS);
            CSQ->push(receivedPacket);
            LeaveCriticalSection(CSQ_CS);
        }
    }

    closesocket(s);
    return 0;
}

DWORD WINAPI ClientServerThread(LPVOID arg) {
    auto* args = reinterpret_cast<ThreadArg*>(arg);
    SOCKET s = args->GetSocket();
    auto* CIA = args->GetClientInfoArray();
    auto* CSQ = args->GetClientServerQueue();
    auto* SCA = args->GetServerClientArray();
    bool* isGameStarted = args->GetGameStartOrNot();
    HANDLE* CIA_Event = args->GetClientInfoArrayEvent();
    CRITICAL_SECTION* CSQ_CS = args->GetClientServerQueueCS();
    CRITICAL_SECTION* SCA_CS = args->GetServerClientArrayCS();

    init(s, CIA, CSQ, CSQ_CS, SCA_CS);

    while (true) {
        if (*isGameStarted) {
            static int elapsedTime = 0;
            const int Timeout = 5;

            if (elapsedTime > Timeout) {
                EnterCriticalSection(SCA_CS);
                for (const auto& packet : *SCA) {
                    send(s, (char*)&packet, sizeof(packet), 0);
                }
                LeaveCriticalSection(SCA_CS);

                elapsedTime = 0;
            }

            elapsedTime++;
            Sleep(1000);
        }
    }

    closesocket(s);
    delete args;
    return 0;
}
