# Sketches

This directory contains Arduino sketches used in the [WordClock](https://github.com/maarten-pennings/WordClock) project.
All sketches are intended for an ESP8266.

 - [WordClockLed](WordClockLed)  
   A clock that tells time, in Dutch, in steps of 5 minutes on a MAX7221 controlled 8x8 LED matrix.
   Clock is hand-set (with flash button) and runs on ESP8266 crystal.
   It also has a demo mode: every 0.1ms the clock advances 1 minute.
   
 - [NeoPixelAmps](NeoPixelAmps)  
   Sketch used to measure power used by NeoPixels.
   Used for electronics design (amps needed).
   
 - [WordClockNeo](WordClockNeo)  
   A clock that tells time, in Dutch, in steps of 5 minutes on a NeoPixel 8x8 LED matrix
   Clock is hand-set (with flash button) and runs on ESP8266 crystal
   It also has a demo mode: every 0.1ms the clock advances 1 minute

 - [TimeKeeping](TimeKeeping)  
   Keep track of time via NTP (local time zone and daylight saving time included).
   This is just time keeping. In a later step, this is integrated into `WordClockNeo`
   resulting in `WordClockSimple`.

 - [NeoPixelTest](NeoPixelTest)  
   Sketch used to test all 3 LEDs in all 64 NeoPixels.
   Written to validate the electrical design.

 - [WordClockSimple](WordClockSimple)  
   A WordClock using NeoPixels, keeping time via NTP.
   All configuration choices hardwired in sketch.  
   **This is the one that you might be looking for if you want a working but simple clock.**

 - [WordClockFull](WordClockFull)  
   A wordClock using NeoPixels, keeping time via NTP, with dynamic configuration (via web).
   Supports color changes and display animations. Has a demo mode.
   This is the one that you might be looking for if you want a full featured clock.
   See the [mian page](../README.md#13-Model-6) for a list of features.
    
