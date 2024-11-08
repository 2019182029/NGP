#include "Packet.h"

Packet::Packet() : x(0), y(0) {
    state[0] = 0;
    state[1] = 0;
}

void Packet::setPosition(float xPos, float yPos) {
    x = xPos;
    y = yPos;
}

// �غ� �Ϸ� ���� ���� (7��° ��Ʈ)
void Packet::setReady(bool ready) {
    if (ready) state[0] |= 0x80;
    else state[0] &= ~0x80;
}

// ���� ���� ���� ���� (6��° ��Ʈ)
void Packet::setGameStart(bool start) {
    if (start) state[0] |= 0x40;
    else state[0] &= ~0x40;
}

// �÷��̾� ��ȣ ���� (Valid ��Ʈ 1��Ʈ + Number ��Ʈ 2��Ʈ: 5, 4, 3��° ��Ʈ)
void Packet::setPlayerNumber(uint8_t playerNumber) {
    state[0] = (state[0] & 0xC7) | ((playerNumber & 0x07) << 3);
}

// �÷��̾� ��ȣ ��ȿ�� ���� (Valid ��Ʈ�� ����, 5��° ��Ʈ)
void Packet::setPlayerNumberValid(bool valid) {
    if (valid) state[0] |= 0x20;
    else state[0] &= ~0x20;
}

// ������ ���� ���� ���� (2��° ��Ʈ)
void Packet::setItemPossession(bool hasItem) {
    if (hasItem) state[0] |= 0x04;
    else state[0] &= ~0x04;
}

// ������ ���� ���� ���� (1��° ��Ʈ)
void Packet::setItemApplied(bool itemApplied) {
    if (itemApplied) state[0] |= 0x02;
    else state[0] &= ~0x02;
}

// HP ���� (0��° ��Ʈ)
void Packet::setHp(bool hp) {
    if (hp) state[0] |= 0x01;
    else state[0] &= ~0x01;
}

// ���� �ٶ󺸴� ���� ���� (state[1]�� 1, 0��° ��Ʈ)
void Packet::setFacingDirection(uint8_t direction) {
    state[1] = (state[1] & 0xFC) | (direction & 0x03);
}

// Ű �Է� ���� (state[1]�� 5, 4, 3, 2��° ��Ʈ)
void Packet::setKeyPress(uint8_t keyCode) {
    state[1] = (state[1] & 0xC3) | ((keyCode & 0x0F) << 2);
}

// ��ֹ� �õ� ���� (state[1]�� 7, 6��° ��Ʈ)
void Packet::setObstacleSeed(uint8_t seed) {
    state[1] = (state[1] & 0x3F) | (seed << 6);
}

// ���� Ȯ�� �Լ���
bool Packet::isReady() const {
    return (state[0] & 0x80) != 0;
}

bool Packet::isGameStarted() const {
    return (state[0] & 0x40) != 0;
}

// �÷��̾� ��ȣ�� ��ȯ (Number ��Ʈ, 2��Ʈ: 4, 3��° ��Ʈ)
uint8_t Packet::getPlayerNumber() const {
    return (state[0] >> 3) & 0x03;
}

// �÷��̾� ��ȣ ��ȿ�� ��ȯ (Valid ��Ʈ, 1��Ʈ: 5��° ��Ʈ)
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
