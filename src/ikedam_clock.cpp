#include "ikedam_clock.hpp"
#include <M5EPD.h>
#include <M5EPD_Canvas.h>

IkedamClock::IkedamClock()
    : m_canvas(nullptr)
{
}

void IkedamClock::setup() {
    M5.begin(
        false,  // touchEnable
        false,  // SDEnable
        false,  // SerialEnable
        false,  // BatteryADCEnable (TODO: what's this?)
        true    // I2CEnable (Used for temperature and humidity)
    );
    M5.RTC.begin();
    M5.SHT30.Begin();
    m_canvas.setDriver(&M5.EPD);
    M5.EPD.SetRotation(0);
    M5.EPD.Clear(true);
    m_canvas.createCanvas(960, 540);
    m_canvas.setTextSize(100);
}

void IkedamClock::loop() {
    rtc_date_t date;
    rtc_time_t time;
    M5.RTC.getDate(&date);
    M5.RTC.getTime(&time);

    M5.SHT30.UpdateData();
    float temprature = M5.SHT30.GetTemperature();
    float humid = M5.SHT30.GetRelHumidity();

    {
        char timeBuf[6];
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", time.hour, time.min);
        m_canvas.drawString(timeBuf, 0, 0);
    }

    {
        char tempBuf[6];
        snprintf(tempBuf, sizeof(tempBuf), "%2.1f*C", temprature);
        m_canvas.drawString(tempBuf, 0, 100);
    }

    {
        char humidBuf[5];
        snprintf(humidBuf, sizeof(humidBuf), "%2.1f%%", humid);
        m_canvas.drawString(humidBuf, 0, 200);
    }

    m_canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);

    delay(1000);
}