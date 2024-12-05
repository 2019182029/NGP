#include <iostream>
#include <queue>
#include "Common.h"
#include "ClientServerThread.h"

DWORD WINAPI RecvThread(LPVOID arg);

void init(SOCKET s, std::array<Packet, 4>* CIA, std::queue<Packet>* CSQ, HANDLE* CIA_WriteEvent, HANDLE* CIA_ReadEvent, CRITICAL_SECTION* CSQ_CS, CRITICAL_SECTION* SCQ_CS, LPVOID arg) {
    bool slotFound = false;
    int count = 0;

    WaitForSingleObject(CIA_ReadEvent, INFINITE);

    if (!(*((ThreadArg*)arg)->GetGameStartOrNot())) {
        for (int i = 0; i < 4; ++i) {
            if (!CIA->at(i).GetValidBit()) {

                slotFound = true;

                CIA->at(i).SetValidBit(true);
                CIA->at(i).SetPlayerNumber(i);

                Packet response;
                response.SetValidBit(true);
                response.SetPlayerNumber(i);
                //std::cout << response.GetValidBit() << std::endl;

                int retval = send(s, (char*)&response, sizeof(response), 0);
                if (retval <= 0) {
                    err_display("send()");
                    break;
                }

                HANDLE hRecvThread = CreateThread(NULL, 0, RecvThread, arg, 0, NULL);
                if (hRecvThread == NULL) {
                    err_display("RecvThread 생성 실패");
                    closesocket(s);
                    return;
                }
                CloseHandle(hRecvThread);
                return;
            }
        }

        SetEvent(CIA_WriteEvent);

        if (!slotFound) {
            std::cout << "자리 꽉 참" << std::endl;
            Packet response;
            response.SetValidBit(false);
            send(s, (char*)&response, sizeof(response), 0);
        }
    }
    else {
        Packet response;
        response.SetValidBit(false);
        response.SetStartBit(true);
        //std::cout << response.GetValidBit() << std::endl;

        int retval = send(s, (char*)&response, sizeof(response), 0);
        if (retval <= 0) {
            err_display("send()");
        }
    }
}

DWORD WINAPI RecvThread(LPVOID arg) {
    auto* args = reinterpret_cast<ThreadArg*>(arg);
    SOCKET s = args->GetSocket();
    auto* CIA = args->GetClientInfoArray();
    auto* CSQ = args->GetClientServerQueue();
    volatile bool* isGameStarted = args->GetGameStartOrNot();
    HANDLE* CIA_WriteEvent = args->GetClientInfoArrayWriteEvent();
    HANDLE* CIA_ReadEvent = args->GetClientInfoArrayReadEvent();
    CRITICAL_SECTION* CSQ_CS = args->GetClientServerQueueCS();
    CRITICAL_SECTION* SCA_CS = args->GetServerClientArrayCS();

    while (true) {
        Packet receivedPacket;
        int retval = recv(s, (char*)&receivedPacket, sizeof(receivedPacket), 0);

        if (!receivedPacket.GetValidBit()) {
            std::cout << "recv() 정상 종료" << std::endl;

            int playerNumber = receivedPacket.GetPlayerNumber();
            if (playerNumber >= 0 && playerNumber < 4) {
                std::cout << "플레이어 " << playerNumber << " 연결 종료." << std::endl;

                // 클라이언트 슬롯 무효화
                WaitForSingleObject(*CIA_ReadEvent, INFINITE);
                CIA->at(playerNumber).SetValidBit(false);
                CIA->at(playerNumber).SetReadyBit(false);
                SetEvent(*CIA_WriteEvent);

                // 스레드 종료
                break;
            }
        }

        if (!*isGameStarted) {
            WaitForSingleObject(*CIA_ReadEvent, INFINITE);
            int playerNumber = receivedPacket.GetPlayerNumber();
            if (playerNumber >= 0 && playerNumber < 4) {
                CIA->at(playerNumber) = receivedPacket;
            }
            SetEvent(*CIA_WriteEvent);
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
    auto* Obstacles = args->GetObstacleArray();
    volatile bool* isGameStarted = args->GetGameStartOrNot();
    HANDLE* CIA_WriteEvent = args->GetClientInfoArrayWriteEvent();
    HANDLE* CIA_ReadEvent = args->GetClientInfoArrayReadEvent();
    CRITICAL_SECTION* CSQ_CS = args->GetClientServerQueueCS();
    CRITICAL_SECTION* SCA_CS = args->GetServerClientArrayCS();

    Packet ret;

    init(s, CIA, CSQ, CIA_WriteEvent, CIA_ReadEvent, CSQ_CS, SCA_CS, arg);

    while (true) {
        if (*isGameStarted) {
            ret.SetStartBit(true);
            send(s, (char*)&ret, sizeof(ret), 0);

            auto beforeTime = std::chrono::high_resolution_clock::now();
            auto currentTime = std::chrono::high_resolution_clock::now();
            double elapsedTime = 0.0;

            double Timeout = 1.0 / 60.0;

            while (1) {
                if (elapsedTime > Timeout) {
                    EnterCriticalSection(SCA_CS);
                    send(s, (char*)SCA, sizeof(*SCA), 0);
                    send(s, (char*)Obstacles, sizeof(*Obstacles), 0);
                    LeaveCriticalSection(SCA_CS);

                    elapsedTime = 0.0;
                }

                currentTime = std::chrono::high_resolution_clock::now();
                elapsedTime += ((std::chrono::duration<double>)std::chrono::duration_cast<std::chrono::microseconds>(currentTime - beforeTime)).count();
                beforeTime = std::chrono::high_resolution_clock::now();

                std::this_thread::sleep_for(std::chrono::milliseconds(1)); // CPU 사용량 감소
            }
        }
    }

    closesocket(s);
    delete args;
    return 0;
}
