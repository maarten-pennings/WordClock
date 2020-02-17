// NeoPixelAmps - measuring power uszed by NeoPixels 
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
#define NUMPIXELS      16
#define DELAY          200

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

int state_leds;
int state_phase;
uint32_t state_color;

void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nWelcome to NeoPixelAmps - measuring power\n");
  pixels.begin(); // This initializes the NeoPixel library.

  state_leds=0;
  state_phase=0;
  state_color=0xffffff; // 0 1 2 3. 4 5 6 7. 8 9 a b. c d e f.
}

void loop() {
  if( state_phase==0 ) {
    pixels.clear();
    for(int i=0; i<state_leds; i++ ) pixels.setPixelColor(i,state_color);
    pixels.show();
    Serial.printf("Showing %d leds with %06x\n",state_leds, state_color);
    state_phase=1; 
  } else if( state_phase==1 ) {
    int val=Serial.read();
    if( val=='\n' ) { state_leds++; state_phase=0; }
  } 
}
