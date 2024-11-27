#pragma once

class Object : public Packet {
private:
	float xdir, ydir;

public:
	void SetDir(float fxdir, float fydir) { xdir += fxdir; ydir += fydir; }

	float GetXDir() { return xdir; }
	float GetYDir() { return ydir; }
};

void Init(std::array<Packet, 4>* ClientInfoArray, std::array<Packet, 4>* ServerClientArray, std::array<Object, 4>* ClientInfo, CRITICAL_SECTION* ServerClientArray_CS, volatile bool* isGameStart);
void PopFromClientServerQueue(std::queue<Packet>* ClientServerQueue, std::array<Object, 4>* ClientInfo, CRITICAL_SECTION* ClientServerQueue_CS);
void Update(Packet packet, std::array<Object, 4>* ClientInfo);
void MovePlayer(std::array<Object, 4>* ClientInfo, double elapsedTime);
void MoveObstacle(std::array<Object, 10>* Obstacles, double elapsedTime);
void RenewalServerClientArray(std::array<Packet, 4>* ServerClientArray, std::array<Object, 4>* ClientInfo, CRITICAL_SECTION* ServerClientArray_CS);

DWORD WINAPI InGameThread(LPVOID arg);
