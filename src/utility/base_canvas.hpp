#pragma once

#include <stdint.h>
#include <M5EPD_Canvas.h>

class BaseCanvas {
public:
    void setDriver(M5EPD_Driver* pDriver);

    bool drawIfUpdated();
    void draw();
    void setUpdated() { m_updated = true; }
    uint16_t getFontsize() const { return m_fontsize; }
protected:
    BaseCanvas(uint32_t x, uint32_t y, uint16_t fontsize);
    virtual uint32_t getWidth(M5EPD_Driver* pDriver) = 0;
    virtual void drawText(M5EPD_Canvas* pCanvas) = 0;

    // Somehow, truetype font requires some more margins.
    // They are offsets to cancel the margin.
    static const float XOFFSET;
    static const float YOFFSET;
private:
    bool m_updated;
    uint32_t m_x;
    uint32_t m_y;
    uint32_t m_w;
    uint32_t m_h;
    uint16_t m_fontsize;
    M5EPD_Canvas m_canvas;
};
