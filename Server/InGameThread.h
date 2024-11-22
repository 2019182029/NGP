#pragma once

void Init(std::array<Packet, 4>* ClientInfo, std::array<Packet, 4>* ClientInfoArray, std::array<Packet, 4>* ServerClientArray, CRITICAL_SECTION* ServerClientArray_CS, bool* isGameStart);
DWORD WINAPI InGameThread(LPVOID arg);
