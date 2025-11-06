// CanInterface.h
#pragma once
#include "stm32f4xx_hal.h"
#include <can.h>
#include <cstdint>

class CanInterface {
public:
    CanInterface(CAN_HandleTypeDef* hcan, uint16_t nodeId);
    void init();
    void send(const uint8_t* data, uint8_t len);
    void send(uint32_t id, const uint8_t* data, uint8_t len);

private:
    CAN_HandleTypeDef* hcan_;
    CAN_TxHeaderTypeDef txHeader_;
    CAN_RxHeaderTypeDef rxHeader_;
    uint8_t txData_[8];
    uint8_t rxData_[8];
    uint32_t txMailbox_;
    uint16_t nodeId_;
};
