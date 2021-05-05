#include "time_canvas.hpp"

TimeCanvas::TimeCanvas(uint32_t x, uint32_t y, uint16_t fontsize)
    : BaseCanvas(x, y, fontsize)
    , m_hour(0)
    , m_min(0)
{

}

void TimeCanvas::setTime(int8_t hour, int8_t min) {
    if (!checkUpdated(hour, min)) {
        return;
    }
    setUpdated();
    m_hour = hour;
    m_min = min;
}

uint32_t TimeCanvas::getWidth(M5EPD_Driver* pDriver) {
    return getFontsize() * 5 / 2;
}

void TimeCanvas::drawText(M5EPD_Canvas* pCanvas) {
    snprintf(m_buf, sizeof(m_buf), "%02d", m_hour);
    pCanvas->drawString(m_buf, XOFFSET * getFontsize(), YOFFSET * getFontsize());
    // Displaying : with the usual baseline cause strange looking.
    pCanvas->drawString(":", (1 + XOFFSET) * getFontsize(), (YOFFSET + -0.1f) * getFontsize());
    snprintf(m_buf, sizeof(m_buf), "%02d", m_min);
    pCanvas->drawString(m_buf, (1.5f + XOFFSET) * getFontsize(), YOFFSET * getFontsize());
}
