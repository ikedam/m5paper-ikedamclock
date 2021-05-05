#include "base_canvas.hpp"

const float BaseCanvas::XOFFSET = 0.0f;
const float BaseCanvas::YOFFSET = -0.1f;

BaseCanvas::BaseCanvas(uint32_t x, uint32_t y, uint16_t fontsize)
    : m_updated(true)
    , m_x(x)
    , m_y(y)
    , m_w(0)
    , m_h(0)
    , m_fontsize(fontsize)
    , m_canvas(nullptr)
{
}

void BaseCanvas::setDriver(M5EPD_Driver* pDriver) {
    m_canvas.setDriver(pDriver);
    m_canvas.setTextSize(m_fontsize);
    m_h = m_fontsize;   // doesn't support textHeight()
    m_w = getWidth(pDriver);
    m_canvas.createCanvas(m_w, m_h);
}

bool BaseCanvas::drawIfUpdated() {
    if (!m_updated) {
        return false;
    }
    m_updated = false;
    draw();
    return true;
}

void BaseCanvas::draw() {
    m_canvas.fillCanvas(0);
    m_canvas.setTextColor(15);
    drawText(&m_canvas);
    m_canvas.pushCanvas(m_x, m_y, UPDATE_MODE_DU);
}
