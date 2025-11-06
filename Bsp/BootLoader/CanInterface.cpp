// CanInterface.cpp
#include "CanInterface.h"
#include "BootLoader.h"

CanInterface::CanInterface(CAN_HandleTypeDef *hcan, uint16_t nodeId) : hcan_(hcan), nodeId_(nodeId), txMailbox_(0)
{
}

void CanInterface::init()
{
    CAN_FilterTypeDef filterConfig;

    filterConfig.FilterBank = 0;
    filterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
    filterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    filterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    filterConfig.FilterIdHigh = 0x0000;
    filterConfig.FilterIdLow = 0x0000;
    filterConfig.FilterMaskIdHigh = 0x0000;
    filterConfig.FilterMaskIdLow = 0x0000;
    filterConfig.FilterActivation = ENABLE;
    filterConfig.SlaveStartFilterBank = 14;

    if (HAL_CAN_ConfigFilter(hcan_, &filterConfig) != HAL_OK) {
        Error_Handler();
    }

    /*start can*/
    HAL_CAN_Start(hcan_);
    HAL_CAN_ActivateNotification(hcan_,
                                 CAN_IT_TX_MAILBOX_EMPTY |
                                 CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_RX_FIFO1_MSG_PENDING |
                                 /* we probably only want this */
                                 CAN_IT_RX_FIFO0_FULL | CAN_IT_RX_FIFO1_FULL |
                                 CAN_IT_RX_FIFO0_OVERRUN | CAN_IT_RX_FIFO1_OVERRUN |
                                 CAN_IT_WAKEUP | CAN_IT_SLEEP_ACK |
                                 CAN_IT_ERROR_WARNING | CAN_IT_ERROR_PASSIVE |
                                 CAN_IT_BUSOFF | CAN_IT_LAST_ERROR_CODE |
                                 CAN_IT_ERROR);

    txHeader_.StdId = nodeId_;
    txHeader_.ExtId = 0x00;
    txHeader_.RTR = CAN_RTR_DATA;
    txHeader_.IDE = CAN_ID_STD;
    txHeader_.DLC = 8;
    txHeader_.TransmitGlobalTime = DISABLE;
}

void CanInterface::send(const uint8_t *data, uint8_t len)
{
    if (len > 8) len = 8;
    for (uint8_t i = 0; i < len; i++)
        txData_[i] = data[i];

    txMailbox_ = 0;
    HAL_CAN_AddTxMessage(hcan_, &txHeader_, txData_, &txMailbox_);
}

void CanInterface::send(uint32_t id, const uint8_t *data, uint8_t len)
{
    if (len > 8) len = 8;
    for (uint8_t i = 0; i < len; i++)
        txData_[i] = data[i];

    txHeader_.StdId = id;
    txMailbox_ = 0;
    HAL_CAN_AddTxMessage(hcan_, &txHeader_, txData_, &txMailbox_);
}
