#pragma once
#ifndef PACKET_H
#define PACKET_H

#include "main.h"

class Packet {
public:
    Packet();
    void setPosition(float x, float y);
    void setReady(bool ready);
    void setGameStart(bool start);
    void setPlayerNumber(uint8_t playerNumber);
    void setPlayerNumberValid(bool valid);
    void setItemPossession(bool hasItem);
    void setItemApplied(bool itemApplied);
    void setHp(bool hp);
    void setFacingDirection(uint8_t direction);
    void setKeyPress(uint8_t keyCode);
    void setObstacleSeed(uint8_t seed);

    bool isReady() const;
    bool isGameStarted() const;
    uint8_t getPlayerNumber() const;
    bool isPlayerNumberValid() const;
    bool hasItem() const;
    bool isItemApplied() const;
    bool getHp() const;
    uint8_t getFacingDirection() const;
    uint8_t getKeyPress() const;
    uint8_t getObstacleSeed() const;

private:
    float x, y;          // Player position
    uint8_t state[2];    // Player state information
};

#endif // PACKET_H
