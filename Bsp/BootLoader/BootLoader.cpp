#include "Bootloader.h"

void Bootloader::sendConfirm(uint8_t id, uint8_t status)
{
    uint8_t msg[3];
    msg[0] = status;                    // Status
    msg[1] = flashIndex_ & 0xFF;        // Current write offset low 8 bits
    msg[2] = (flashIndex_ >> 8) & 0xFF; // Current write offset high 8 bits

    uint16_t can_id = ((uint16_t)id << 7) | 0x11; // cmd 0x11
    can_.send(can_id, msg, 3);
}

void Bootloader::sendCRC(uint8_t id, uint32_t crc)
{
    uint8_t msg[4];
    msg[0] = (crc >> 24) & 0xFF;
    msg[1] = (crc >> 16) & 0xFF;
    msg[2] = (crc >> 8) & 0xFF;
    msg[3] = crc & 0xFF;

    uint16_t can_id = ((uint16_t)id << 7) | 0x12;
    can_.send(can_id, msg, 4);
}

void Bootloader::processCanCmd(uint8_t id, uint8_t cmd, uint8_t *data, uint8_t len)
{
    lastCmdTick_ = HAL_GetTick(); // Reset timeout when command received

    switch (cmd) {
    case 0x01: // Erase flash
        if (loaderMode_) {
            if (flash_.eraseApplication()) {
                sendConfirm(id, 0xFF);
            } else {
                sendConfirm(id, 0x00);
            }
        }
        break;
    case 0x02: // Start flash write
        if (loaderMode_) {
            if (flash_.beginWrite()) {
                flashInProgress_ = true;
                flashIndex_ = 0;
                sendConfirm(id, 0xFF);
            } else {
                sendConfirm(id, 0x00);
            }
        }
        break;
    case 0x03: // Write word
        if (loaderMode_ && flashInProgress_ && len >= 4) {
            uint32_t word = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);

            if (flash_.writeWord(word)) {
                flashIndex_ += 4;
                sendConfirm(id, 0xFF); // Write success
            } else {
                sendConfirm(id, 0x00); // Write failed
            }
        }
        break;
    case 0x04: // End flash write
        if (loaderMode_ && flashInProgress_) {
            if (flash_.endWrite()) {
                flashInProgress_ = false;
                uint32_t crc = flash_.getAppCRC();
                flash_.writeCRC(crc);
                sendConfirm(id, 0xFF);
            } else {
                sendConfirm(id, 0x00);
            }
        }
        break;
    case 0x05: // Request CRC
        if (loaderMode_) {
            uint32_t crc = flash_.getAppCRC();
            sendCRC(id, crc);
        }
        break;
    default:
        break;
    }
}

// Bootloader main loop
void Bootloader::run()
{
    const uint32_t timeout_ms = 1000; // Jump to APP after 1 seconds without command
    lastCmdTick_ = (uint32_t)HAL_GetTick();

    uint32_t lastLedTick = HAL_GetTick();
    Led led_1(1);
    Led led_2(2);
    led_1.turnOff();
    led_2.turnOff();

    while (1) {
        uint32_t now = HAL_GetTick();

        if ((uint32_t)(now - lastCmdTick_) > timeout_ms) {
            if (flash_.isAppValid()) {
                uint32_t appStack = *(uint32_t *)APP_START_ADDRESS;
                uint32_t appEntry = *(uint32_t *)(APP_START_ADDRESS + 4);

                // Complete preparation before jump
                __disable_irq(); // Disable all interrupts

                // Reset all peripherals
                HAL_RCC_DeInit();
                HAL_DeInit();

                // Reset SysTick
                SysTick->CTRL = 0;
                SysTick->LOAD = 0;
                SysTick->VAL = 0;

                // Set vector table offset
                SCB->VTOR = APP_START_ADDRESS;

                // Clear all interrupt pending bits
                for (int i = 0; i < 8; i++) {
                    NVIC->ICER[i] = 0xFFFFFFFF; // Disable all interrupts
                    NVIC->ICPR[i] = 0xFFFFFFFF; // Clear all pending bits
                }

                // Set stack pointer
                __set_MSP(appStack);

                // Jump to application
                void (*appResetHandler)(void) = (void (*)(void))appEntry;
                appResetHandler();

                // System reset if jump fails
                NVIC_SystemReset();
            } else {
                // Application invalid, reset timeout and continue waiting
                lastCmdTick_ = now;
            }
        }

        if (now - lastLedTick >= 1000) {
            lastLedTick = now;
            led_1.Toggle();
            led_2.Toggle();
        }

        HAL_Delay(1);
    }
}