#pragma once

class Object : public Packet {
private:
	float z;
	float xdir, ydir, zdir;

public:
	void SetZPosition(float fz) { z = fz; }
	void SetDir(float fxdir, float fydir, float fzdir = 0.0f) { xdir += fxdir; ydir += fydir; zdir = fzdir; }

	float GetZPosition() { return z; }
	float GetXDir() { return xdir; }
	float GetYDir() { return ydir; }
	float GetZDir() { return zdir; }
};

void Init(std::array<Packet, 4>* ClientInfoArray, std::array<Packet, 4>* ServerClientArray, std::array<Object, 4>* ClientInfo, std::array<Object, 10>* Obstacles, CRITICAL_SECTION* ServerClientArray_CS, volatile bool* isGameStart);
void PopFromClientServerQueue(std::queue<Packet>* ClientServerQueue, std::array<Object, 4>* ClientInfo, CRITICAL_SECTION* ClientServerQueue_CS);
void Update(Packet packet, std::array<Object, 4>* ClientInfo);
void MovePlayer(std::array<Object, 4>* ClientInfo, double elapsedTime);
void MoveObstacle(std::array<Object, 10>* Obstacles, double elapsedTime);
void CheckCollision(std::array<Object, 4>* ClientInfo, std::array<Object, 10>* Obstacles);
void RenewalServerClientArray(std::array<Packet, 4>* ServerClientArray, std::array<Object, 4>* ClientInfo, CRITICAL_SECTION* ServerClientArray_CS);

DWORD WINAPI InGameThread(LPVOID arg);
