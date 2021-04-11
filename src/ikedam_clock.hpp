#include <M5EPD_Canvas.h>

class IkedamClock {
public:
    IkedamClock();
    void setup();
    void loop();
private:
    M5EPD_Canvas m_canvas;
};