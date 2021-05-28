#include "health_canvas.hpp"

HealthCanvas::HealthCanvas(uint32_t x, uint32_t y, uint16_t fontsize, const char* label)
    : BaseCanvas(x, y, fontsize)
    , m_status(false)
    , m_label(label)
{
    m_bufSize = m_label.length() + 4;
    m_pBuf = new char[m_bufSize];
}

HealthCanvas::~HealthCanvas() {
    delete[] m_pBuf;
}

void HealthCanvas::setStatus(bool status) {
    if (m_status == status) {
        return;
    }
    setUpdated();
    m_status = status;
}

uint32_t HealthCanvas::getWidth(M5EPD_Driver* pDriver) {
    return getFontsize() * (m_bufSize - 1) / 2;
}

void HealthCanvas::drawText(M5EPD_Canvas* pCanvas) {
    if (m_status) {
        snprintf(m_pBuf, m_bufSize, "%s:OK", m_label.c_str());
    } else {
        snprintf(m_pBuf, m_bufSize, "%s:NG", m_label.c_str());
    }
    pCanvas->drawString(m_pBuf, XOFFSET * getFontsize(), YOFFSET * getFontsize());
}
