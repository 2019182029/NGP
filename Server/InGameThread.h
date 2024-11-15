#pragma once

void Init(std::array<Packet, 4>* ClientInfoArray, std::array<Packet, 4>* ServerClientArray, bool* isGameStart);
DWORD WINAPI InGameThread(LPVOID arg);
