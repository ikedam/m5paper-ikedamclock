#include "ikedam_clock.hpp"
#include <rom/tjpgd.h>
#include <M5EPD_Canvas.h>
#include <M5EPD.h>


namespace {
    const uint16_t FONTSIZE_SMALL = 40;
    const uint16_t FONTSIZE_NORMAL = 100;
    const uint16_t FONTSIZE_LARGE = 200;

    const uint32_t XOFFSET = 40;
    const uint32_t YOFFSET = 40;

    const uint8_t WAKEUP_OFFSET_SEC = 10;
    const uint8_t SLEEP_MIN_SEC = 30;
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
{
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
    if (!SPIFFS.begin(false)) {
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

void IkedamClock::loop() {
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