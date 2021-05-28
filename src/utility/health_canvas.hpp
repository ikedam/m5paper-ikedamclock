#pragma once

#include "base_canvas.hpp"

class HealthCanvas: public BaseCanvas {
public:
    HealthCanvas(uint32_t x, uint32_t y, uint16_t fontsize, const char* label);
    virtual ~HealthCanvas();

    void setStatus(bool ok);
protected:
    virtual uint32_t getWidth(M5EPD_Driver* pDriver);
    virtual void drawText(M5EPD_Canvas* pCanvas);
private:
    bool m_status;
    String m_label;
    char* m_pBuf;
    size_t m_bufSize;
};
