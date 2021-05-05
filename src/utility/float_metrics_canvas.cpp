#include "float_metrics_canvas.hpp"

FloatMetricsCanvas::FloatMetricsCanvas(uint32_t x, uint32_t y, uint16_t fontsize, const char* suffix)
    : BaseCanvas(x, y, fontsize)
    , m_metrics(0.0f)
    , m_suffix(suffix)
{
    m_length = 4 + m_suffix.length();
    m_buf = new char[m_length + 1];
}

FloatMetricsCanvas::~FloatMetricsCanvas()
{
    delete[] m_buf;
}

void FloatMetricsCanvas::setMetrics(float metrics) {
    if (m_metrics == metrics) {
        return;
    }
    setUpdated();
    m_metrics = metrics;
}

uint32_t FloatMetricsCanvas::getWidth(M5EPD_Driver* pDriver) {
    return getFontsize() * m_length / 2;
}

void FloatMetricsCanvas::drawText(M5EPD_Canvas* pCanvas) {
    snprintf(m_buf, m_length + 1, "%2.1f%s", m_metrics, m_suffix.c_str());
    pCanvas->drawString(m_buf, XOFFSET * getFontsize(), YOFFSET * getFontsize());
}
