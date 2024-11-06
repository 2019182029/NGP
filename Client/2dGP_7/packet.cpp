#include "main.h"

class Packet {
public:
    float x, y;  // �÷��̾� ��ġ
    uint8_t state[2];  // �÷��̾� ���� ����

    // �غ� �Ϸ� ���� (1 ��Ʈ)
    bool getReadyStatus() const {
        return state[0] & 0x80;
    }
    void setReadyStatus(bool ready) {
        if (ready) state[0] |= 0x80;
        else state[0] &= ~0x80;
    }

    // ���� ���� ���� (1 ��Ʈ)
    bool getGameStartStatus() const {
        return state[0] & 0x40;
    }
    void setGameStartStatus(bool start) {
        if (start) state[0] |= 0x40;
        else state[0] &= ~0x40;
    }

    // ��ȿ ��Ʈ (1 ��Ʈ)
    bool getValidBit() const {
        return state[0] & 0x20;
    }
    void setValidBit(bool valid) {
        if (valid) state[0] |= 0x20;
        else state[0] &= ~0x20;
    }

    // ���� ��Ʈ (2 ��Ʈ)
    uint8_t getNumberBit() const {
        return (state[0] >> 3) & 0x03;
    }
    void setNumberBit(uint8_t number) {
        state[0] = (state[0] & ~0x18) | ((number & 0x03) << 3);
    }

    // ������ ���� ���� (1 ��Ʈ)
    bool getItemOwnership() const {
        return state[0] & 0x04;
    }
    void setItemOwnership(bool hasItem) {
        if (hasItem) state[0] |= 0x04;
        else state[0] &= ~0x04;
    }

    // ������ ���� ���� (1 ��Ʈ)
    bool getItemApplication() const {
        return state[0] & 0x02;
    }
    void setItemApplication(bool applied) {
        if (applied) state[0] |= 0x02;
        else state[0] &= ~0x02;
    }

    // HP ���� (1 ��Ʈ)
    bool getHP() const {
        return state[0] & 0x01;
    }
    void setHP(bool hp) {
        if (hp) state[0] |= 0x01;
        else state[0] &= ~0x01;
    }

    // �ٴ� �� ���� (2 ��Ʈ)
    uint8_t getSurfaceOrientation() const {
        return (state[1] >> 6) & 0x03;
    }
    void setSurfaceOrientation(uint8_t orientation) {
        state[1] = (state[1] & ~0xC0) | ((orientation & 0x03) << 6);
    }

    // Ű �Է� ���� (4 ��Ʈ)
    uint8_t getKeyPressStatus() const {
        return (state[1] >> 2) & 0x0F;
    }
    void setKeyPressStatus(uint8_t keys) {
        state[1] = (state[1] & ~0x3C) | ((keys & 0x0F) << 2);
    }

    // ��ֹ� �õ� (2 ��Ʈ)
    uint8_t getObstacleSeed() const {
        return state[1] & 0x03;
    }
    void setObstacleSeed(uint8_t seed) {
        state[1] = (state[1] & ~0x03) | (seed & 0x03);
    }
};
