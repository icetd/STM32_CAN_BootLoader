#include "FlashInterface.h"
#include "CanInterface.h"
#include "BootLoader.h"

FlashInterface flash(APP_START_ADDRESS, APP_END_ADDRESS);
CanInterface can(&hcan1, NODE_ID);
Bootloader loader(flash, can);

extern "C" void Main()
{
    can.init();
    
    loader.run();

    while (1)
    {
    }
}

extern "C" void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef* hcan)
{
    CAN_RxHeaderTypeDef rxHeader;
    uint8_t rxData[8];
    if(HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rxHeader, rxData) != HAL_OK)
    {
        Error_Handler();
    }

    uint8_t id  = (rxHeader.StdId >> 7);
    uint8_t cmd = (rxHeader.StdId & 0x7F);

    if (id == NODE_ID)
        loader.processCanCmd(id, cmd, rxData, rxHeader.DLC);
}
