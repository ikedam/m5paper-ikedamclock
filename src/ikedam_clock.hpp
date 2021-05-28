#pragma once
#include "utility/time_canvas.hpp"
#include "utility/float_metrics_canvas.hpp"
#include "utility/health_canvas.hpp"
#include "utility/battery_canvas.hpp"
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
    void loadImageConfig();
    void loadImage();
    bool loadImageFromUrl();
    bool drawPng(M5EPD_Canvas* pImage, Stream* pStream);
    bool drawJpeg(M5EPD_Canvas* pImage, Stream* pStream);
    bool drawBmp(M5EPD_Canvas* pImage, Stream* pStream);
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
    BatteryCanvas m_batteryCanvas;
    HealthCanvas m_wifiHealthCanvas;
    HealthCanvas m_ntpHealthCanvas;
    HealthCanvas m_sensorHealthCanvas;
    SpiffsStatus m_spiffsStatus;

    bool m_syncTimeTrigger;
    SyncTime m_syncTime;
    String m_imageUrl;
    time_t m_nextImageLoad;
    bool m_shouldUpdate;
    time_t m_lastRefresh;
};
