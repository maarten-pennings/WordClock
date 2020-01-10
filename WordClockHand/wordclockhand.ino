// wordclockhand.ino - A clock that tells time, in Dutch, in steps of 5 minutes on a MAX7221 controlled 8x8 LED matrix
//   Clock is hand-set (with flash button) and runs on ESP8266 crystal
// Idea http://www.espruino.com/Tiny+Word+Clock
// Demo https://www.youtube.com/watch?v=YDhCZarNm9g
// MAX722 datasheet https://datasheets.maximintegrated.com/en/ds/MAX7219-MAX7221.pdf
// datasheet explained https://www.youtube.com/watch?v=tFDiMw5QKZQ


// Clock ============================================================================


// The LEDs are numbered from top to bottom, from right to left (this matches the 8x8 matrix order).
// Assumption: IN side is on the right hand
//  7  6  5  4  3  2  1  0  -  VYFKWART
// 15 14 13 12 11 10  9  8  -  TIENVOOR # space missing
// 23 22 21 20 19 18 17 16  -  OVERHALF
// 31 30 27 28 27 26 25 24  -  ACHTWEEN
// 39 38 37 36 35 34 33 32  -  TIENEGEN
// 47 46 45 44 43 42 41 40  -  ZDRIELFS
// 55 54 53 52 51 50 49 48  -  V#RZEVEN #=IE
// 63 62 61 60 59 58 57 56  -  VYFTWAA# #=LF
//                          -           # UUR missing


// All words
const int clk_min_vijf[]     ={7,6,5,-1};
const int clk_min_kwart[]   ={4,3,2,1,0,-1};
const int clk_min_tien[]    ={15,14,13,12,-1};
const int clk_min_voor[]    ={11,10,9,8,-1};
const int clk_min_over[]    ={23,22,21,20,-1};
const int clk_min_half[]    ={19,18,17,16,-1};
const int clk_hour_acht[]   ={31,30,29,28,-1};
const int clk_hour_twee[]   ={28,27,26,25,-1};
const int clk_hour_een[]    ={26,25,24,-1};
const int clk_hour_tien[]   ={39,38,37,36,-1};
const int clk_hour_negen[]  ={36,35,34,33,32,-1};
const int clk_hour_zes[]    ={47,43,40,-1};
const int clk_hour_drie[]   ={46,45,44,43,-1};
const int clk_hour_elf[]    ={43,42,41,-1};
const int clk_hour_vier[]   ={55,54,53,-1}; // 49=IE
const int clk_hour_zeven[]  ={52,51,50,49,48,-1};
const int clk_hour_vijf[]   ={63,62,61,-1}; 
const int clk_hour_twaalf[] ={60,59,58,57,56,-1};  // 63=LF
const int clk_hour_empty[]  ={-1}; 


// The clk_hours in an array, note that 0:xx and 12:30 are supported (as well as an empty hour)
const int * clk_hours[] = {clk_hour_twaalf,clk_hour_een,clk_hour_twee,clk_hour_drie,clk_hour_vier,clk_hour_vijf,clk_hour_zes,clk_hour_zeven,clk_hour_acht,clk_hour_negen,clk_hour_tien,clk_hour_elf,clk_hour_twaalf,clk_hour_een,clk_hour_empty,clk_hour_empty};


// A frame is encoded in a 64 bit mask.
// This function sets bits (switches on leds) in `mask`, 
// namely those listed in array `bits` (which is terminated with -1)
void clk_addbits(uint64_t*mask, const int bits[]) {
  const int * bit= bits;
  while( *bit>=0 ) {
    *mask |= 1LLU << *bit;
    bit++;
  }
}


// Returns a frame showing the time specified by `hour` and `min`.
// E.g. 7:35 -> vijf over half acht
uint64_t clk_timemask(int hour, int min ) {
  uint64_t mask= 0;
  switch( min ) {
    case 0: case 1: case 2: 
      clk_addbits(&mask,clk_hours[hour]);
      break;  
    case 3: case 4: case 5: case 6: case 7: 
      clk_addbits(&mask,clk_min_vijf); clk_addbits(&mask,clk_min_over); clk_addbits(&mask,clk_hours[hour]);
      break;  
    case 8: case 9: case 10: case 11: case 12: 
      clk_addbits(&mask,clk_min_tien); clk_addbits(&mask,clk_min_over); clk_addbits(&mask,clk_hours[hour]);
      break;  
    case 13: case 14: case 15: case 16: case 17: 
      clk_addbits(&mask,clk_min_kwart); clk_addbits(&mask,clk_min_over); clk_addbits(&mask,clk_hours[hour]);
      break;  
    case 18: case 19: case 20: case 21: case 22: 
      clk_addbits(&mask,clk_min_tien); clk_addbits(&mask,clk_min_voor); clk_addbits(&mask,clk_min_half); clk_addbits(&mask,clk_hours[hour+1]);
      break;  
    case 23: case 24: case 25: case 26: case 27: 
      clk_addbits(&mask,clk_min_vijf); clk_addbits(&mask,clk_min_voor); clk_addbits(&mask,clk_min_half); clk_addbits(&mask,clk_hours[hour+1]);
      break;  
    case 28: case 29: case 30: case 31: case 32: 
      clk_addbits(&mask,clk_min_half); clk_addbits(&mask,clk_hours[hour+1]);
      break;  
    case 33: case 34: case 35: case 36: case 37: 
      clk_addbits(&mask,clk_min_vijf); clk_addbits(&mask,clk_min_over); clk_addbits(&mask,clk_min_half); clk_addbits(&mask,clk_hours[hour+1]);
      break;  
    case 38: case 39: case 40: case 41: case 42: 
      clk_addbits(&mask,clk_min_tien); clk_addbits(&mask,clk_min_over); clk_addbits(&mask,clk_min_half); clk_addbits(&mask,clk_hours[hour+1]);
      break;  
    case 43: case 44: case 45: case 46: case 47: 
      clk_addbits(&mask,clk_min_kwart); clk_addbits(&mask,clk_min_voor); clk_addbits(&mask,clk_hours[hour+1]);
      break;  
    case 48: case 49: case 50: case 51: case 52: 
      clk_addbits(&mask,clk_min_tien); clk_addbits(&mask,clk_min_voor); clk_addbits(&mask,clk_hours[hour+1]);
      break;  
    case 53: case 54: case 55: case 56: case 57: 
      clk_addbits(&mask,clk_min_vijf); clk_addbits(&mask,clk_min_voor); clk_addbits(&mask,clk_hours[hour+1]);
      break;  
    case 58: case 59:  
      clk_addbits(&mask,clk_hours[hour+1]);
      break;  
  }
  return mask;
}


// Display ==========================================================================


// Pin from ESP8266 to the MAX7221 controlled 8x8 LED matrix
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


// Writes framebuffer to the MAX7221 (which controls the 8x8 LED matrix)
void dsp_img(uint64_t data) {
  for(int i=1; i<=8; i++ ) {
    byte row = data&0xFF;
    dsp_out(i,row);
    data >>= 8;
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


#define BUT_PIN D3


int but_crnt;
int but_prev;


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


// Setup ==========================================================================


int time_hour;
int time_min;

uint32_t last;

void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nWord clock - set by hand\n");
  dsp_init();
  but_init();

  last= millis();

  time_hour= 1;
  Serial.printf("Increment HOUR by pressing the button (long press to quit)\n");
  Serial.printf("hour= %02d\n",time_hour);
  dsp_img( clk_timemask(time_hour,0) );
  while( 1 ) {
    but_scan();
    if( but_wentdown() ) {
      time_hour= (time_hour+1) % 12;
      Serial.printf("hour= %02d\n",time_hour);
      dsp_img( clk_timemask(time_hour,0) );
      last= millis();
    }
    if( but_isdown() && millis()-last>1000 ) {
      time_hour= (time_hour+11) % 12;
      Serial.printf("hour= %02d\n",time_hour);
      dsp_img( clk_timemask(time_hour,0) );
      break;
    }
    yield();
  }

  // Wait for button release
  while( but_isdown() ) { but_scan(); yield(); }

  time_min= 5;
  Serial.printf("Increment MINUTES by pressing the button (long press to quit)\n");
  Serial.printf("min = %02d\n",time_min);
  dsp_img( clk_timemask(14,time_min) );
  while( 1 ) {
    but_scan();
    if( but_wentdown() ) {
      time_min = (time_min+5) % 60;
      Serial.printf("min = %02d\n",time_min);
      dsp_img( clk_timemask(14,time_min) );
      last= millis();
    }
    if( but_isdown() && millis()-last>1000 ) {
      time_min= (time_min+55) % 60;
      Serial.printf("min = %02d\n",time_min);
      dsp_img( clk_timemask(14,time_min) );
      break;
    }
    yield();
  }
  
  last= millis();
}


void loop() {
  Serial.printf("%2d:%02d\n", time_hour,time_min); 
  dsp_img( clk_timemask(time_hour,time_min) );
  if( millis()-last > 60000 ) {
    time_min= time_min+1; 
    if( time_min==60 ) { time_min= 0; time_hour= (time_hour+1)%12; }
    last= millis();
  }
  delay(1000);
}
