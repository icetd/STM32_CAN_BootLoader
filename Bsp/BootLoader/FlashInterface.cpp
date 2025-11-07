#include "FlashInterface.h"
#include "BootLoader.h"

#if defined(STM32F4xx)
#include "stm32f4xx_hal.h"
#elif defined(STM32F1xx)
#include "stm32f1xx_hal.h"
#endif

FlashInterface::FlashInterface(uint32_t appStart, uint32_t appEnd) : appStart_(appStart), appEnd_(appEnd), flashAddress_(appStart)
{
    // Ensure application start address is after bootloader
    if (appStart_ < APP_START_ADDRESS) {
        appStart_ = APP_START_ADDRESS;
    }
}

#if defined(STM32F4xx)
static uint32_t addrToSector(uint32_t addr) {
#if defined(STM32F412Cx) || defined(STM32F412Rx) || defined(STM32F412Vx) || defined(STM32F412Zx)
    // STM32F412 Sector Layout (1MB Flash)
    if (addr < 0x08004000) return FLASH_SECTOR_0;      // 16KB
    else if (addr < 0x08008000) return FLASH_SECTOR_1; // 16KB  
    else if (addr < 0x0800C000) return FLASH_SECTOR_2; // 16KB
    else if (addr < 0x08010000) return FLASH_SECTOR_3; // 16KB
    else if (addr < 0x08020000) return FLASH_SECTOR_4; // 64KB
    else if (addr < 0x08040000) return FLASH_SECTOR_5; // 128KB
    else if (addr < 0x08060000) return FLASH_SECTOR_6; // 128KB
    else if (addr < 0x08080000) return FLASH_SECTOR_7; // 128KB
    else if (addr < 0x080A0000) return FLASH_SECTOR_8; // 128KB
    else if (addr < 0x080C0000) return FLASH_SECTOR_9; // 128KB
    else if (addr < 0x080E0000) return FLASH_SECTOR_10; // 128KB
    else if (addr < 0x08100000) return FLASH_SECTOR_11; // 128KB
    else return FLASH_SECTOR_11; // Safe return
#else
    // Standard F4 Sector Layout
    if (addr < 0x08004000) return FLASH_SECTOR_0;
    else if (addr < 0x08008000) return FLASH_SECTOR_1;
    else if (addr < 0x0800C000) return FLASH_SECTOR_2;
    else if (addr < 0x08010000) return FLASH_SECTOR_3;
    else if (addr < 0x08020000) return FLASH_SECTOR_4;
    else if (addr < 0x08040000) return FLASH_SECTOR_5;
    else if (addr < 0x08060000) return FLASH_SECTOR_6;
    else if (addr < 0x08080000) return FLASH_SECTOR_7;
    else if (addr < 0x080A0000) return FLASH_SECTOR_8;
    else if (addr < 0x080C0000) return FLASH_SECTOR_9;
    else if (addr < 0x080E0000) return FLASH_SECTOR_10;
    else return FLASH_SECTOR_11;
#endif
}

static bool calculateSectors(uint32_t startAddr, uint32_t endAddr,
                             uint32_t &startSector, uint32_t &nbSectors)
{
    // Protect bootloader sectors (sector 0-1: 0x08000000 - 0x08008000)
    const uint32_t BOOTLOADER_END_SECTOR = FLASH_SECTOR_1;

    startSector = addrToSector(startAddr);
    uint32_t endSector = addrToSector(endAddr - 1);

    // Force start from sector 2 to protect bootloader
    if (startSector <= BOOTLOADER_END_SECTOR) {
        startSector = BOOTLOADER_END_SECTOR + 1;
    }

    // Recalculate sector count
    if (startSector <= endSector) {
        nbSectors = endSector - startSector + 1;
    } else {
        nbSectors = 0;
        return false;
    }

    return (nbSectors > 0);
}
#endif

#if defined(STM32F1xx)
static uint32_t addrToPage(uint32_t addr)
{
    return (addr - FLASH_BASE) / FLASH_PAGE_SIZE;
}

static bool calculatePages(uint32_t startAddr, uint32_t endAddr,
                           uint32_t &startPage, uint32_t &nbPages)
{
    startPage = addrToPage(startAddr);
    uint32_t endPage = addrToPage(endAddr - 1);

    // Protect bootloader pages (first 32 pages = 32KB)
    uint32_t protectedPages = 32; // 32 pages * 1KB = 32KB
    if (startPage < protectedPages) {
        startPage = protectedPages;
    }

    if (startPage <= endPage) {
        nbPages = endPage - startPage + 1;
    } else {
        nbPages = 0;
        return false;
    }

    return (nbPages > 0);
}
#endif

bool FlashInterface::eraseApplication()
{
    // Critical protection check
    if (appStart_ < APP_START_ADDRESS) {
        return false;
    }

    if (HAL_FLASH_Unlock() != HAL_OK) {
        return false;
    }

    // Clear all error flags
#if defined(STM32F1xx)
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR);
#else
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
#endif

    FLASH_EraseInitTypeDef eraseInit;
    uint32_t sectorError = 0;
    bool success = false;

#if defined(STM32F4xx)
    uint32_t startSector = 0, nbSectors = 0;

    // Ensure erase range includes CRC area
    uint32_t eraseEnd = (CRC_ADDRESS > appEnd_) ? CRC_ADDRESS : appEnd_;

    if (calculateSectors(appStart_, eraseEnd, startSector, nbSectors)) {
        eraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
        eraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;
        eraseInit.Sector = startSector;
        eraseInit.NbSectors = nbSectors;

        if (HAL_FLASHEx_Erase(&eraseInit, &sectorError) == HAL_OK) {
            success = true;
        }
    }

#elif defined(STM32F1xx)
    uint32_t startPage = 0, nbPages = 0;

    uint32_t eraseEnd = (CRC_ADDRESS > appEnd_) ? CRC_ADDRESS : appEnd_;

    if (calculatePages(appStart_, eraseEnd, startPage, nbPages)) {
        eraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
        eraseInit.PageAddress = FLASH_BASE + (startPage * FLASH_PAGE_SIZE);
        eraseInit.NbPages = nbPages;

        if (HAL_FLASHEx_Erase(&eraseInit, &sectorError) == HAL_OK) {
            success = true;
        }
    }
#endif

    HAL_FLASH_Lock();

    if (success) {
        flashAddress_ = appStart_;
    }

    return success;
}

bool FlashInterface::beginWrite()
{
    flashAddress_ = appStart_;

    if (HAL_FLASH_Unlock() != HAL_OK) {
        return false;
    }

    // Clear all error flags
#if defined(STM32F1xx)
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR);
#else
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
#endif

    return true;
}

bool FlashInterface::writeWord(uint32_t word)
{
    // Reserve last 4 bytes for application length
    if (flashAddress_ >= appEnd_ - 4) {
        return false;
    }

    // Check address alignment
    if (flashAddress_ & 0x3) {
        return false;
    }

#if defined(STM32F1xx)
    uint16_t halfWord1 = word & 0xFFFF;
    uint16_t halfWord2 = (word >> 16) & 0xFFFF;

    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, flashAddress_, halfWord1) != HAL_OK) {
        return false;
    }

    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, flashAddress_ + 2, halfWord2) != HAL_OK) {
        return false;
    }
#else
    // Program word for F4
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, flashAddress_, word) != HAL_OK) {
        return false;
    }
#endif

    // Verify programming
    uint32_t readback = *(volatile uint32_t *)flashAddress_;
    if (readback != word) {
        return false;
    }

    flashAddress_ += 4;
    return true;
}

bool FlashInterface::endWrite()
{
    // Store application length at the end of flash area
    uint32_t appLength = flashAddress_ - appStart_;

    // Check if we have space to store length
    if (appEnd_ - appStart_ < 4) {
        HAL_FLASH_Lock();
        return false;
    }

    uint32_t lengthAddress = appEnd_ - 4;

    // Program length
#if defined(STM32F1xx)
    uint16_t halfWord1 = appLength & 0xFFFF;
    uint16_t halfWord2 = (appLength >> 16) & 0xFFFF;

    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, lengthAddress, halfWord1) != HAL_OK) {
        HAL_FLASH_Lock();
        return false;
    }

    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, lengthAddress + 2, halfWord2) != HAL_OK) {
        HAL_FLASH_Lock();
        return false;
    }
#else
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, lengthAddress, appLength) != HAL_OK) {
        HAL_FLASH_Lock();
        return false;
    }
#endif

    // Strict multiple verification (before locking)
    uint32_t readback1 = *(volatile uint32_t *)lengthAddress;
    uint32_t readback2 = *(volatile uint32_t *)lengthAddress;
    uint32_t readback3 = *(volatile uint32_t *)lengthAddress;

    // Lock after verification
    HAL_FLASH_Lock();

    // All three reads must be consistent to consider success
    return (readback1 == appLength) && (readback2 == appLength) && (readback3 == appLength);
}

uint32_t FlashInterface::getAppLength() const
{
    uint32_t len = *(volatile uint32_t *)(appEnd_ - 4);

    // Validate length
    if (len == 0xFFFFFFFF || len > (appEnd_ - appStart_ - 4)) {
        return 0;
    }

    return len;
}

uint32_t FlashInterface::getAppCRC() const
{
    uint32_t crc = 0xFFFFFFFF;
    uint32_t len = getAppLength();

    if (len == 0 || len > (appEnd_ - appStart_)) {
        return 0xFFFFFFFF;
    }

    // Calculate CRC32 for application data
    for (uint32_t addr = appStart_; addr < appStart_ + len; addr += 4) {
        uint32_t data = *(volatile uint32_t *)addr;
        crc ^= data;

        for (int i = 0; i < 32; i++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }

    return ~crc;
}

bool FlashInterface::writeCRC(uint32_t crc)
{
    if (HAL_FLASH_Unlock() != HAL_OK) {
        return false;
    }

    // Clear all error flags
#if defined(STM32F1xx)
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR);

#else
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
#endif

    // Program CRC
#if defined(STM32F1xx)
    uint16_t halfWord1 = crc & 0xFFFF;
    uint16_t halfWord2 = (crc >> 16) & 0xFFFF;

    HAL_StatusTypeDef status1 = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, CRC_ADDRESS, halfWord1);
    HAL_StatusTypeDef status2 = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, CRC_ADDRESS + 2, halfWord2);

    HAL_StatusTypeDef status = (status1 == HAL_OK && status2 == HAL_OK) ? HAL_OK : HAL_ERROR;
#else
    HAL_StatusTypeDef status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, CRC_ADDRESS, crc);
#endif

    // Immediate read verification
    uint32_t readback = *(volatile uint32_t *)CRC_ADDRESS;

    HAL_FLASH_Lock();

    return (status == HAL_OK) && (readback == crc);
}

uint32_t FlashInterface::readCRC() const
{
    return *(volatile uint32_t *)CRC_ADDRESS;
}

bool FlashInterface::checkCRC() const
{
    uint32_t crcCalc = getAppCRC();
    uint32_t crcStored = readCRC();

    // Handle case where CRC hasn't been written yet
    if (crcStored == 0xFFFFFFFF) {
        return false;
    }

    return (crcCalc == crcStored);
}

bool FlashInterface::isAppValid() const
{
    // 1. Check if CRC matches
    if (!checkCRC()) {
        return false;
    }

    // 2. Check if stack pointer is within valid RAM range
    uint32_t appStack = *(uint32_t *)APP_START_ADDRESS;
    if (appStack < RAM_START || appStack > (RAM_START + RAM_SIZE)) {
        return false;
    }

    // 3. Check if reset vector address is within Flash range
    uint32_t appEntry = *(uint32_t *)(APP_START_ADDRESS + 4);
    if (appEntry < APP_START_ADDRESS || appEntry >= (APP_START_ADDRESS + FLASH_SIZE)) {
        return false;
    }

    // 4. Check if the first few words of application are all 0xFFFFFFFF (indicating unprogrammed)
    for (int i = 0; i < 8; i++) {
        if (*(uint32_t *)(APP_START_ADDRESS + i * 4) != 0xFFFFFFFF) {
            return true; // At least some part is programmed
        }
    }

    return false; // First 32 bytes are all 0xFFFFFFFF, likely empty Flash
}