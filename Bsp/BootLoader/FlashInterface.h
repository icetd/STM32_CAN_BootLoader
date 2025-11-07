#pragma once
#include <cstdint>

class FlashInterface
{
public:
    FlashInterface(uint32_t appStart, uint32_t appEnd);

    bool eraseApplication();
    bool beginWrite();
    bool writeWord(uint32_t word);
    bool endWrite();
    uint32_t getAppCRC() const;

    bool writeCRC(uint32_t crc);
    uint32_t readCRC() const;
    bool checkCRC() const;
    bool isAppValid() const;
    uint32_t getAppLength() const;

private:
    uint32_t flashAddress_;
    uint32_t appStart_;
    uint32_t appEnd_;
};
