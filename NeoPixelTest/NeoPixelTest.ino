// NeoPixelTest - Sketch used to test all 3 LEDs in all 64 NeoPixels
#include <Adafruit_NeoPixel.h>

// Neopixels run at 5v0; required signal level is at 70%. 
// Since 5v0*70%=3v5 supplying 5v0 and signalling 3v3 is a border case.
// I power Neopixels from the 3v3 and GND pins from the ESP8266 board.
// But for the bigger Neopixel rings, I use the VIN (5V0) pin of the RobotDyn board.

// From https://learn.adafruit.com/adafruit-neopixel-uberguide/basic-connections
// When connecting NeoPixels live, always connect ground (–) before anything else.
// Add a large capacitor (1000 µF, 6.3V or higher) across the + and – terminals. This prevents the initial onrush of current from damaging the pixels. 
// Add a ~470 ohm resistor between MCU and data-in. This prevents spikes on the data line to damage the first pixel. 

#define PIN            D6
#define NUMPIXELS      64
#define DELAY          200

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

int state_leds;
uint32_t state_color;

void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nWelcome to NeoPixelTest - testing all LEDs in all NeoPixels\n");
  pixels.begin(); // This initializes the NeoPixel library.

  state_leds=NUMPIXELS+1;
  state_color=0x11000000;
  pixels.clear();
}

void loop() {
  if( state_color==0 ) return;
  
  if( state_leds>NUMPIXELS ) {
    Serial.printf("New series\n");
    pixels.clear();
    state_leds= 0;
    state_color>>=8;
    if( state_color==0 ) {
      for(int i=56; i<64; i++ ) pixels.setPixelColor(i,0x111111);
      for(int i=40; i<56; i++ ) pixels.setPixelColor(i,0x111100);
      for(int i=24; i<40; i++ ) pixels.setPixelColor(i,0x001111);
      for(int i= 8; i<24; i++ ) pixels.setPixelColor(i,0x110011);
      for(int i= 0; i< 8; i++ ) pixels.setPixelColor(i,0x111111);
      pixels.show();
    }
  }

  for(int i=0; i<state_leds; i++ ) pixels.setPixelColor(i,state_color);
  pixels.show();
  Serial.printf("  showing %d leds with %06x\n",state_leds, state_color);

  delay(50);
  state_leds++;
}
