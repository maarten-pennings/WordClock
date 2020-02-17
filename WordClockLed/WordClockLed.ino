// WordClockLed.ino - A clock that tells time, in Dutch, in steps of 5 minutes on a MAX7221 controlled 8x8 LED matrix
//   Clock is hand-set (with flash button) and runs on ESP8266 crystal
//   It also has a demo mode: every 0.1ms the clock advances 1 minute
// Idea http://www.espruino.com/Tiny+Word+Clock
// Demo https://www.youtube.com/watch?v=YDhCZarNm9g
// MAX722 datasheet https://datasheets.maximintegrated.com/en/ds/MAX7219-MAX7221.pdf
// datasheet explained https://www.youtube.com/watch?v=tFDiMw5QKZQ


// Model(s) =================================================================================
// A model describes where each of the letters and words of a WordCLock is.
// This section must setup an array for each word. 
// The array must contain indexes (on the 8x8 matrix, so 0..63) of the LEDs that form a word (-1 terminates the array).
// There are words to denote the minutes
//   model_min_vijf, model_min_kwart, model_min_tien, model_min_voor, model_min_over, model_min_half
// and there are words for the hours
//   model_hour_een, model_hour_twee, model_hour_drie, model_hour_vier, model_hour_vijf, model_hour_zes, 
//   model_hour_zeven, model_hour_acht, model_hour_negen, model_hour_tien, model_hour_elf, model_hour_twaalf, model_hour_empty
// (the empty word is used for hand setting the minutes)


#if 0


// All words horizontal, but two paired letters (vIEr, twaaLF), and one split word (Z-E-S)
#define MODEL_NAME "Model 1 - Maarten"


// The LEDs are numbered from top to bottom, from right to left (this matches the 8x8 matrix order).
// Assumption: IN side of the LED PCB is on the (top) right hand (looking to the front)
//  7  6  5  4  3  2  1  0  -  VYFKWART
// 15 14 13 12 11 10  9  8  -  TIENVOOR # space missing
// 23 22 21 20 19 18 17 16  -  OVERHALF # space missing
// 31 30 29 28 27 26 25 24  -  ACHTWEEN
// 39 38 37 36 35 34 33 32  -  TIENEGEN
// 47 46 45 44 43 42 41 40  -  ZDRIELFS
// 55 54 53 52 51 50 49 48  -  V#RZEVEN #=IE
// 63 62 61 60 59 58 57 56  -  VYFTWAA# #=LF
//                          -           # UUR missing


// All words: indexes of the LEDs that form a word (-1 terminates)
const int model_min_vijf[]    ={7,6,5,-1};
const int model_min_kwart[]   ={4,3,2,1,0,-1};
const int model_min_tien[]    ={15,14,13,12,-1};
const int model_min_voor[]    ={11,10,9,8,-1};
const int model_min_over[]    ={23,22,21,20,-1};
const int model_min_half[]    ={19,18,17,16,-1};
const int model_hour_acht[]   ={31,30,29,28,-1};
const int model_hour_twee[]   ={28,27,26,25,-1};
const int model_hour_een[]    ={26,25,24,-1};
const int model_hour_tien[]   ={39,38,37,36,-1};
const int model_hour_negen[]  ={36,35,34,33,32,-1};
const int model_hour_zes[]    ={47,43,40,-1};
const int model_hour_drie[]   ={46,45,44,43,-1};
const int model_hour_elf[]    ={43,42,41,-1};
const int model_hour_vier[]   ={55,54,53,-1}; // 49=IE
const int model_hour_zeven[]  ={52,51,50,49,48,-1};
const int model_hour_vijf[]   ={63,62,61,-1}; 
const int model_hour_twaalf[] ={60,59,58,57,56,-1};  // 63=LF
const int model_hour_empty[]  ={-1}; 


#endif


#if 1


// No split words, no paired letters, but two words diagonal 
#define MODEL_NAME "Model 2 - Marc"


// The LEDs are numbered from top to bottom, from right to left (this matches the 8x8 matrix order).
// Assumption: IN side of the LED PCB is on the (top) right hand (looking to the front)
//  7  6  5  4  3  2  1  0  -  KWARTIEN
// 15 14 13 12 11 10  9  8  -  VYFJVOOR # J is a dummy (can maybe spell 'JARIG')
// 23 22 21 20 19 18 17 16  -  OVERHALF
// 31 30 29 28 27 26 25 24  -  ELFZDRIE # ZEVEN diagonal
// 39 38 37 36 35 34 33 32  -  TIENEGEN
// 47 46 45 44 43 42 41 40  -  VIERZVYF # ZES diagonal
// 55 54 53 52 51 50 49 48  -  ACHTWEEN
// 63 62 61 60 59 58 57 56  -  TWAALFSN
//                          -           # UUR missing

// All words: indexes of the LEDs that form a word (-1 terminates)
const int model_min_vijf[]    = {15,14,13,-1};
const int model_min_kwart[]   = {7,6,5,4,3,-1};
const int model_min_tien[]    = {3,2,1,0,-1};
const int model_min_voor[]    = {11,10,9,8,-1};
const int model_min_over[]    = {23,22,21,20,-1};
const int model_min_half[]    = {19,18,17,16,-1};
const int model_hour_elf[]    = {31,30,29,-1};
const int model_hour_drie[]   = {27,26,25,24,-1};
const int model_hour_zeven[]  = {28,35,42,49,56,-1};
const int model_hour_tien[]   = {39,38,37,36,-1};
const int model_hour_negen[]  = {36,35,34,33,32,-1};
const int model_hour_vier[]   = {47,46,45,44,-1}; 
const int model_hour_vijf[]   = {42,41,40,-1}; 
const int model_hour_zes[]    = {43,50,57,-1};
const int model_hour_acht[]   = {55,54,53,52,-1};
const int model_hour_twee[]   = {52,51,50,49,-1};
const int model_hour_een[]    = {50,49,48,-1};
const int model_hour_twaalf[] = {63,62,61,60,59,58,-1};
const int model_hour_empty[]  = {-1}; 


#endif


// Frame ===========================================================================================
// A frame buffer is a uint64_t that tells which of the 8x8 LEDs should switch on.
// In our case, a typically frame holds the bits for the words that make up the time.
// For example, when the time is 11:05, the LEDs that make up 'vijf', the LEDs that make up 'over', and the LEDs that make up 'elf'
// have a 1 in the frame buffer, so that the text reads 'vijf over elf'


// The model_hour in an array for easy indexing and integer hour to an index array
// Note that 0:xx and 12:30 are supported (as well as an empty hour using 14:xx)
const int * frame_hours[] = {model_hour_twaalf,model_hour_een,model_hour_twee,model_hour_drie,model_hour_vier,model_hour_vijf,model_hour_zes,model_hour_zeven,model_hour_acht,model_hour_negen,model_hour_tien,model_hour_elf,model_hour_twaalf,model_hour_een,model_hour_empty,model_hour_empty};


// A `frame` (buffer) is encoded in a 64 bit unsigned `.
// This function sets bits (switches on LEDs) in `frame`, 
// namely those listed in array `word` (which is terminated with -1).
void frame_add_word(uint64_t*frame, const int word[]) {
  const int * bit= word; // Point to the first LED index in `word`
  while( *bit>=0 ) {
    *frame |= 1LLU << *bit;
    bit++; // Point to the next LED index in `word`
  }
}


// Returns a frame showing the time specified by `hour` and `min`.
// E.g. 7:35 -> vijf over half acht
uint64_t frame_set_time(int hour, int min ) {
  uint64_t frame= 0;
  switch( min ) {
    case 0: case 1: case 2: 
      frame_add_word(&frame,frame_hours[hour]);
      break;  
    case 3: case 4: case 5: case 6: case 7: 
      frame_add_word(&frame,model_min_vijf); frame_add_word(&frame,model_min_over); frame_add_word(&frame,frame_hours[hour]);
      break;  
    case 8: case 9: case 10: case 11: case 12: 
      frame_add_word(&frame,model_min_tien); frame_add_word(&frame,model_min_over); frame_add_word(&frame,frame_hours[hour]);
      break;  
    case 13: case 14: case 15: case 16: case 17: 
      frame_add_word(&frame,model_min_kwart); frame_add_word(&frame,model_min_over); frame_add_word(&frame,frame_hours[hour]);
      break;  
    case 18: case 19: case 20: case 21: case 22: 
      frame_add_word(&frame,model_min_tien); frame_add_word(&frame,model_min_voor); frame_add_word(&frame,model_min_half); frame_add_word(&frame,frame_hours[hour+1]);
      break;  
    case 23: case 24: case 25: case 26: case 27: 
      frame_add_word(&frame,model_min_vijf); frame_add_word(&frame,model_min_voor); frame_add_word(&frame,model_min_half); frame_add_word(&frame,frame_hours[hour+1]);
      break;  
    case 28: case 29: case 30: case 31: case 32: 
      frame_add_word(&frame,model_min_half); frame_add_word(&frame,frame_hours[hour+1]);
      break;  
    case 33: case 34: case 35: case 36: case 37: 
      frame_add_word(&frame,model_min_vijf); frame_add_word(&frame,model_min_over); frame_add_word(&frame,model_min_half); frame_add_word(&frame,frame_hours[hour+1]);
      break;  
    case 38: case 39: case 40: case 41: case 42: 
      frame_add_word(&frame,model_min_tien); frame_add_word(&frame,model_min_over); frame_add_word(&frame,model_min_half); frame_add_word(&frame,frame_hours[hour+1]);
      break;  
    case 43: case 44: case 45: case 46: case 47: 
      frame_add_word(&frame,model_min_kwart); frame_add_word(&frame,model_min_voor); frame_add_word(&frame,frame_hours[hour+1]);
      break;  
    case 48: case 49: case 50: case 51: case 52: 
      frame_add_word(&frame,model_min_tien); frame_add_word(&frame,model_min_voor); frame_add_word(&frame,frame_hours[hour+1]);
      break;  
    case 53: case 54: case 55: case 56: case 57: 
      frame_add_word(&frame,model_min_vijf); frame_add_word(&frame,model_min_voor); frame_add_word(&frame,frame_hours[hour+1]);
      break;  
    case 58: case 59:  
      frame_add_word(&frame,frame_hours[hour+1]);
      break;  
  }
  return frame;
}


// Display ==========================================================================
// This section controls the 8x8 LED matrix, which is connected to a MAX7221.
// The MAX7221 has a serial (data/clock) input (and a chip enable)


// Pins from ESP8266 to the MAX7221 controlled 8x8 LED matrix
//      DSP_VCC  3V3   // VCC pin of display is connected to 3V3 of ESP8266
//      DSP_GND  GND   // GND pin of display is connected to GND of ESP8266
#define DSP_DIN   D7   // DIN pin of display is connected to D7 of ESP8266
#define DSP_CS    D2   // CS  pin of display is connected to D2 of ESP8266 
#define DSP_CLK   D5   // CLK pin of display is connected to D5 of ESP8266


// Writes `data` to `addr` in the MAX7221 (which controls the 8x8 LED matrix)
// `addr` is the address of the register
// `data` is the value for the register
void dsp_out(byte addr, byte data) {
  digitalWrite(DSP_CS, LOW);

  shiftOut(DSP_DIN, DSP_CLK, MSBFIRST, addr );
  shiftOut(DSP_DIN, DSP_CLK, MSBFIRST, data );
  
  digitalWrite(DSP_CS, HIGH);
}


// Writes frame buffer to the MAX7221 (which controls the 8x8 LED matrix)
void dsp_img(uint64_t frame) {
  for(int i=1; i<=8; i++ ) {
    byte row = frame & 0xFF;
    dsp_out(i,row);
    frame >>= 8;
  }
}


// Configures ESP8266 pins that are connected to the MAX7221
// Configure MAX7221 to individually addressable LEDs
void dsp_init(void) {
  digitalWrite(DSP_CS, HIGH);
  pinMode(DSP_DIN, OUTPUT);
  pinMode(DSP_CS, OUTPUT);
  pinMode(DSP_CLK, OUTPUT);

  // Configure MAX7221 to use all LEDs at medium brightness, individually addressable
  dsp_out(0x0f, 0x00); // 'Display Test': normal mode (not test)
  dsp_out(0x0c, 0x00); // 'Shutdown'    : shutdown ("off")
  dsp_out(0x0b, 0x07); // 'Scan Limit'  : rows digits 0 thru 7
  dsp_out(0x0a, 0x07); // 'Intensity'   : medium brightness
  dsp_out(0x09, 0x00); // 'Decode mode' : no-decode for all digits

  // Send empty frame buffer
  dsp_img( 0 );
  // Switch display on
  dsp_out(0x0c, 0x01); // 'Shutdown'    : normal operation ("on")  
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

// Hand coded frame buffers for the two modes - note: up-side-down
#define main_frame_modetime 0b\
00111100\
01000010\
10000001\
10011101\
10010001\
10010001\
01000010\
00111100LLU
#define main_frame_modefast 0b\
00000000\
11001100\
01100110\
00110011\
00110011\
01100110\
11001100\
00000000LLU


int      main_mode;
int      main_hour;
int      main_min;
uint32_t main_last;


void main_init( void ) {
  // Hand set mode
  main_mode= 1;
  Serial.printf("Increment MODE by repeatedly pressing the button (long press to quit)\n");
  #define MODE_UPDATE()  Serial.printf("mode= %s\n",main_mode?"time":"fast"); dsp_img( main_mode?main_frame_modetime:main_frame_modefast );
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
  #define HOUR_UPDATE()  Serial.printf("hour= %02d\n",main_hour); dsp_img( frame_set_time(main_hour,0) | (0xffLLU<<0) );
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
  #define UPDATE_MIN() Serial.printf("min = %02d\n",main_min); dsp_img( frame_set_time(14,main_min) | (0xffLLU<<56) );
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
  Serial.printf("\n\nWord clock LED - using '%s'\n", MODEL_NAME);
  dsp_init();
  but_init();
  main_init(); // Let user set mode, hour, min
  main_last= millis();
}


void loop() {
  Serial.printf("%2d:%02d\n", main_hour,main_min); 
  dsp_img( frame_set_time(main_hour,main_min) );
  if( main_mode==0 ) {
    // Fast
    main_min= main_min+1; 
    if( main_min==60 ) { main_min= 0; main_hour= (main_hour+1)%12; }
    delay(100);
  } else {
    // Clock
    if( millis()-main_last > 60000 ) {
      main_min= main_min+1; 
      if( main_min==60 ) { main_min= 0; main_hour= (main_hour+1)%12; }
      main_last= millis();
    }
    delay(900);
    dsp_img(0);
    delay(100);
  }
}
