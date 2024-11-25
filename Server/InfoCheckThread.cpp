#include "Common.h"
#include "InfoCheckThread.h"
#include "InGameThread.h"

DWORD __stdcall InfoCheckThread(LPVOID arg) {
    while (1) {
        WaitForSingleObject(*((ThreadArg*)arg)->GetClientInfoArrayWriteEvent(), INFINITE);  // ClientInfoArray ���� ���

        std::cout << "�÷��̾� �غ� �Ϸ� �˻� : " << std::count_if(  // �غ� �Ϸ�� �÷��̾� ��
            (*((ThreadArg*)arg)->GetClientInfoArray()).begin(),
            (*((ThreadArg*)arg)->GetClientInfoArray()).end(),
            [](Packet& packet) {
                return packet.GetReadyBit();
            }) << "/" << std::count_if(  // �÷��̾� ��
                (*((ThreadArg*)arg)->GetClientInfoArray()).begin(),
                (*((ThreadArg*)arg)->GetClientInfoArray()).end(),
                [](Packet& packet) {
                    return packet.GetValidBit();
                }) << std::endl;

        if (std::count_if(  // �÷��̾� ��
            (*((ThreadArg*)arg)->GetClientInfoArray()).begin(),
            (*((ThreadArg*)arg)->GetClientInfoArray()).end(),
            [](Packet& packet) {
                return packet.GetValidBit();
            }) == std::count_if(  // �غ� �Ϸ�� �÷��̾� ��
                (*((ThreadArg*)arg)->GetClientInfoArray()).begin(),
                (*((ThreadArg*)arg)->GetClientInfoArray()).end(),
                [](Packet& packet) {
                    return packet.GetReadyBit();
                })) {
            break;
        }

        std::cout << "��� �÷��̾� �غ� �Ϸ�" << std::endl << std::endl;
        std::cout << "���� ���� �غ� ��..." << std::endl;

        SetEvent(((ThreadArg*)arg)->GetClientInfoArrayReadEvent());
    }

    HANDLE hThread;
    hThread = CreateThread(NULL, 0, InGameThread, arg, 0, NULL);
    if (hThread != NULL) { CloseHandle(hThread); }

    return 0;
}
