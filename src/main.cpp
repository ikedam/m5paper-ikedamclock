#include "ikedam_clock.hpp"


IkedamClock g_ikedamClock;

void setup() {
  g_ikedamClock.setup();
}

void loop() {
  g_ikedamClock.loop();
}