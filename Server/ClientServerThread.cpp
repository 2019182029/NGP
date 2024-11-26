#include <iostream>
#include <queue>
#include "Common.h"
#include "ClientServerThread.h"

DWORD WINAPI RecvThread(LPVOID arg);

void init(SOCKET s, std::array<Packet, 4>* CIA, std::queue<Packet>* CSQ, HANDLE* CIA_WriteEvent, HANDLE* CIA_ReadEvent, CRITICAL_SECTION* CSQ_CS, CRITICAL_SECTION* SCQ_CS, LPVOID arg) {
    bool slotFound = false;
    int count = 0;
    
    WaitForSingleObject(CIA_ReadEvent, INFINITE);

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
    HANDLE* CIA_WriteEvent = args->GetClientInfoArrayWriteEvent();
    HANDLE* CIA_ReadEvent = args->GetClientInfoArrayReadEvent();
    CRITICAL_SECTION* CSQ_CS = args->GetClientServerQueueCS();
    CRITICAL_SECTION* SCA_CS = args->GetServerClientArrayCS();

    while (true) {
        Packet receivedPacket;
        int retval = recv(s, (char*)&receivedPacket, sizeof(receivedPacket), 0);

        if (retval <= 0) {
            std::cout << WSAGetLastError() << std::endl;
            err_display("recv()");
            break;
        }

        std::cout << "패킷 수신" << std::endl;

        if (!*isGameStarted) {
            std::cout << "이벤트 대기 전" << std::endl;
            WaitForSingleObject(*CIA_ReadEvent, INFINITE);  // 시간 설정 필요
            std::cout << "이벤트 대기 후" << std::endl;
            int playerNumber = receivedPacket.GetPlayerNumber();
            if (playerNumber >= 0 && playerNumber < 4) {
                CIA->at(playerNumber) = receivedPacket;
            }
            SetEvent(*CIA_WriteEvent);
            std::cout << "이벤트 세팅 완료" << std::endl;
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

            while (1) {
                auto beforeTime = std::chrono::high_resolution_clock::now();
                auto currentTime = std::chrono::high_resolution_clock::now();
                auto elapsedTime = std::chrono::microseconds(0);
                auto totalElapsedTime = std::chrono::microseconds(0);

                int Timeout = 3;

                if (elapsedTime.count() > Timeout) {


                    EnterCriticalSection(SCA_CS);
                    for (const auto& packet : *SCA) {
                        send(s, (char*)&packet, sizeof(packet), 0);
                    }
                    LeaveCriticalSection(SCA_CS);


                }

                currentTime = std::chrono::high_resolution_clock::now();
                elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - beforeTime);
                totalElapsedTime += std::chrono::duration_cast<std::chrono::microseconds>(currentTime - beforeTime);
                beforeTime = std::chrono::high_resolution_clock::now();

                Sleep(1000);
            }
        }
    }



    closesocket(s);
    delete args;
    return 0;
}
