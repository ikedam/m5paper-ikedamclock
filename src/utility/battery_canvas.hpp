#pragma once

#include "base_canvas.hpp"

class BatteryCanvas: public BaseCanvas {
public:
    BatteryCanvas(uint32_t x, uint32_t y, uint16_t fontsize);

    void setVoltage(uint32_t voltage);
protected:
    virtual uint32_t getWidth(M5EPD_Driver* pDriver);
    virtual void drawText(M5EPD_Canvas* pCanvas);
private:
    uint8_t getPercentage() const;
    uint32_t m_voltage;
    char m_buf[6];
};