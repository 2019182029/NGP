#pragma once
#ifndef PACKET_H
#define PACKET_H
#define _CRT_SECURE_NO_WARNINGS // 구형 C 함수 사용 시 경고 끄기
#define _WINSOCK_DEPRECATED_NO_WARNINGS // 구형 소켓 API 사용 시 경고 끄기

#include "main.h"

class Packet {
public:
    Packet();
    float getX();
    float getY();

    void SetPosition(float fx, float fy) { x = fx; y = fy; }
    void SetReadyBit(bool ready) { state[0] = ready ? (state[0] | 0b10000000) : (state[0] & 0b01111111); }
    void SetStartBit(bool start) { state[0] = start ? (state[0] | 0b01000000) : (state[0] & 0b10111111); }
    void SetValidBit(bool valid) { state[0] = valid ? (state[0] | 0b00100000) : (state[0] & 0b11011111); }
    void SetPlayerNumber(int number) { state[0] = ((state[0] & 0b11100111) | (number << 3)); }
    void SetItemBit(bool item) { state[0] = item ? (state[0] | 0b00000100) : (state[0] & 0b11111011); }
    void SetAplliedBit(bool applied) { state[0] = applied ? (state[0] | 0b00000010) : (state[0] & 0b11111101); }
    void SetSurvivingBit(bool surviving) { state[0] = surviving ? (state[0] | 0b00000001) : (state[0] & 0b11111110); }
    void SetCurrentSurface(int surface) { state[1] = ((state[1] & 0b00111111) | (surface << 6)); }
    void SetKeyState(int keyState) { state[1] = ((state[1] & 0b11000011) | (keyState << 2)); }
    void SetSeed(int seed) { state[1] = ((state[1] & 0b11111100) | seed); }

    float GetXPosition() { return x; }
    float GetYPosition() { return y; }
    bool GetReadyBit() { return (state[0] & 0b10000000) >> 7; }
    bool GetStartBit() { return (state[0] & 0b01000000) >> 6; }
    bool GetValidBit() { return (state[0] & 0b00100000) >> 5; }
    int GetPlayerNumber() { return (state[0] & 0b00011000) >> 3; }
    bool GetItemBit() { return (state[0] & 0b00000100) >> 2; }
    bool GetAppliedBit() { return (state[0] & 0b00000010) >> 1; }
    bool GetSurvivingBit() { return state[0] & 0b00000001; }
    int GetCurrentSurface() { return (state[1] & 0b11000000) >> 6; }
    int GetKeyState() { return (state[1] & 0b00111100) >> 2; }
    int GetSeed() { return state[1] & 0b00000011; }



    // 키 상태 업데이트 메서드 추가
    void setKeyState(char key, bool isPressed);

    // 키 상태 확인 메서드 추가
    bool getKeyState(char key) const;

private:
    float x, y;          // Player position
    BYTE state[2];
};

#endif // PACKET_H
