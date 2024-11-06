#include "main.h"

class Packet {
public:
    float x, y;  // 플레이어 위치
    uint8_t state[2];  // 플레이어 상태 정보

    // 준비 완료 여부 (1 비트)
    bool getReadyStatus() const {
        return state[0] & 0x80;
    }
    void setReadyStatus(bool ready) {
        if (ready) state[0] |= 0x80;
        else state[0] &= ~0x80;
    }

    // 게임 시작 여부 (1 비트)
    bool getGameStartStatus() const {
        return state[0] & 0x40;
    }
    void setGameStartStatus(bool start) {
        if (start) state[0] |= 0x40;
        else state[0] &= ~0x40;
    }

    // 유효 비트 (1 비트)
    bool getValidBit() const {
        return state[0] & 0x20;
    }
    void setValidBit(bool valid) {
        if (valid) state[0] |= 0x20;
        else state[0] &= ~0x20;
    }

    // 숫자 비트 (2 비트)
    uint8_t getNumberBit() const {
        return (state[0] >> 3) & 0x03;
    }
    void setNumberBit(uint8_t number) {
        state[0] = (state[0] & ~0x18) | ((number & 0x03) << 3);
    }

    // 아이템 보유 여부 (1 비트)
    bool getItemOwnership() const {
        return state[0] & 0x04;
    }
    void setItemOwnership(bool hasItem) {
        if (hasItem) state[0] |= 0x04;
        else state[0] &= ~0x04;
    }

    // 아이템 적용 여부 (1 비트)
    bool getItemApplication() const {
        return state[0] & 0x02;
    }
    void setItemApplication(bool applied) {
        if (applied) state[0] |= 0x02;
        else state[0] &= ~0x02;
    }

    // HP 상태 (1 비트)
    bool getHP() const {
        return state[0] & 0x01;
    }
    void setHP(bool hp) {
        if (hp) state[0] |= 0x01;
        else state[0] &= ~0x01;
    }

    // 바닥 면 방향 (2 비트)
    uint8_t getSurfaceOrientation() const {
        return (state[1] >> 6) & 0x03;
    }
    void setSurfaceOrientation(uint8_t orientation) {
        state[1] = (state[1] & ~0xC0) | ((orientation & 0x03) << 6);
    }

    // 키 입력 상태 (4 비트)
    uint8_t getKeyPressStatus() const {
        return (state[1] >> 2) & 0x0F;
    }
    void setKeyPressStatus(uint8_t keys) {
        state[1] = (state[1] & ~0x3C) | ((keys & 0x0F) << 2);
    }

    // 장애물 시드 (2 비트)
    uint8_t getObstacleSeed() const {
        return state[1] & 0x03;
    }
    void setObstacleSeed(uint8_t seed) {
        state[1] = (state[1] & ~0x03) | (seed & 0x03);
    }
};
