#include "Common.h"
#include "InfoCheckThread.h"
#include "InGameThread.h"

DWORD __stdcall InfoCheckThread(LPVOID arg) {
    while (1) {
        WaitForSingleObject(*(((ThreadArg*)arg)->GetClientInfoArrayWriteEvent()), INFINITE);  // ClientInfoArray 갱신 대기

        std::cout << "플레이어 준비 완료 검사 : " << std::count_if(  // 준비 완료된 플레이어 수
            (*((ThreadArg*)arg)->GetClientInfoArray()).begin(),
            (*((ThreadArg*)arg)->GetClientInfoArray()).end(),
            [](Packet& packet) {
                return packet.GetReadyBit();
            }) << "/" << std::count_if(  // 플레이어 수
                (*((ThreadArg*)arg)->GetClientInfoArray()).begin(),
                (*((ThreadArg*)arg)->GetClientInfoArray()).end(),
                [](Packet& packet) {
                    return packet.GetValidBit();
                }) << "\n";

        if (std::count_if(  // 플레이어 수
            (*((ThreadArg*)arg)->GetClientInfoArray()).begin(),
            (*((ThreadArg*)arg)->GetClientInfoArray()).end(),
            [](Packet& packet) {
                return packet.GetValidBit();
            }) == std::count_if(  // 준비 완료된 플레이어 수
                (*((ThreadArg*)arg)->GetClientInfoArray()).begin(),
                (*((ThreadArg*)arg)->GetClientInfoArray()).end(),
                [](Packet& packet) {
                    return packet.GetReadyBit();
                })) {
            std::cout << "모든 플레이어 준비 완료" << std::endl << std::endl;
            std::cout << "게임 시작 준비 중..." << std::endl;
            break;
        }

        SetEvent(*(((ThreadArg*)arg)->GetClientInfoArrayReadEvent()));
    }

    HANDLE hInGameThread;
    hInGameThread = CreateThread(NULL, 0, InGameThread, arg, 0, NULL);

    WaitForSingleObject(hInGameThread, INFINITE);

    return 0;
}
