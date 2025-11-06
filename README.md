# STM32 CAN BootLoader

CAN bus bootloader with firmware update and application validation.
**Tested on STM32F412CGU6**

## Features

- CAN bus communication (500Kbps)
- Firmware over-the-air updates
- CRC32 checksum verification
- Application integrity check
- Safe jump mechanism

## Memory Layout

```c
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

```



## Configuration

```c
#define RAM_START        0x20000000U  // SRAM Start
#define RAM_SIZE         (256 * 1024) // 256KB RAM
#define FLASH_SIZE       (1024 * 1024) // 1MB Flash

#define APP_START_ADDRESS 0x08008000 // Application start address
#define APP_END_ADDRESS   0x080C0000 // Application code end address
#define CRC_ADDRESS       0x080C0004 // CRC storage address
#define NODE_ID           0x02       // CAN node ID
```



## Command Set

| Command     | Code | Description            |
| :---------- | :--- | :--------------------- |
| Erase Flash | 0x01 | Erase application area |
| Start Write | 0x02 | Begin firmware write   |
| Write Data  | 0x03 | Write 4-byte data      |
| End Write   | 0x04 | End write operation    |
| Request CRC | 0x05 | Get application CRC    |

## Usage

1. Device boots into BootLoader
2. Auto-jump to application after 1 seconds (if no commands)
3. Firmware update via CAN commands
4. Run application after CRC verification

## Build Requirements

- STM32 HAL Library
- Properly configured linker script
- Application start address set to 0x08008000