#include <Windows.h> // For BYTE definition
#include "packet.h"

Packet::Packet() : x(0), y(0) {
    state[0] = 0;
    state[1] = 0;
}

float Packet::getX() {
    return x;
}
float Packet::getY() {
    return y;
}

void Packet::setKeyState(char key, bool isPressed) {
    BYTE bitMask = 0;

    // Ű�� ���� ��Ʈ ���� (2~5��° ��Ʈ: W, A, D, C)
    switch (key) {
    case 'w': bitMask = 0x01; break; // W -> 2��° ��Ʈ
    case 'a': bitMask = 0x02; break; // A -> 3��° ��Ʈ
    case 'd': bitMask = 0x04; break; // D -> 4��° ��Ʈ
    case 'c': bitMask = 0x08; break; // C -> 5��° ��Ʈ
    default: return; // ��ȿ���� ���� Ű
    }

    bitMask <<= 2; // 2��° ��Ʈ���� �����ϵ��� �̵�

    if (isPressed) {
        state[1] |= bitMask; // ���� ���� -> ��Ʈ ����
    }
    else {
        state[1] &= ~bitMask; // ���� ���� -> ��Ʈ ����
    }
}

bool Packet::getKeyState(char key) const {
    BYTE bitMask = 0;

    switch (key) {
    case 'w': bitMask = 0x01; break;
    case 'a': bitMask = 0x02; break;
    case 'd': bitMask = 0x04; break;
    case 'c': bitMask = 0x08; break;
    default: return false; // ��ȿ���� ���� Ű
    }

    bitMask <<= 2; // 2��° ��Ʈ���� �����ϵ��� �̵�
    return (state[1] & bitMask) != 0; // ��Ʈ�� 1���� Ȯ��
}
