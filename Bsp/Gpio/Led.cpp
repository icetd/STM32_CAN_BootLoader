#include "Led.h"
#include "gpio.h"

Led::Led(int id) :
    m_id(id)
{
    switch (m_id) {
#define XX(id)                                                                                \
    case id:                                                                                  \
        HAL_GPIO_WritePin(LED##id##_GPIO_Port, LED##id##_Pin, GPIO_PinState::GPIO_PIN_RESET); \
        break;
        XX(1);
        XX(2);
#undef XX
    }
    m_status = false;
}
Led::~Led()
{
}

void Led::turnOn()
{
    switch (m_id) {
#define XX(id)                                                                                \
    case id:                                                                                  \
        HAL_GPIO_WritePin(LED##id##_GPIO_Port, LED##id##_Pin, GPIO_PinState::GPIO_PIN_RESET); \
        break;
        XX(1);
        XX(2);
#undef XX
    }
    m_status = true;
}
void Led::turnOff()
{
    switch (m_id) {
#define XX(id)                                                                              \
    case id:                                                                                \
        HAL_GPIO_WritePin(LED##id##_GPIO_Port, LED##id##_Pin, GPIO_PinState::GPIO_PIN_SET); \
        break;
        XX(1);
        XX(2);
#undef XX
    }

    m_status = false;
}
void Led::Toggle()
{
    switch (m_id) {
#define XX(id)                                                  \
    case id:                                                    \
        HAL_GPIO_TogglePin(LED##id##_GPIO_Port, LED##id##_Pin); \
        break;
        XX(1);
        XX(2);
#undef XX
    }

    m_status = (m_status ? false : true);
}
