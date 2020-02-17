// WordClockNeo.ino - A clock that tells time, in Dutch, in steps of 5 minutes on a neopixel 8x8 LED matrix
//   Clock is hand-set (with flash button) and runs on ESP8266 crystal
//   It also has a demo mode: every 0.1ms the clock advances 1 minute
// Idea http://www.espruino.com/Tiny+Word+Clock
// This project https://github.com/maarten-pennings/WordClock/blob/master/README.md

#include <Adafruit_NeoPixel.h>


// The LEDs are numbered from top to bottom, from right to left (this matches the 8x8 matrix order).
// Assumption: IN side of the LED PCB is on the (top) right hand (looking to the front)
// 63 62 61 60 59 58 57 56  -  KWARTIEN
// 55 54 53 52 51 50 49 48  -  VYF*VOOR 
// 47 46 45 44 43 42 41 40  -  OVERHALF
// 39 38 37 36 35 34 33 32  -  ELFZDRIE # ZEVEN diagonal
// 31 30 29 28 27 26 25 24  -  TIENEGEN
// 23 22 21 20 19 18 17 16  -  VIERZVYF # ZES diagonal
// 15 14 13 12 11 10  9  8  -  ACHTWEEN
//  7  6  5  4  3  2  1  0  -  TWAALFSN
//                          -           # UUR missing

// All words: indexes of the LEDs that form a word (-1 terminates)
const int model_min_vijf[]    = {55,54,53,-1};
const int model_min_kwart[]   = {63,62,61,60,59,-1};
const int model_min_tien[]    = {59,58,57,56,-1};
const int model_min_voor[]    = {51,50,49,48,-1};
const int model_min_over[]    = {47,46,45,44,-1};
const int model_min_half[]    = {43,42,41,40,-1};
const int model_hour_elf[]    = {39,38,37,-1};
const int model_hour_drie[]   = {35,34,33,32,-1};
const int model_hour_zeven[]  = {36,27,18,9,0,-1};
const int model_hour_tien[]   = {31,30,29,28,-1};
const int model_hour_negen[]  = {28,27,26,25,24,-1};
const int model_hour_vier[]   = {23,22,21,20,-1}; 
const int model_hour_vijf[]   = {18,17,16,-1}; 
const int model_hour_zes[]    = {19,10,1,-1};
const int model_hour_acht[]   = {15,14,13,12,-1};
const int model_hour_twee[]   = {12,11,10,9,-1};
const int model_hour_een[]    = {10,9,8,-1};
const int model_hour_twaalf[] = {7,6,5,4,3,2,-1};
const int model_hour_empty[]  = {-1}; 


// Frame ===========================================================================================
// The frame_hours in an array for easy indexing and integer hour to an index array
// Note that 0:xx and 12:30 are supported (as well as an empty hour using 14:xx)
const int * frame_hours[] = {model_hour_twaalf,model_hour_een,model_hour_twee,model_hour_drie,model_hour_vier,model_hour_vijf,model_hour_zes,model_hour_zeven,model_hour_acht,model_hour_negen,model_hour_tien,model_hour_elf,model_hour_twaalf,model_hour_een,model_hour_empty,model_hour_empty};

// This section controls the 8x8 NeoPixel matrix

// Neopixels run at 5v0; required signal level is at 70%. 
// Since 5v0*70%=3v5 supplying 5v0 and signalling 3v3 is a border case.
// I power Neopixels from a standalone power supply.

// From https://learn.adafruit.com/adafruit-neopixel-uberguide/basic-connections
// When connecting NeoPixels live, always connect ground (–) before anything else.
// Add a large capacitor (1000 µF, 6.3V or higher) across the + and – terminals. This prevents the initial onrush of current from damaging the pixels. 
// Add a ~470 ohm resistor between MCU and data-in. This prevents spikes on the data line to damage the first pixel. 

#define FRAME_PIN_DATA   D6
#define FRAME_NUMPIXELS  64

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(FRAME_NUMPIXELS, FRAME_PIN_DATA, NEO_GRB + NEO_KHZ800);


void frame_init() {
    pinMode(FRAME_PIN_DATA, OUTPUT);
}

void frame_add_word(uint32_t color, const int word[] ) {
  const int * letter= word; // Point to the first NeoPixel in `word`
  while( *letter>=0 ) {
    pixels.setPixelColor(*letter,color);
    letter++; // Point to the next NeoPixel in `word`
  }
}

void frame_set_word(uint32_t color, const int word[] ) {
  pixels.clear();
  frame_add_word(color,word);
  pixels.show();
}


#define COL_MIN 0x000033
#define COL_HOUR 0x330000
#define COL_HELP 0x003300

// Sets the NeoPixels for the time specified by `hour` and `min`.
// E.g. 7:35 -> vijf over half acht
void frame_set_time(int hour, int min ) {
  pixels.clear();
  switch( min ) {
    case 0: case 1: case 2: 
      frame_add_word(0x220000,frame_hours[hour]);
      break;  
    case 3: case 4: case 5: case 6: case 7: 
      frame_add_word(COL_MIN,model_min_vijf); frame_add_word(COL_HELP,model_min_over); frame_add_word(COL_HOUR,frame_hours[hour]);
      break;  
    case 8: case 9: case 10: case 11: case 12: 
      frame_add_word(COL_MIN,model_min_tien); frame_add_word(COL_HELP,model_min_over); frame_add_word(COL_HOUR,frame_hours[hour]);
      break;  
    case 13: case 14: case 15: case 16: case 17: 
      frame_add_word(COL_MIN,model_min_kwart); frame_add_word(COL_HELP,model_min_over); frame_add_word(COL_HOUR,frame_hours[hour]);
      break;  
    case 18: case 19: case 20: case 21: case 22: 
      frame_add_word(COL_MIN,model_min_tien); frame_add_word(COL_HELP,model_min_voor); frame_add_word(COL_MIN,model_min_half); frame_add_word(COL_HOUR,frame_hours[hour+1]);
      break;  
    case 23: case 24: case 25: case 26: case 27: 
      frame_add_word(COL_MIN,model_min_vijf); frame_add_word(COL_HELP,model_min_voor); frame_add_word(COL_MIN,model_min_half); frame_add_word(COL_HOUR,frame_hours[hour+1]);
      break;  
    case 28: case 29: case 30: case 31: case 32: 
      frame_add_word(COL_MIN,model_min_half); frame_add_word(COL_HOUR,frame_hours[hour+1]);
      break;  
    case 33: case 34: case 35: case 36: case 37: 
      frame_add_word(COL_MIN,model_min_vijf); frame_add_word(COL_HELP,model_min_over); frame_add_word(COL_MIN,model_min_half); frame_add_word(COL_HOUR,frame_hours[hour+1]);
      break;  
    case 38: case 39: case 40: case 41: case 42: 
      frame_add_word(COL_MIN,model_min_tien); frame_add_word(COL_HELP,model_min_over); frame_add_word(COL_MIN,model_min_half); frame_add_word(COL_HOUR,frame_hours[hour+1]);
      break;  
    case 43: case 44: case 45: case 46: case 47: 
      frame_add_word(COL_MIN,model_min_kwart); frame_add_word(COL_HELP,model_min_voor); frame_add_word(COL_HOUR,frame_hours[hour+1]);
      break;  
    case 48: case 49: case 50: case 51: case 52: 
      frame_add_word(COL_MIN,model_min_tien); frame_add_word(COL_HELP,model_min_voor); frame_add_word(COL_HOUR,frame_hours[hour+1]);
      break;  
    case 53: case 54: case 55: case 56: case 57: 
      frame_add_word(COL_MIN,model_min_vijf); frame_add_word(COL_HELP,model_min_voor); frame_add_word(COL_HOUR,frame_hours[hour+1]);
      break;  
    case 58: case 59:  
      frame_add_word(COL_HOUR,frame_hours[hour+1]);
      break;  
  }
  pixels.show();
}


// Button ==========================================================================
// This section implements functions that control a button.
// The button is the 'flash' button on most ESP8266 NodeMCU boards.
// The app should call scan() regularly to poll the button state.
// The state can then be observed via but_isdown() and but_wentdown().

#define BUT_PIN D3


int but_crnt; // current state - from last but_scan()
int but_prev; // state from previous but_scan()


void but_scan() {
  but_prev= but_crnt;
  but_crnt= digitalRead(BUT_PIN)==0; // Low active
}


void but_init(void) {
  pinMode(BUT_PIN, INPUT);
  but_scan();
  but_scan();
}


int but_isdown() {
  return but_crnt;
}


int but_wentdown() {
  return but_crnt & !but_prev;
}


// Main ==========================================================================

// 63 62 61 60 59 58 57 56
// 55 54 53 52 51 50 49 48
// 47 46 45 44 43 42 41 40
// 39 38 37 36 35 34 33 32
// 31 30 29 28 27 26 25 24
// 23 22 21 20 19 18 17 16
// 15 14 13 12 11 10  9  8
//  7  6  5  4  3  2  1  0

// Indexes of the LEDs that form a symbol (-1 terminates)
const int main_frame_modetime[]   = {61,60,59,58, 54,49, 47,39,31,23, 40,32,24,16, 14,9, 5,4,3,2, 44,36,28,27,26, -1};
const int main_frame_modefast[]   = {55,54,46,45,37,36,29,28,22,21,15,14, 51,50,42,41,33,32,25,24,18,17,11,10, -1};


int      main_mode;
int      main_hour;
int      main_min;
uint32_t main_last;


void main_init( void ) {
  // Hand set mode
  main_mode= 1;
  Serial.printf("Increment MODE by repeatedly pressing the button (long press to quit)\n");
  #define MODE_UPDATE()  Serial.printf("mode= %s\n",main_mode?"time":"fast"); frame_set_word( 0x220000, main_mode?main_frame_modetime:main_frame_modefast );
  MODE_UPDATE();
  while( 1 ) {
    but_scan();
    if( but_wentdown() ) {
      main_mode= (main_mode+1) % 2;
      MODE_UPDATE();
      main_last= millis();
      delay(100); // debounce
    }
    if( but_isdown() && millis()-main_last>1000 ) {
      main_mode= (main_mode+1) % 2;
      MODE_UPDATE();
      break;
    }
    yield();
  }

  // Wait for button release
  Serial.printf("Release\n");
  while( but_isdown() ) { but_scan(); yield(); }

  // Hand set hours
  main_hour= 1;
  Serial.printf("Increment HOUR by repeatedly pressing the button (long press to quit)\n");
  #define HOUR_UPDATE()  Serial.printf("hour= %02d\n",main_hour); frame_set_time(main_hour,0 );
  HOUR_UPDATE();
  while( 1 ) {
    but_scan();
    if( but_wentdown() ) {
      main_hour= (main_hour+1) % 12;
      HOUR_UPDATE();
      main_last= millis();
      delay(100); // debounce
    }
    if( but_isdown() && millis()-main_last>1000 ) {
      main_hour= (main_hour+11) % 12;
      HOUR_UPDATE();
      break;
    }
    yield();
  }

  // Wait for button release
  Serial.printf("Release\n");
  while( but_isdown() ) { but_scan(); yield(); }

  // Hand set minutes
  main_min= 5;
  Serial.printf("Increment MINUTES by repeatedly pressing the button (long press to quit)\n");
  #define UPDATE_MIN() Serial.printf("min = %02d\n",main_min); frame_set_time(14,main_min );
  UPDATE_MIN();
  while( 1 ) {
    but_scan();
    if( but_wentdown() ) {
      main_min = (main_min+5) % 60;
      UPDATE_MIN();
      main_last= millis();
      delay(100); // debounce
    }
    if( but_isdown() && millis()-main_last>1000 ) {
      main_min= (main_min+55) % 60;
      UPDATE_MIN();
      break;
    }
    yield();
  }
  
  // Wait for button release
  Serial.printf("Release\n");
  while( but_isdown() ) { but_scan(); yield(); }
}

void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nWord clock - NeoPixels\n");

  frame_init();
  but_init();
  main_init(); // Let user set mode, hour, min
  main_last= millis();
}

int main_prev_min;
int main_prev_hour;

void loop() {
  Serial.printf("%2d:%02d\n", main_hour,main_min); 
  if( main_prev_min!=main_min || main_prev_hour!=main_hour) {
    frame_set_time(main_hour,main_min);
    main_prev_min=main_min;
    main_prev_hour=main_hour;
  }
  if( main_mode==0 ) {
    // Fast
    main_min= main_min+1; 
    if( main_min==60 ) { main_min= 0; main_hour= (main_hour+1)%12; }
    delay(200);
  } else {
    // Clock
    if( millis()-main_last > 60000 ) {
      main_min= main_min+1; 
      if( main_min==60 ) { main_min= 0; main_hour= (main_hour+1)%12; }
      main_last= millis();
    }
    delay(1000);
  }
}
