#include "battery_canvas.hpp"

BatteryCanvas::BatteryCanvas(uint32_t x, uint32_t y, uint16_t fontsize, bool withVoltage)
    : BaseCanvas(x, y, fontsize)
    , m_voltage(0)
    , m_withVoltage(withVoltage)
{

}

void BatteryCanvas::setVoltage(uint32_t voltage) {
    if (m_voltage == voltage) {
        return;
    }
    setUpdated();
    m_voltage = voltage;
}

uint32_t BatteryCanvas::getWidth(M5EPD_Driver* pDriver) {
    // 3 letters for icon
    // 3 letters for figures
    // 1 letters for %
    // (with voltage)
    // 1 letters for padding
    // 4 letters for figures
    // 2 letters for "mV"
    return m_withVoltage ? (getFontsize() * 14 / 2) : (getFontsize() * 7 / 2);
}

uint8_t BatteryCanvas::getPercentage() const {
    // normalize to 3300 - 4350
    const uint32_t min = 3300;
    const uint32_t max = 4350;
    if (m_voltage <= min) {
        return 0;
    }
    if (m_voltage >= max) {
        return 100;
    }
    return (m_voltage - min) * 100 / (max - min);
}

void BatteryCanvas::drawText(M5EPD_Canvas* pCanvas) {
    const uint16_t margin = 3;
    uint8_t percent = getPercentage();
    pCanvas->drawRect(margin, margin, getFontsize() * 1.5f - margin * 2, getFontsize() - margin * 2, 15);
    uint16_t width = (getFontsize() * 1.5f - margin * 4) * percent / 100;
    if (width > 0) {
        pCanvas->fillRect(margin * 2, margin * 2, width, getFontsize() - margin * 4, 15);
    }
    snprintf(m_buf, sizeof(m_buf), "%u%%", percent);
    pCanvas->drawString(m_buf, (1.5f + XOFFSET) * getFontsize(), YOFFSET * getFontsize());
    if (m_withVoltage) {
        snprintf(m_voltageBuf, sizeof(m_voltageBuf), "%umV", m_voltage);
        pCanvas->drawString(m_voltageBuf, (4 + XOFFSET) * getFontsize(), YOFFSET * getFontsize());
    }
}
