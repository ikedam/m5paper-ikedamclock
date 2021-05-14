#include "ikedam_clock.hpp"
#include "utility/wifi_setting.hpp"
#include <rom/tjpgd.h>
#include <M5EPD_Canvas.h>
#include <M5EPD.h>
#include <WiFi.h>
#include <hwcrypto/aes.h>

namespace {
    const uint16_t FONTSIZE_SMALL = 40;
    const uint16_t FONTSIZE_NORMAL = 100;
    const uint16_t FONTSIZE_LARGE = 200;

    const uint32_t XOFFSET = 40;
    const uint32_t YOFFSET = 40;

    const uint8_t WAKEUP_OFFSET_SEC = 10;
    const uint8_t SLEEP_MIN_SEC = 30;

    // docker run --rm python:3-slim python -c 'import os; print(os.urandom(32))'
    const unsigned char* const ENCRYPT_KEY = reinterpret_cast<const unsigned char*>("\t\xdc" "y\x9a\xc6\x18\xca\x89\xbb\x1e" "V*\xf7\t\xa8\x06\xe1\x1f\rHb\xd7" "j\x84\xbe\xb8\x1e\x03\x89\xb7\xf0" "d");

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
    , m_spiffsStatus(SPIFFS_NOT_INITIALIZED)
    , m_syncTimeTrigger(false)
{
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
    loadJpeg();
}

void IkedamClock::setupForWakeup() {
    rtc_time_t time;
    M5.RTC.getTime(&time);
    Serial.printf("%02d:%02d:%02d Back from sleep\n", time.hour, time.min, time.sec);
}

/*
namespace {
    uint32_t jpgReadFile(JDEC* decoder, uint8_t* buf, uint32_t len) {
        File* file = static_cast<File*>(decoder->device);
        if (buf) {
            return file->read(buf, len);
        } else {
            file->seek(len, SeekCur);
        }
        return len;
    }
}
*/

void IkedamClock::loadJpeg() {
    if (!beginSpiffs()) {
        log_e("Failed to initialize SPIFFS");
        return;
    }
    /*
    const char* path = "/image.jpg";
    const int16_t width = 405;
    const int16_t height = 540;
    if (!SPIFFS.exists(path)) {
        Serial.printf("Image doesnot exist: %s\n", path);
        return;
    }

    // load metadata
    File file = SPIFFS.open(path);
    if (!file) {
        Serial.printf("Failed to open: %s\n", path);
        return;
    }

    JDEC decoder;
    {
        uint8_t work[3100];
        JRESULT jres = jd_prepare(
            &decoder,
            jpgReadFile,
            work,
            sizeof(work),
            &file
        );
        file.close();
        if (jres != JDR_OK) {
            Serial.printf("Failed to read image: %d\n", jres);
            return;
        }
    }

    float xscale = static_cast<float>(width) / decoder.width;
    float yscale = static_cast<float>(height) / decoder.height;
    float scale = (xscale < yscale) ? xscale : yscale;

    uint16_t scaledWidth = decoder.width * scale;
    uint16_t scaledHeight = decoder.height * scale;
    */

    M5EPD_Canvas image(&M5.EPD);
    image.createCanvas(405, 540);
    image.drawJpgFile(
        SPIFFS,
        "/image.jpg",
        0,
        0,
        405,
        540,
        0,
        0,
        JPEG_DIV_4
    );
    image.pushCanvas(555, 0, UPDATE_MODE_GC16);
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
    case SYSTEM_EVENT_STA_CONNECTED:
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

void IkedamClock::onTimeSync(struct timeval *tv) {
    // Calling RTC methods here conflicts with loop().
    m_syncTimeTrigger = true;
    m_syncTime.tvSec = tv->tv_sec;
    m_syncTime.tvUsec = tv->tv_usec;
    m_syncTime.syncMillis = millis();
}

void IkedamClock::loop() {
    if (m_syncTimeTrigger) {
        long millisOffset = static_cast<long>(millis() - m_syncTime.syncMillis);
        millisOffset += m_syncTime.tvUsec / 1000;
        time_t now = m_syncTime.tvSec + millisOffset / 1000 + 9 * 60 * 60;  // JST
        tm t;
        gmtime_r(&now, &t);
        rtc_date_t date(t.tm_wday, t.tm_mon + 1, t.tm_mday, t.tm_year + 1900);
        rtc_time_t time(t.tm_hour, t.tm_min, t.tm_sec);
        M5.RTC.setTime(&time);
        M5.RTC.setDate(&date);

        log_e("Sync: %d/%02d/%02d %02d:%02d:%02d", date.year, date.mon, date.day, time.hour, time.min, time.sec);
        m_syncTimeTrigger = false;
    }
    rtc_time_t time;
    M5.RTC.getTime(&time);
    M5.BtnP.read();

    if (!m_timeCanvas.checkUpdated(time.hour, time.min) && M5.BtnP.releasedFor(1000)) {
        delay(1000);
        return;
    }

    rtc_date_t date;
    M5.RTC.getDate(&date);

    uint8_t err = M5.SHT30.UpdateData();
    if (err == I2C_ERROR_OK) {
        m_tempratureCanvas.setMetrics(M5.SHT30.GetTemperature());
        m_humidCanvas.setMetrics(M5.SHT30.GetRelHumidity());
    }

    m_timeCanvas.setTime(time.hour, time.min);

    bool updated = m_timeCanvas.drawIfUpdated();
    m_tempratureCanvas.drawIfUpdated();
    m_humidCanvas.drawIfUpdated();
    if (updated) {
        // M5.EPD.UpdateFull(UPDATE_MODE_DU);
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