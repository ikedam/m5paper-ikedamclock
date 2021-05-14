#pragma once
#include "utility/time_canvas.hpp"
#include "utility/float_metrics_canvas.hpp"
#include <M5EPD_Canvas.h>
#include <WiFi.h>

class IkedamClock {
public:
    IkedamClock();
    void setup();
    void loop();
    void onTimeSync(struct timeval *tv);
private:
    bool beginSpiffs();
    void setupForFirst();
    void setupForWakeup();
    void loadJpeg();
    void startWifi();
    void onWiFi(system_event_id_t event, system_event_info_t info);
    bool setupTrueTypeFont();

    enum SpiffsStatus {
        SPIFFS_NOT_INITIALIZED,
        SPIFFS_INITIALIZED,
        SPIFFS_INITIALIZE_FAILED,
    };

    struct SyncTime {
        time_t tvSec;
        suseconds_t tvUsec;
        unsigned long syncMillis;
    };

    // dummy canvas. to access font data
    M5EPD_Canvas m_canvas;
    TimeCanvas m_timeCanvas;
    FloatMetricsCanvas m_tempratureCanvas;
    FloatMetricsCanvas m_humidCanvas;
    SpiffsStatus m_spiffsStatus;

    bool m_syncTimeTrigger;
    SyncTime m_syncTime;
};
