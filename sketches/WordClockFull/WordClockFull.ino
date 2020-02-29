// WordClockFull.ino - A wordClock using NeoPixels, keeping time via NTP, with dynamic configuration
#define VERSION 1

#include <time.h>
#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>


// LED ==========================================================================

#define LED_PIN LED_BUILTIN

void led_off() {
  digitalWrite(LED_PIN, HIGH); // low active
}

void led_on() {
  digitalWrite(LED_PIN, LOW); // low active
}

void led_tgl() {
  digitalWrite(LED_PIN, !digitalRead(LED_PIN) );
}

void led_init() {
  pinMode(LED_PIN, OUTPUT);
  led_off();
  Serial.printf("led : init\n");
}


// WIFI =========================================================================

#define WIFI_SSID "GuestFamPennings"
#define WIFI_PASS "there_is_no_password"

void wifi_init() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("wifi: init\n");
}

bool wifi_check() {
  static bool wifi_on= false;
  if( WiFi.status()==WL_CONNECTED ) {
    if( !wifi_on ) Serial.printf("Wifi connected, IP address %s\n", WiFi.localIP().toString().c_str() );
    wifi_on= true;
  } else {
    if( wifi_on ) Serial.printf("Wifi disconnected\n" );
    wifi_on= false;    
  }
  return wifi_on;
}

// CLOCK ========================================================================

#define CLK_SVR1 "pool.ntp.org"
#define CLK_SVR2 "europe.pool.ntp.org"
#define CLK_SVR3 "north-america.pool.ntp.org"

#define CLK_TZ   "CET-1CEST,M3.5.0,M10.5.0/3" // Amsterdam

void clk_init() {
  // Default NTP poll time is SNTP_UPDATE_DELAY (sntp.c, value 3600000 ms or 1 hour).
  // Change with sntp_set_update_delay(ms)
  configTime(CLK_TZ, CLK_SVR1, CLK_SVR2, CLK_SVR3);
  Serial.printf("clk : init\n");
}

bool clk_get(int*hour, int*min, int*sec, int*dst ) {
  // Get the current timr
  time_t      tnow= time(NULL);       // Returns seconds - note `time_t` is just a `long`.
  struct tm * snow= localtime(&tnow); // Returns a struct with time fields (https://www.tutorialspoint.com/c_standard_library/c_function_localtime.htm)

  // Get time components
  if( hour ) *hour= snow->tm_hour;
  if( min  ) *min = snow->tm_min;
  if( sec  ) *sec = snow->tm_sec;
  if( dst  ) *dst = snow->tm_isdst;
  
  // In `snow` the `tm_year` field is 1900 based, `tm_month` is 0 based, rest is as expected
  return snow->tm_year + 1900 >= 2020;
}

// NEO =========================================================================

#define NEO_DIN_PIN        D6
#define NEO_NUMPIXELS      64

// When we setup the NeoPixel library, we tell it how many pixels, which NEO_DIN_PIN to use to send signals, and some pixel type settings.
Adafruit_NeoPixel neo = Adafruit_NeoPixel(NEO_NUMPIXELS, NEO_DIN_PIN, NEO_GRB + NEO_KHZ800);

int      neo_test_ledix;
uint32_t neo_test_ledcol;
bool     neo_test_complete;

void neo_test() {
  // If we are at "all LEDs off" start with red again
  if( neo_test_ledcol==0 ) neo_test_ledcol=0x110000; 
  // If we are at the first led (again) clear the display
  if( neo_test_ledix==0 ) neo.clear(); 
  // Switch on led `neo_test_ledix`
  neo.setPixelColor(neo_test_ledix,neo_test_ledcol);
  // Send to NeoPixels string
  neo.show();

  // Go to next pixel
  neo_test_ledix++;
  if( neo_test_ledix>NEO_NUMPIXELS ) { 
    neo_test_ledix= 0; 
    neo_test_ledcol>>=8; 
    neo_test_complete= neo_test_ledcol==0; 
    if( neo_test_complete ) Serial.printf("neo : tested\n");
  }
}

void neo_init() {
  neo.begin();
  neo.clear();
  neo.show();
  Serial.printf("neo : init\n");
}


// LETTERS =====================================================================

#define LETTERS_COL_HOUR 0x110000
#define LETTERS_COL_MIN  0x001100
#define LETTERS_COL_HELP 0x000011

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

// All words: indexes of the LEDs that form a word (-1 terminates)
const int letters_min_vijf[]    = {55,54,53,-1};
const int letters_min_kwart[]   = {63,62,61,60,59,-1};
const int letters_min_tien[]    = {59,58,57,56,-1};
const int letters_min_voor[]    = {51,50,49,48,-1};
const int letters_min_over[]    = {47,46,45,44,-1};
const int letters_min_half[]    = {43,42,41,40,-1};
const int letters_hour_elf[]    = {39,38,37,-1};
const int letters_hour_drie[]   = {35,34,33,32,-1};
const int letters_hour_zeven[]  = {36,27,18,9,0,-1};
const int letters_hour_tien[]   = {31,30,29,28,-1};
const int letters_hour_negen[]  = {28,27,26,25,24,-1};
const int letters_hour_vier[]   = {23,22,21,20,-1}; 
const int letters_hour_vijf[]   = {18,17,16,-1}; 
const int letters_hour_zes[]    = {19,10,1,-1};
const int letters_hour_acht[]   = {15,14,13,12,-1};
const int letters_hour_twee[]   = {12,11,10,9,-1};
const int letters_hour_een[]    = {10,9,8,-1};
const int letters_hour_twaalf[] = {7,6,5,4,3,2,-1};
const int letters_hour_empty[]  = {-1}; 

const int*letters_hours[] = {letters_hour_twaalf,letters_hour_een,letters_hour_twee,letters_hour_drie,letters_hour_vier,letters_hour_vijf,letters_hour_zes,letters_hour_zeven,letters_hour_acht,letters_hour_negen,letters_hour_tien,letters_hour_elf,letters_hour_twaalf,letters_hour_een};

void letters_add_word(uint32_t color, const int word[] ) {
  const int * letter= word; // Point to the first NeoPixel in `word`
  while( *letter>=0 ) {
    neo.setPixelColor(*letter,color);
    letter++; // Point to the next NeoPixel in `word`
  }
}

// Sets the NeoPixels for the time specified by `hour` and `min`.
// E.g. 7:35 -> vijf over half acht
void letters_add_time(int hour, int min ) {
  switch( min ) {
    case 0: case 1: case 2: 
      letters_add_word(0x220000,letters_hours[hour]);
      break;  
    case 3: case 4: case 5: case 6: case 7: 
      letters_add_word(LETTERS_COL_MIN,letters_min_vijf); letters_add_word(LETTERS_COL_HELP,letters_min_over); letters_add_word(LETTERS_COL_HOUR,letters_hours[hour]);
      break;  
    case 8: case 9: case 10: case 11: case 12: 
      letters_add_word(LETTERS_COL_MIN,letters_min_tien); letters_add_word(LETTERS_COL_HELP,letters_min_over); letters_add_word(LETTERS_COL_HOUR,letters_hours[hour]);
      break;  
    case 13: case 14: case 15: case 16: case 17: 
      letters_add_word(LETTERS_COL_MIN,letters_min_kwart); letters_add_word(LETTERS_COL_HELP,letters_min_over); letters_add_word(LETTERS_COL_HOUR,letters_hours[hour]);
      break;  
    case 18: case 19: case 20: case 21: case 22: 
      letters_add_word(LETTERS_COL_MIN,letters_min_tien); letters_add_word(LETTERS_COL_HELP,letters_min_voor); letters_add_word(LETTERS_COL_MIN,letters_min_half); letters_add_word(LETTERS_COL_HOUR,letters_hours[hour+1]);
      break;  
    case 23: case 24: case 25: case 26: case 27: 
      letters_add_word(LETTERS_COL_MIN,letters_min_vijf); letters_add_word(LETTERS_COL_HELP,letters_min_voor); letters_add_word(LETTERS_COL_MIN,letters_min_half); letters_add_word(LETTERS_COL_HOUR,letters_hours[hour+1]);
      break;  
    case 28: case 29: case 30: case 31: case 32: 
      letters_add_word(LETTERS_COL_MIN,letters_min_half); letters_add_word(LETTERS_COL_HOUR,letters_hours[hour+1]);
      break;  
    case 33: case 34: case 35: case 36: case 37: 
      letters_add_word(LETTERS_COL_MIN,letters_min_vijf); letters_add_word(LETTERS_COL_HELP,letters_min_over); letters_add_word(LETTERS_COL_MIN,letters_min_half); letters_add_word(LETTERS_COL_HOUR,letters_hours[hour+1]);
      break;  
    case 38: case 39: case 40: case 41: case 42: 
      letters_add_word(LETTERS_COL_MIN,letters_min_tien); letters_add_word(LETTERS_COL_HELP,letters_min_over); letters_add_word(LETTERS_COL_MIN,letters_min_half); letters_add_word(LETTERS_COL_HOUR,letters_hours[hour+1]);
      break;  
    case 43: case 44: case 45: case 46: case 47: 
      letters_add_word(LETTERS_COL_MIN,letters_min_kwart); letters_add_word(LETTERS_COL_HELP,letters_min_voor); letters_add_word(LETTERS_COL_HOUR,letters_hours[hour+1]);
      break;  
    case 48: case 49: case 50: case 51: case 52: 
      letters_add_word(LETTERS_COL_MIN,letters_min_tien); letters_add_word(LETTERS_COL_HELP,letters_min_voor); letters_add_word(LETTERS_COL_HOUR,letters_hours[hour+1]);
      break;  
    case 53: case 54: case 55: case 56: case 57: 
      letters_add_word(LETTERS_COL_MIN,letters_min_vijf); letters_add_word(LETTERS_COL_HELP,letters_min_voor); letters_add_word(LETTERS_COL_HOUR,letters_hours[hour+1]);
      break;  
    case 58: case 59:  
      letters_add_word(LETTERS_COL_HOUR,letters_hours[hour+1]);
      break;  
  }
}

void letters_update(int hour, int min) {
  static int hour_prev;
  static int min_prev;

  if( hour_prev!=hour || min_prev!=min ) {
    neo.clear();
    letters_add_time(hour%12,min);
    neo.show();
  }
  
  hour_prev= hour;
  min_prev= min;
}

// MAIN ========================================================================

void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nmain: WordClockFull v%d\n\n",VERSION);

  led_init();
  clk_init();
  wifi_init();
  neo_init();

  Serial.printf("\n");
  led_on(); // Show the app is running (and time not yet synced)
  Serial.printf("neo : testing\n");
}


void loop() {
  int hour, min, sec, dst, avail;
    
  bool wifi= wifi_check();
  if( !neo_test_complete || !wifi) { neo_test(); delay(25); return; } 

  avail= clk_get(&hour,&min,&sec,&dst);
  if( avail ) {
    Serial.printf("main: %02d:%02d:%02d (dst=%d)\n", hour, min, sec, dst );
    letters_update(hour,min);
    led_off();
  } else {
    Serial.printf("main: clock not yet synced\n"); // We miss-uses old time as indication of " time not yet set"
  }
  
  delay(1000); 
}
