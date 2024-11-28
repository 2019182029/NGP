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

    // 키에 따른 비트 매핑 (2~5번째 비트: W, A, D, C)
    switch (key) {
    case 'w': bitMask = 0x01; break; // W -> 2번째 비트
    case 'a': bitMask = 0x02; break; // A -> 3번째 비트
    case 'd': bitMask = 0x04; break; // D -> 4번째 비트
    case 'c': bitMask = 0x08; break; // C -> 5번째 비트
    default: return; // 유효하지 않은 키
    }

    bitMask <<= 2; // 2번째 비트부터 시작하도록 이동

    if (isPressed) {
        state[1] |= bitMask; // 눌린 상태 -> 비트 설정
    }
    else {
        state[1] &= ~bitMask; // 떼진 상태 -> 비트 해제
    }
}

bool Packet::getKeyState(char key) const {
    BYTE bitMask = 0;

    switch (key) {
    case 'w': bitMask = 0x01; break;
    case 'a': bitMask = 0x02; break;
    case 'd': bitMask = 0x04; break;
    case 'c': bitMask = 0x08; break;
    default: return false; // 유효하지 않은 키
    }

    bitMask <<= 2; // 2번째 비트부터 시작하도록 이동
    return (state[1] & bitMask) != 0; // 비트가 1인지 확인
}
