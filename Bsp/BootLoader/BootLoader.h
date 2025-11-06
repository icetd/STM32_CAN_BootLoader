#pragma once
#include "FlashInterface.h"
#include "CanInterface.h"
#include "Led.h"

/*
Flash Memory Layout (STM32F412, 1 MB Flash)

0x08000000 ──+-------------------+
             | 0x08007FFF        | <- Bootloader Region (32KB)
             | Bootloader Code   |
             |                   |
0x08008000 ──+-------------------+
             |                   | <- Application Code Region
             |   Application     |    (736KB)
             |                   |
0x080BFFFC ──+-------------------+
             | App Length (4B)   | <- Application Length Storage at APP_END_ADDRESS - 4
0x080C0000 ──+-------------------+
             | CRC (4B)          | <- CRC Storage
0x080C0004 ──+-------------------+
*/


#define RAM_START        0x20000000U  // SRAM Start
#define RAM_SIZE         (256 * 1024) // 256KB RAM
#define FLASH_SIZE       (1024 * 1024) // 1MB Flash

#define APP_START_ADDRESS 0x08008000 // Application start address
#define APP_END_ADDRESS   0x080C0000 // Application code end address
#define CRC_ADDRESS       0x080C0004 // CRC storage address
#define NODE_ID           0x02       // CAN node ID

class Bootloader
{
public:
    Bootloader(FlashInterface &flash, CanInterface &can) : flash_(flash), can_(can), loaderMode_(true), flashInProgress_(false), flashIndex_(0) {}

    void processCanCmd(uint8_t id, uint8_t cmd, uint8_t *data, uint8_t len);
    void run();

private:
    volatile uint32_t lastCmdTick_ = 0;
    FlashInterface &flash_;
    CanInterface &can_;
    bool loaderMode_;
    bool flashInProgress_;
    uint32_t flashIndex_;

    void sendConfirm(uint8_t id, uint8_t status);
    void sendCRC(uint8_t id, uint32_t crc);
};