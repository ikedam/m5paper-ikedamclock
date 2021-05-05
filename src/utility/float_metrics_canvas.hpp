#pragma once

#include "base_canvas.hpp"
#include <stdint.h>

class FloatMetricsCanvas: public BaseCanvas {
public:
    FloatMetricsCanvas(uint32_t x, uint32_t y, uint16_t fontsize, const char* suffix);
    virtual ~FloatMetricsCanvas();

    void setMetrics(float metrics);
protected:
    virtual uint32_t getWidth(M5EPD_Driver* pDriver) ;
    virtual void drawText(M5EPD_Canvas* pCanvas);
private:
    float m_metrics;
    char* m_buf;
    size_t m_length;
    String m_suffix;
};
