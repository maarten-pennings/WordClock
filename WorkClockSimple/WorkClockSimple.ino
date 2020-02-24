// WorkClockSimple.ino - A WordClock using NeoPixels, keeping time via NTP - but simple, no runtimeconfig
#define VERSION 1

#include <time.h>
#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>


// LED ==========================================================================
// Driver for the built-in LED - used for simple signalling to the user

#define LED_PIN LED_BUILTIN

void led_off() {
  digitalWrite(LED_PIN, HIGH); // low active
}

void led_on() {
  digitalWrite(LED_PIN, LOW); // low active
}

void led_init() {
  pinMode(LED_PIN, OUTPUT);
  led_off();
  Serial.printf("led : init\n");
}


// WIFI =========================================================================
// Driver for Wifi

#define WIFI_SSID "GuestFamPennings"
#define WIFI_PASS "there_is_no_password"

void wifi_init() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("wifi: init\n");
}

// Prints WiFi status to the user (only when changed)
bool wifi_check() {
  static bool wifi_on= false;
  if( WiFi.status()==WL_CONNECTED ) {
    if( !wifi_on ) Serial.printf("wifi: connected, IP address %s\n", WiFi.localIP().toString().c_str() );
    wifi_on= true;
  } else {
    if( wifi_on ) Serial.printf("wifi: disconnected\n" );
    wifi_on= false;    
  }
  return wifi_on;
}


// CLOCK ========================================================================
// Maintains local time, using NTP servers plus timezone settings

#define CLK_SVR1   "pool.ntp.org"
#define CLK_SVR2   "europe.pool.ntp.org"
#define CLK_SVR3   "north-america.pool.ntp.org"

#define CLK_TZ     "CET-1CEST,M3.5.0,M10.5.0/3" // Amsterdam

#define CLK_ROUND  (60+60+30) // Optional round up in seconds (since our clock has resolution of 5 min)

void clk_init() {
  // Default NTP poll time is SNTP_UPDATE_DELAY (sntp.c, value 3600000 ms or 1 hour).
  // Change with sntp_set_update_delay(ms)
  configTime(CLK_TZ, CLK_SVR1, CLK_SVR2, CLK_SVR3);
  Serial.printf("clk : init\n");
}

// Out parameters have current time, return value indicates if local time is already available.
bool clk_get(int*hour, int*min ) {
  // Get the current time
  time_t tnow= time(NULL); // Returns seconds - note `time_t` is just a `long`.
  
  // In `snow` the `tm_year` field is 1900 based, `tm_month` is 0 based, rest is as expected
  struct tm * snow= localtime(&tnow); // Returns a struct with time fields (https://www.tutorialspoint.com/c_standard_library/c_function_localtime.htm)
  bool avail= snow->tm_year + 1900 >= 2020; // If time is in the past, treat it as not yet set

  // Debug prints of current time (NTP with timezone and DST)
  Serial.printf("clk : %d-%02d-%02d %02d:%02d:%02d (dst=%d) ", snow->tm_year + 1900, snow->tm_mon + 1, snow->tm_mday, snow->tm_hour, snow->tm_min, snow->tm_sec, snow->tm_isdst );
  if( !avail ) Serial.printf("[no NTP sync yet]\n" ); // else ... letters prints the \n

  // Now, round up, as requested by user
  tnow+= CLK_ROUND;
  snow= localtime(&tnow); // Returns a struct with time fields (https://www.tutorialspoint.com/c_standard_library/c_function_localtime.htm)
  if( avail ) Serial.printf("%02d:%02d ", snow->tm_hour, snow->tm_min );

  // Get time components
  if( hour ) *hour= snow->tm_hour;
  if( min  ) *min = snow->tm_min;
  
  return avail;
}


// NEO =========================================================================
// Driver for the NeoPixel board

#define NEO_DIN_PIN        D6
#define NEO_NUMPIXELS      64

// When we setup the NeoPixel library, we tell it how many pixels, which NEO_DIN_PIN to use to send signals, and some pixel type settings.
Adafruit_NeoPixel neo = Adafruit_NeoPixel(NEO_NUMPIXELS, NEO_DIN_PIN, NEO_GRB + NEO_KHZ800);

// Begin with blabk display
void neo_init() {
  neo.begin();
  neo.clear();
  neo.show();
  Serial.printf("neo : init\n");
}


// LETTERS =====================================================================
// Library that maps words to NeoPixels

// The colors used for the words
#define LETTERS_COL_HOUR 0x220000
#define LETTERS_COL_MIN  0x002200
#define LETTERS_COL_HELP 0x000033

// The NeoPixels are numbered from bottom to top, from right to left (this matches the 8x8 matrix order).
// Assumption: IN side of the board is on the lower right hand (looking to the front)
// 63 62 61 60 59 58 57 56  -  KWARTIEN
// 55 54 53 52 51 50 49 48  -  VYF*VOOR 
// 47 46 45 44 43 42 41 40  -  OVERHALF
// 39 38 37 36 35 34 33 32  -  ELFZDRIE # ZEVEN diagonal
// 31 30 29 28 27 26 25 24  -  TIENEGEN
// 23 22 21 20 19 18 17 16  -  VIERZVYF # ZES diagonal
// 15 14 13 12 11 10  9  8  -  ACHTWEEN
//  7  6  5  4  3  2  1  0  -  TWAALFSN

// All words: indexes of the NeoPixels that form a word (-1 terminates)
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

// Array of the hours (note that indices 0 and 13 are filled, for 0:15 respectively 12:45)
const int*letters_hours[] = {letters_hour_twaalf,letters_hour_een,letters_hour_twee,letters_hour_drie,letters_hour_vier,letters_hour_vijf,letters_hour_zes,letters_hour_zeven,letters_hour_acht,letters_hour_negen,letters_hour_tien,letters_hour_elf,letters_hour_twaalf,letters_hour_een};

// Helpers function to print the current time in words over Serial
void letters_print( const int word[] ) {
  if( word==letters_min_vijf   ) Serial.print("vijf-");
  if( word==letters_min_kwart  ) Serial.print("kwart-");
  if( word==letters_min_tien   ) Serial.print("tien-");
  if( word==letters_min_voor   ) Serial.print("voor-");
  if( word==letters_min_over   ) Serial.print("over-");
  if( word==letters_min_half   ) Serial.print("half-");
  if( word==letters_hour_elf   ) Serial.print("elf");
  if( word==letters_hour_drie  ) Serial.print("drie");
  if( word==letters_hour_zeven ) Serial.print("zeven");
  if( word==letters_hour_tien  ) Serial.print("tien");
  if( word==letters_hour_negen ) Serial.print("negen");
  if( word==letters_hour_vier  ) Serial.print("vier");
  if( word==letters_hour_vijf  ) Serial.print("vijf");
  if( word==letters_hour_zes   ) Serial.print("zes");
  if( word==letters_hour_acht  ) Serial.print("acht");
  if( word==letters_hour_twee  ) Serial.print("twee");
  if( word==letters_hour_een   ) Serial.print("een");
  if( word==letters_hour_twaalf) Serial.print("twaalf");
}

// Sets all NeoPixels in `word` to color `color`.
void letters_add_word(uint32_t color, const int word[] ) {
  letters_print(word);
  const int * letter= word; // Point to the first NeoPixel in `word`
  while( *letter>=0 ) {
    neo.setPixelColor(*letter,color);
    letter++; // Point to the next NeoPixel in `word`
  }
}

// Sets the NeoPixels for the time specified by `hour` and `min` to "on" (in the appropriate color, see LETTERS_COL_XXX).
// E.g. 7:35 -> vijf over half acht
void letters_add_time(int hour, int min ) {
  switch( min ) {
    case 0: case 1: case 2: case 3: case 4: 
      letters_add_word(0x220000,letters_hours[hour]);
      break;  
    case 5: case 6: case 7: case 8: case 9: 
      letters_add_word(LETTERS_COL_MIN,letters_min_vijf); letters_add_word(LETTERS_COL_HELP,letters_min_over); letters_add_word(LETTERS_COL_HOUR,letters_hours[hour]);
      break;  
    case 10: case 11: case 12: case 13: case 14: 
      letters_add_word(LETTERS_COL_MIN,letters_min_tien); letters_add_word(LETTERS_COL_HELP,letters_min_over); letters_add_word(LETTERS_COL_HOUR,letters_hours[hour]);
      break;  
    case 15: case 16: case 17: case 18: case 19: 
      letters_add_word(LETTERS_COL_MIN,letters_min_kwart); letters_add_word(LETTERS_COL_HELP,letters_min_over); letters_add_word(LETTERS_COL_HOUR,letters_hours[hour]);
      break;  
    case 20: case 21: case 22: case 23: case 24: 
      letters_add_word(LETTERS_COL_MIN,letters_min_tien); letters_add_word(LETTERS_COL_HELP,letters_min_voor); letters_add_word(LETTERS_COL_MIN,letters_min_half); letters_add_word(LETTERS_COL_HOUR,letters_hours[hour+1]);
      break;  
    case 25: case 26: case 27: case 28: case 29: 
      letters_add_word(LETTERS_COL_MIN,letters_min_vijf); letters_add_word(LETTERS_COL_HELP,letters_min_voor); letters_add_word(LETTERS_COL_MIN,letters_min_half); letters_add_word(LETTERS_COL_HOUR,letters_hours[hour+1]);
      break;  
    case 30: case 31: case 32: case 33: case 34: 
      letters_add_word(LETTERS_COL_MIN,letters_min_half); letters_add_word(LETTERS_COL_HOUR,letters_hours[hour+1]);
      break;  
    case 35: case 36: case 37: case 38: case 39: 
      letters_add_word(LETTERS_COL_MIN,letters_min_vijf); letters_add_word(LETTERS_COL_HELP,letters_min_over); letters_add_word(LETTERS_COL_MIN,letters_min_half); letters_add_word(LETTERS_COL_HOUR,letters_hours[hour+1]);
      break;  
    case 40: case 41: case 42: case 43: case 44: 
      letters_add_word(LETTERS_COL_MIN,letters_min_tien); letters_add_word(LETTERS_COL_HELP,letters_min_over); letters_add_word(LETTERS_COL_MIN,letters_min_half); letters_add_word(LETTERS_COL_HOUR,letters_hours[hour+1]);
      break;  
    case 45: case 46: case 47: case 48: case 49: 
      letters_add_word(LETTERS_COL_MIN,letters_min_kwart); letters_add_word(LETTERS_COL_HELP,letters_min_voor); letters_add_word(LETTERS_COL_HOUR,letters_hours[hour+1]);
      break;  
    case 50: case 51: case 52: case 53: case 54: 
      letters_add_word(LETTERS_COL_MIN,letters_min_tien); letters_add_word(LETTERS_COL_HELP,letters_min_voor); letters_add_word(LETTERS_COL_HOUR,letters_hours[hour+1]);
      break;  
    case 55: case 56: case 57: case 58: case 59:
      letters_add_word(LETTERS_COL_MIN,letters_min_vijf); letters_add_word(LETTERS_COL_HELP,letters_min_voor); letters_add_word(LETTERS_COL_HOUR,letters_hours[hour+1]);
      break;  
  }
}

// Updates the NeoPixel board with the new `hour` and `min` (if they differ from previous value)
void letters_update(int hour, int min) {
  static int hour_prev;
  static int min_prev;

  if( hour_prev!=hour || min_prev!=min ) {
    neo.clear();
    letters_add_time(hour%12,min); // Remember to print PM hours to 0..11
    neo.show();
  }
  
  hour_prev= hour;
  min_prev= min;

  Serial.printf("\n");
}


// MAIN ========================================================================

void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nmain: WorkClockSimple v%d\n\n",VERSION);

  neo_init();
  led_init();
  clk_init();
  wifi_init();

  Serial.printf("\n");
  led_on(); // Show the app is running, and local time not yet synced with NTP
}


void loop() {
  wifi_check();

  int hour, min;
  if( clk_get(&hour,&min) ) {
    letters_update(hour,min); 
    led_off(); // signals "NTP synced" to user
  }
  
  delay(1000); 
}
