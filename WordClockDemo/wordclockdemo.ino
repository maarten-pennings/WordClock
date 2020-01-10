// wordclockdemo.ino - A clock that tells time, in Dutch, in steps of 5 minutes on a MAX7221 controlled 8x8 LED matrix
// Idea http://www.espruino.com/Tiny+Word+Clock
// Demo https://www.youtube.com/watch?v=YDhCZarNm9g
// MAX722 datasheet https://datasheets.maximintegrated.com/en/ds/MAX7219-MAX7221.pdf
// datasheet explained https://www.youtube.com/watch?v=tFDiMw5QKZQ


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
const int _vyf[]   ={7,6,5,-1};
const int _kwart[] ={4,3,2,1,0,-1};
const int _tien[]  ={15,14,13,12,-1};
const int _voor[]  ={11,10,9,8,-1};
const int _over[]  ={23,22,21,20,-1};
const int _half[]  ={19,18,17,16,-1};
const int acht[]   ={31,30,29,28,-1};
const int twee[]   ={28,27,26,25,-1};
const int een[]    ={26,25,24,-1};
const int tien[]   ={39,38,37,36,-1};
const int negen[]  ={36,35,34,33,32,-1};
const int zes[]    ={47,43,40,-1};
const int drie[]   ={46,45,44,43,-1};
const int elf[]    ={43,42,41,-1};
const int vier[]   ={55,54,53,-1}; // 49=IE
const int zeven[]  ={52,51,50,49,48,-1};
const int vyf[]    ={63,62,61,-1}; 
const int twaalf[] ={60,59,58,57,56,-1};  // 63=LF


// The hours in an array, note that 0:xx and 12:30 are supported 
const int * hours[] = {twaalf,een,twee,drie,vier,vyf,zes,zeven,acht,negen,tien,elf,twaalf,een};


// A frame is encoded in a 64 bit mask.
// This function sets bits (switches on leds) in `mask`, 
// namely those listed in array `bits` (which is terminated with -1)
void addbits(uint64_t*mask, const int bits[]) {
  const int * bit= bits;
  while( *bit>=0 ) {
    *mask |= 1LLU << *bit;
    bit++;
  }
}


// Returns a frame showing the time specified by `hour` and `min`.
// E.g. 7:35 -> vijf over half acht
uint64_t timemask(int hour, int min ) {
  uint64_t mask= 0;
  switch( min ) {
    case 0: case 1: case 2: 
      addbits(&mask,hours[hour]);
      break;  
    case 3: case 4: case 5: case 6: case 7: 
      addbits(&mask,_vyf); addbits(&mask,_over); addbits(&mask,hours[hour]);
      break;  
    case 8: case 9: case 10: case 11: case 12: 
      addbits(&mask,_tien); addbits(&mask,_over); addbits(&mask,hours[hour]);
      break;  
    case 13: case 14: case 15: case 16: case 17: 
      addbits(&mask,_kwart); addbits(&mask,_over); addbits(&mask,hours[hour]);
      break;  
    case 18: case 19: case 20: case 21: case 22: 
      addbits(&mask,_tien); addbits(&mask,_voor); addbits(&mask,_half); addbits(&mask,hours[hour+1]);
      break;  
    case 23: case 24: case 25: case 26: case 27: 
      addbits(&mask,_vyf); addbits(&mask,_voor); addbits(&mask,_half); addbits(&mask,hours[hour+1]);
      break;  
    case 28: case 29: case 30: case 31: case 32: 
      addbits(&mask,_half); addbits(&mask,hours[hour+1]);
      break;  
    case 33: case 34: case 35: case 36: case 37: 
      addbits(&mask,_vyf); addbits(&mask,_over); addbits(&mask,_half); addbits(&mask,hours[hour+1]);
      break;  
    case 38: case 39: case 40: case 41: case 42: 
      addbits(&mask,_tien); addbits(&mask,_over); addbits(&mask,_half); addbits(&mask,hours[hour+1]);
      break;  
    case 43: case 44: case 45: case 46: case 47: 
      addbits(&mask,_kwart); addbits(&mask,_voor); addbits(&mask,hours[hour+1]);
      break;  
    case 48: case 49: case 50: case 51: case 52: 
      addbits(&mask,_tien); addbits(&mask,_voor); addbits(&mask,hours[hour+1]);
      break;  
    case 53: case 54: case 55: case 56: case 57: 
      addbits(&mask,_vyf); addbits(&mask,_voor); addbits(&mask,hours[hour+1]);
      break;  
    case 58: case 59:  
      addbits(&mask,hours[hour+1]);
      break;  
  }
  return mask;
}


// Pin from ESP8266 to the MAX7221 controlled 8x8 LED matrix
//      DSP_VCC  3V3   // VCC pin of display is connected to 3V3 of ESP8266
//      DSP_GND  GND   // GND pin of display is connected to GND of ESP8266
#define DSP_DIN   D7   // DIN pin of display is connected to D7 of ESP8266
#define DSP_CS    D2   // CS  pin of display is connected to D2 of ESP8266 
#define DSP_CLK   D5   // CLK pin of display is connected to D5 of ESP8266


// Configures ESP8266 pins that are connected to the MAX7221
void dsp_init(void) {
  digitalWrite(DSP_CS, HIGH);
  pinMode(DSP_DIN, OUTPUT);
  pinMode(DSP_CS, OUTPUT);
  pinMode(DSP_CLK, OUTPUT);
}


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

void setup() {
  Serial.begin(115200);
  Serial.println("\n\nWord clock on 8x8 LED matrix with MAX7221");
  dsp_init();

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

int time_hour = 0;
int time_min = 0;

void loop() {
  Serial.printf("%2d:%02d - %lu\n", time_hour,time_min,millis()/1000); 
  dsp_img( timemask(time_hour,time_min) );
  time_min= time_min+1; if( time_min==60 ) { time_min= 0; time_hour= (time_hour+1)%12; }
  delay(100); // Demo mode: go fast
}
