#pragma once
#include "utility/time_canvas.hpp"
#include "utility/float_metrics_canvas.hpp"
#include <M5EPD_Canvas.h>

class IkedamClock {
public:
    IkedamClock();
    void setup();
    void loop();
private:
    void setupForFirst();
    void setupForWakeup();
    void loadJpeg();
    bool setupTrueTypeFont();
    // dummy canvas. to access font data
    M5EPD_Canvas m_canvas;
    TimeCanvas m_timeCanvas;
    FloatMetricsCanvas m_tempratureCanvas;
    FloatMetricsCanvas m_humidCanvas;
};
