#include "ikedam_clock.hpp"
#include "utility/wifi_setting.hpp"
#include <rom/tjpgd.h>
#include <M5EPD_Canvas.h>
#include <M5EPD.h>
#include <WiFi.h>
#include <hwcrypto/aes.h>
#include <HTTPClient.h>

namespace {
    const uint16_t SCREEN_WIDTH = 960;
    const uint16_t SCREEN_HEIGHT = 540;

    const uint16_t FONTSIZE_SMALL = 40;
    const uint16_t FONTSIZE_NORMAL = 100;
    const uint16_t FONTSIZE_LARGE = 200;

    const uint32_t XOFFSET = 30;
    const uint32_t YOFFSET = 60;

    const uint8_t WAKEUP_OFFSET_SEC = 10;
    const uint8_t SLEEP_MIN_SEC = 30;

    // docker run --rm python:3-slim python -c 'import os; print(os.urandom(32))'
    const unsigned char* const ENCRYPT_KEY = reinterpret_cast<const unsigned char*>("\t\xdc" "y\x9a\xc6\x18\xca\x89\xbb\x1e" "V*\xf7\t\xa8\x06\xe1\x1f\rHb\xd7" "j\x84\xbe\xb8\x1e\x03\x89\xb7\xf0" "d");

    const char* HEADERS[] = {"content-type"};
    size_t HEADERS_NUM = 1;

    // Somehow all timezone-configuring attempts completely failed...
    tm* my_localtime_r(time_t now, tm* t) {
        now += 9 * 60 * 60; // JST
        return localtime_r(&now, t);
    }

}

IkedamClock::IkedamClock()
    : m_canvas(nullptr)
    , m_timeCanvas(
        XOFFSET,
        YOFFSET,
        FONTSIZE_LARGE
    )
    , m_tempratureCanvas(
        XOFFSET,
        YOFFSET + FONTSIZE_LARGE,
        FONTSIZE_NORMAL,
        "℃"
    )
    , m_humidCanvas(
        XOFFSET,
        YOFFSET + FONTSIZE_LARGE + FONTSIZE_NORMAL,
        FONTSIZE_NORMAL,
        "%"
    )
    , m_batteryCanvas(
        0,
        0,
        FONTSIZE_SMALL
    )
    , m_wifiHealthCanvas(
        XOFFSET,
        SCREEN_HEIGHT - FONTSIZE_SMALL,
        FONTSIZE_SMALL,
        "WiFi"
    )
    , m_ntpHealthCanvas(
        XOFFSET + FONTSIZE_SMALL * 4,
        SCREEN_HEIGHT - FONTSIZE_SMALL,
        FONTSIZE_SMALL,
        "NTP"
    )
    , m_sensorHealthCanvas(
        XOFFSET + FONTSIZE_SMALL * 7.5f,
        SCREEN_HEIGHT - FONTSIZE_SMALL,
        FONTSIZE_SMALL,
        "Sensor"
    )
    , m_spiffsStatus(SPIFFS_NOT_INITIALIZED)
    , m_syncTimeTrigger(false)
    , m_nextImageLoad(false)
    , m_shouldUpdate(false)
{
    m_syncTime.syncMillis = 0;
}

bool IkedamClock::beginSpiffs() {
    switch(m_spiffsStatus) {
    case SPIFFS_INITIALIZED:
        return true;
    case SPIFFS_INITIALIZE_FAILED:
        return false;
    default:
        // pass
        break;
    }
    if (!SPIFFS.begin()) {
        m_spiffsStatus = SPIFFS_INITIALIZE_FAILED;
        return false;
    }
    m_spiffsStatus = SPIFFS_INITIALIZED;
    return true;
}

void IkedamClock::setup() {
    M5.begin(
        false,  // touchEnable
        false,  // SDEnable
        true,   // SerialEnable
        true,   // BatteryADCEnable
        true    // I2CEnable (Used for temperature and humidity)
    );
    M5.RTC.begin();
    M5.SHT30.Begin();

    M5.EPD.SetRotation(0);

    // setup fonts.
    if (!setupTrueTypeFont()) {
        m_canvas.setTextFont(8);
    }

    m_timeCanvas.setDriver(&M5.EPD);
    m_tempratureCanvas.setDriver(&M5.EPD);
    m_humidCanvas.setDriver(&M5.EPD);
    m_batteryCanvas.setDriver(&M5.EPD);
    m_wifiHealthCanvas.setDriver(&M5.EPD);
    m_ntpHealthCanvas.setDriver(&M5.EPD);
    m_sensorHealthCanvas.setDriver(&M5.EPD);

    startWifi();

    switch(esp_sleep_get_wakeup_cause()) {
        case ESP_SLEEP_WAKEUP_TIMER:
        case ESP_SLEEP_WAKEUP_GPIO:
            setupForWakeup();
            break;
        default:
            setupForFirst();
            break;
    }
}

void IkedamClock::setupForFirst() {
    M5.EPD.Clear(true);
    loadImageConfig();
    loadImage();
}

void IkedamClock::setupForWakeup() {
    rtc_time_t time;
    M5.RTC.getTime(&time);
    Serial.printf("%02d:%02d:%02d Back from sleep\n", time.hour, time.min, time.sec);
}

void IkedamClock::loadImageConfig() {
    if (!beginSpiffs()) {
        log_e("Failed to initialize SPIFFS");
        return;
    }

    const char* path = "/image.conf";

    if (!SPIFFS.exists(path)) {
        log_e(
            "No %s is available."
            " You can write the URL to load images in %s in SPIFFS.",
            path,
            path
        );
        return;
    }

    File file = SPIFFS.open(path, "rb");
    if (file.find('\n')) {
        file.seek(0);
        m_imageUrl = file.readStringUntil('\n');
    } else {
        file.seek(0);
        m_imageUrl = file.readString();
    }
    file.close();
}

void IkedamClock::loadImage() {
    if (!beginSpiffs()) {
        log_e("Failed to initialize SPIFFS");
        return;
    }
    const char* path = "/image.jpg";
    if (!SPIFFS.exists(path)) {
        log_e("Image doesnot exist: %s", path);
        return;
    }

    File file = SPIFFS.open(path);
    if (!file) {
        log_e("Failed to open: %s", path);
        return;
    }

    M5EPD_Canvas image(&M5.EPD);
    image.createCanvas(405, 540);
    if (!drawJpeg(&image, &file)) {
        log_e("Failed to write jpeg");
        return;
    }
   image.pushCanvas(555, 0, UPDATE_MODE_GC16);
}

bool IkedamClock::loadImageFromUrl() {
    if (m_imageUrl.length() == 0) {
        log_e("Image URL is not configured.");
        return false;
    }
    unsigned long start = millis();
    log_e("Load image from: %s", m_imageUrl.c_str());
    if (WiFi.status() != WL_CONNECTED) {
        log_e("WiFi is not connected: %d", WiFi.status());
        return false;
    }

    HTTPClient http;
    if (!http.begin(m_imageUrl)) {
        log_e("http.begin failed for %s", m_imageUrl.c_str());
        return false;
    }
    http.collectHeaders(HEADERS, HEADERS_NUM);

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        log_e("HTTP failed for status=%d", httpCode);
        http.end();
        return false;
    }

    String contentType = http.header("content-type");
    if (contentType.length() == 0) {
        log_e("No content-type specified");
        http.end();
        return false;
    }
    if (http.getSize() <= 0) {
        log_e("No content-length specified");
        http.end();
        return false;
    }
    unsigned long now = millis();
    log_e(
        "Connected to %s (took %lu secs)",
        m_imageUrl.c_str(),
        (now - start) / 1000
    );

    M5EPD_Canvas image(&M5.EPD);
    image.createCanvas(405, 540);

    bool result = false;
    if (contentType.equals("image/png")) {
        result = drawPng(&image, http.getStreamPtr());
    } else if (contentType.equals("image/jpeg")) {
        result = drawJpeg(&image, http.getStreamPtr());
    } else if (contentType.equals("image/bmp")) {
        result = drawBmp(&image, http.getStreamPtr());
    } else {
        log_e("Unknown content-type: %s", contentType.c_str());
    }
    http.end();

    now = millis();
    log_e(
        "Completed drawing (took %lu secs for total)",
        (now - start) / 1000
    );

    if (!result) {
        log_e("Error in drwawing image data");
        return false;
    }
    image.pushCanvas(555, 0, UPDATE_MODE_GC16);
    return result;
}

bool IkedamClock::setupTrueTypeFont() {
    if (!beginSpiffs()) {
        log_e("Failed to initialize SPIFFS");
        return false;
    }
    esp_err_t err = m_canvas.loadFont("/font.ttf", SPIFFS);
    if (err != ESP_OK) {
        log_e("Failed to load font: %d", err);
        return false;
    }
    m_canvas.createRender(FONTSIZE_NORMAL);
    m_canvas.createRender(FONTSIZE_LARGE);
    m_canvas.createRender(FONTSIZE_SMALL);
    return true;
}

namespace {
    IkedamClock* pThis = NULL;
    void _onTimeSync(struct timeval *tv) {
        pThis->onTimeSync(tv);
    }
}

void IkedamClock::startWifi() {
    if (!beginSpiffs()) {
        log_e("Failed to initialize SPIFFS");
        return;
    }

    WifiSetting setting(
        "/wifi.conf",
        ENCRYPT_KEY,
        256
    );
    if (!setting.readAndEncrypt()) {
        log_e("Failed to read Wifi config");
        return;
    }

    // Do NOT store configurations to flash!
    WiFi.persistent(false);
    WiFi.begin(setting.getEssid(), setting.getPassword());
    WiFi.onEvent([this](system_event_id_t event, system_event_info_t info) -> void {
        this->onWiFi(event, info);
    });
    pThis = this;
    sntp_set_time_sync_notification_cb(_onTimeSync);
}

void IkedamClock::onWiFi(system_event_id_t event, system_event_info_t info) {
    switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
        {
            // actually, JST doesn't make sense in this application...
            configTzTime("JST", "0.pool.ntp.org", "1.pool.ntp.org", "2.pool.ntp.org");
        }
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        {
            WiFi.begin();
        }
        break;
    default:
        log_e("Unexpected wifi event: %d", event);
        break;
    }
}

void IkedamClock::onTimeSync(struct timeval *pTv) {
    // Calling RTC methods here conflicts with loop().
    m_syncTimeTrigger = true;
    m_syncTime.tvSec = pTv->tv_sec;
    m_syncTime.tvUsec = pTv->tv_usec;
    m_syncTime.syncMillis = millis();
    // settimeofday(pTv, NULL);
}

void IkedamClock::loop() {
    if (m_syncTimeTrigger) {
        long millisOffset = static_cast<long>(millis() - m_syncTime.syncMillis);
        millisOffset += m_syncTime.tvUsec / 1000;
        time_t now = m_syncTime.tvSec + millisOffset / 1000;
        tm t;
        my_localtime_r(now, &t);
        rtc_date_t date(t.tm_wday, t.tm_mon + 1, t.tm_mday, t.tm_year + 1900);
        rtc_time_t time(t.tm_hour, t.tm_min, t.tm_sec);
        M5.RTC.setTime(&time);
        M5.RTC.setDate(&date);

        log_e("Sync: %d/%02d/%02d %02d:%02d:%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
        m_syncTimeTrigger = false;
    }
    time_t now;
    time(&now);
    tm t;
    my_localtime_r(now, &t);

    M5.BtnP.read();

    if (!m_timeCanvas.checkUpdated(t.tm_hour, t.tm_min) && M5.BtnP.releasedFor(1000)) {
        delay(1000);
        return;
    }

    uint8_t err = M5.SHT30.UpdateData();
    if (err == I2C_ERROR_OK) {
        m_tempratureCanvas.setMetrics(M5.SHT30.GetTemperature());
        m_humidCanvas.setMetrics(M5.SHT30.GetRelHumidity());
    }

    m_timeCanvas.setTime(t.tm_hour, t.tm_min);

    m_batteryCanvas.setVoltage(M5.getBatteryVoltage());
    m_wifiHealthCanvas.setStatus(WiFi.status() == WL_CONNECTED);
    m_ntpHealthCanvas.setStatus(
        m_syncTime.syncMillis != 0
        && now - m_syncTime.tvSec < 3600 + 300 // period + buffer
    );
    m_sensorHealthCanvas.setStatus(err == I2C_ERROR_OK);

    m_timeCanvas.drawIfUpdated();
    m_tempratureCanvas.drawIfUpdated();
    m_humidCanvas.drawIfUpdated();
    m_batteryCanvas.drawIfUpdated();
    m_wifiHealthCanvas.drawIfUpdated();
    m_ntpHealthCanvas.drawIfUpdated();
    m_sensorHealthCanvas.drawIfUpdated();
    if (
        m_syncTime.syncMillis != 0
        && t.tm_sec < 5
        && m_nextImageLoad <= now
    ) {
        log_e(
            "%d/%02d/%02d %02d:%02d:%02d",
            t.tm_year + 1900,
            t.tm_mon + 1,
            t.tm_mday,
            t.tm_hour,
            t.tm_min,
            t.tm_sec
        );
        loadImageFromUrl();
        m_nextImageLoad = now + 30 * 60;
        m_nextImageLoad -= m_nextImageLoad % (30 * 60);
        m_shouldUpdate = true;
    } else if (
        t.tm_sec < 5
        && (
            m_shouldUpdate
            || now - m_lastRefresh > 3600
        )
    ) {
        M5.EPD.UpdateFull(UPDATE_MODE_GC16);
        m_shouldUpdate = false;
        m_lastRefresh = now;
    }

    /*
    M5.RTC.getTime(&time);

    uint8_t remain_sec = 60 - time.sec;
    if (!m_timeCanvas.checkUpdated(time.hour, time.min) && remain_sec > WAKEUP_OFFSET_SEC + SLEEP_MIN_SEC) {
        esp_sleep_enable_timer_wakeup((remain_sec - WAKEUP_OFFSET_SEC) * 1000 * 1000);
        Serial.printf("%02d:%02d:%02d Go to sleep\n", time.hour, time.min, time.sec);
        esp_sleep_enable_gpio_wakeup();
        esp_deep_sleep_start();
    }
    */
}