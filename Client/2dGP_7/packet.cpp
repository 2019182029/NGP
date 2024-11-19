#include "Packet.h"

Packet::Packet() : x(0), y(0) {
    state[0] = 0;
    state[1] = 0;
}

void Packet::setPosition(float xPos, float yPos) {
    x = xPos;
    y = yPos;
}

// 준비 완료 여부 설정 (7번째 비트)
void Packet::setReady(bool ready) {
    if (ready) state[0] |= 0x80;
    else state[0] &= ~0x80;
}

// 게임 시작 여부 설정 (6번째 비트)
void Packet::setGameStart(bool start) {
    if (start) state[0] |= 0x40;
    else state[0] &= ~0x40;
}

// 플레이어 번호 설정 (Valid 비트 1비트 + Number 비트 2비트: 5, 4, 3번째 비트)
void Packet::setPlayerNumber(uint8_t playerNumber) {
    state[0] = (state[0] & 0xC7) | ((playerNumber & 0x07) << 3);
}

// 플레이어 번호 유효성 설정 (Valid 비트만 설정, 5번째 비트)
void Packet::setPlayerNumberValid(bool valid) {
    if (valid) state[0] |= 0x20;
    else state[0] &= ~0x20;
}

// 아이템 보유 여부 설정 (2번째 비트)
void Packet::setItemPossession(bool hasItem) {
    if (hasItem) state[0] |= 0x04;
    else state[0] &= ~0x04;
}

// 아이템 적용 여부 설정 (1번째 비트)
void Packet::setItemApplied(bool itemApplied) {
    if (itemApplied) state[0] |= 0x02;
    else state[0] &= ~0x02;
}

// HP 설정 (0번째 비트)
void Packet::setHp(bool hp) {
    if (hp) state[0] |= 0x01;
    else state[0] &= ~0x01;
}

// 현재 바라보는 방향 설정 (state[1]의 1, 0번째 비트)
void Packet::setFacingDirection(uint8_t direction) {
    state[1] = (state[1] & 0xFC) | (direction & 0x03);
}

// 키 입력 설정 (state[1]의 5, 4, 3, 2번째 비트)
void Packet::setKeyPress(uint8_t keyCode) {
    state[1] = (state[1] & 0xC3) | ((keyCode & 0x0F) << 2);
}

// 장애물 시드 설정 (state[1]의 7, 6번째 비트)
void Packet::setObstacleSeed(uint8_t seed) {
    state[1] = (state[1] & 0x3F) | (seed << 6);
}

// 상태 확인 함수들
bool Packet::isReady() const {
    return (state[0] & 0x80) != 0;
}

bool Packet::isGameStarted() const {
    return (state[0] & 0x40) != 0;
}

// 플레이어 번호를 반환 (Number 비트, 2비트: 4, 3번째 비트)
uint8_t Packet::getPlayerNumber() const {
    return (state[0] >> 3) & 0x03;
}

// 플레이어 번호 유효성 반환 (Valid 비트, 1비트: 5번째 비트)
bool Packet::isPlayerNumberValid() const {
    return (state[0] & 0x20) != 0;
}

bool Packet::hasItem() const {
    return (state[0] & 0x04) != 0;
}

bool Packet::isItemApplied() const {
    return (state[0] & 0x02) != 0;
}

bool Packet::getHp() const {
    return (state[0] & 0x01) != 0;
}

uint8_t Packet::getFacingDirection() const {
    return state[1] & 0x03;
}

uint8_t Packet::getKeyPress() const {
    return (state[1] >> 2) & 0x0F;
}

uint8_t Packet::getObstacleSeed() const {
    return (state[1] >> 6) & 0x03;
}
