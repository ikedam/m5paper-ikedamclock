#pragma once

#include "base_canvas.hpp"
#include <stdint.h>

class TimeCanvas: public BaseCanvas {
public:
    TimeCanvas(uint32_t x, uint32_t y, uint16_t fontsize);

    void setTime(int8_t hour, int8_t min);
    bool checkUpdated(int8_t hour, int8_t min) {
        return m_hour != hour || m_min != min;
    }
protected:
    virtual uint32_t getWidth(M5EPD_Driver* pDriver);
    virtual void drawText(M5EPD_Canvas* pCanvas);
private:
    int8_t m_hour;
    int8_t m_min;
    char m_buf[3];
};
