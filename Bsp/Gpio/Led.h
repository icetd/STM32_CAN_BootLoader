#ifndef LED_H
#define LED_H

class Led
{
public:
    Led(int id);
    ~Led();

    void turnOn();
    void turnOff();
    void Toggle();

    bool getStatus()
    {
        return m_status;
    }

    void setStatus(bool status)
    {
        m_status = status;
    }

private:
    bool m_status;
    int m_id;
};

#endif