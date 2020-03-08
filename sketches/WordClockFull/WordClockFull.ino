// WordClockFull.ino - A wordClock using NeoPixels, keeping time via NTP, with dynamic configuration
#define VERSION "3"

#include <time.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <Adafruit_NeoPixel.h>
#include <Cfg.h> // A library that lets a user configure an ESP8266 app (https://github.com/maarten-pennings/Cfg)


// CFG (configuration) ==========================================================================
// Persistent storage of configuration parameters

NvmField cfg_fields[] = {
  {"Access points"   , ""                           ,  0, "The clock uses internet to get time. Supply credentials for one or more WiFi access points (APs). " },
  {"Ssid.1"          , "SSID for AP1"               , 32, "The ssid of the first wifi network the WordClock could connect to (mandatory)." },
  {"Password.1"      , "Password for AP1"           , 32, "The password of the first wifi network the WordClock could connect to (mandatory). "},
  {"Ssid.2"          , "SSID for AP2"               , 32, "The ssid of the second wifi network (optional, may be blank)." },
  {"Password.2"      , "Password for AP2"           , 32, "The password of the second wifi network (optional, may be blank). "},
  {"Ssid.3"          , "SSID for AP3"               , 32, "The ssid of the third wifi network (optional, may be blank)." },
  {"Password.3"      , "Password for AP3"           , 32, "The password of the third wifi network (optional, may be blank). "},
  
  {"Time management" , ""                           ,  0, "Time is obtained from so-called NTP servers. They provide UTC time, so also the time-zone must be entered. " },
  {"NTP.server.1"    , "pool.ntp.org"               , 32, "The hostname of the first NTP server." },
  {"NTP.server.2"    , "europe.pool.ntp.org"        , 32, "The hostname of a second NTP server." },
  {"NTP.server.3"    , "north-america.pool.ntp.org" , 32, "The hostname of a third NTP server. " },
  {"Timezone"        , "CET-1CEST,M3.5.0,M10.5.0/3" , 48, "The timezone string (including daylight saving), see <A href='https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html'>details</A>. " },
  {"Round"           , "150"                        ,  4, "The number of seconds to add to the actual time (typically 150, to round 2.5 min up to 5 min intervals). " },
  
  {"Color palette"   , ""                           ,  0, "The colors for the letter categories. Use RRGGBB, hex digits for red, green and blue. E.g. FFFF00 is bright yellow, 000011 dim blue. " },
  {"Color.1"         , "110000"                     ,  6, "The first color; by default used for hour category (een-twaalf)." },
  {"Color.2"         , "001100"                     ,  6, "The second color; by default used for minute 1 category (vijf, tien, kwart)." },
  {"Color.3"         , "0C0C00"                     ,  6, "The third color; by default used for minute 2 category (half). If colors 2 and 3 are equal, 'half' belongs to 'minutes'." },
  {"Color.4"         , "000011"                     ,  6, "The fourth color; by default used for prepositions category (voor, over)." },
  {"Color.5"         , "222222"                     ,  6, "The fifth color; for some animations. " },
  
  {"Display"         , ""                           ,  0, "Which colors are used when. " },
  {"Refresh"         , "one"                        ,  8, "When is the display refreshed: <b>one</b> (every minute - useless for <b>fix</b>/<b>none</b>), <b>five</b> (every 5 minutes)." },
  {"Mapping"         , "cycle"                      ,  8, "Mapping: <b>fix</b> (fixed to default), <b>cycle</b> (colors cycle over categories), <b>random</b> (random between color 1 and 2; 3 and 4 not used)." },  
  {"Animation"       , "wipe"                       ,  8, "The animation: <b>none</b>, <b>wipe</b> (horizontal wipe, using Color.4), <b>dots</b> (letter by letter off then on). " },  
  {0                 , 0                            ,  0, 0},  
};

#define CFG_BUT_PIN 0

Cfg cfg("WordClock", cfg_fields, CFG_SERIALLVL_USR, LED_BUILTIN);


// COL (Color) ================================================================================
// Color management (palettes)

typedef struct {
  uint32_t h;  // Color for hours
  uint32_t m1; // Color for minutes (vijf, tien, kwart)
  uint32_t m2; // Color for minutes (half)
  uint32_t p;  // Color for prepositions (voor, over)
} palette4_t;

typedef struct {
  uint32_t c1;
  uint32_t c2;
  uint32_t c3;
  uint32_t c4;
  uint32_t c5;
} palette5_t;


// Values retrieved from Cfg

palette5_t col_palette; 

#define COL_REFRESH_ONE    1 // cfg string: one
#define COL_REFRESH_FIVE   5 // cfg string: five
int col_refresh;   

#define COL_MAPPING_FIX    1 // cfg string: fix
#define COL_MAPPING_CYCLE  2 // cfg string: cycle
#define COL_MAPPING_RANDOM 3 // cfg string: random
int col_mapping;   

#define COL_ANIMATION_NONE 1 // cfg string: none
#define COL_ANIMATION_WIPE 2 // cfg string: wipe
#define COL_ANIMATION_DOTS 3 // cfg string: dots
int col_animation; 

// The parse and unparse routines

uint32_t col_parse(const char * cfgid) {
  char * val= cfg.getval(cfgid);
  if( strlen(val)==6 ) {
    int ishex= true;
    uint32_t col= 0;
    for(int i=0; i<6; i++) {
      col<<=4;
      if(      '0'<=val[i] && val[i]<='9' ) col+= val[i]-'0';
      else if( 'a'<=val[i] && val[i]<='f' ) col+= val[i]-'a'+10;
      else if( 'A'<=val[i] && val[i]<='F' ) col+= val[i]-'A'+10;
      else ishex= false;
    }
    if( ishex ) return col;
  }
  Serial.printf("ltrs: ERROR in %s (%s)\n",cfgid,val);
  return 0x101010; // Not 6 hex digits, return grey
}

const char * col_refresh_unparse(int val) {
  if(      val==COL_REFRESH_ONE  ) return "one";
  else if( val==COL_REFRESH_FIVE ) return "five";
  else return "?refresh?";
}

int col_refresh_parse() {
  const char * cfgid= "Refresh";
  char * val= cfg.getval(cfgid);
  if( strcmp(val,"one" )==0 ) return COL_REFRESH_ONE;
  if( strcmp(val,"five")==0 ) return COL_REFRESH_FIVE;
  int dft= COL_REFRESH_ONE;
  Serial.printf("ltrs: ERROR in %s (%s->%s)\n",cfgid,val,col_refresh_unparse(dft));
  return dft;
}

const char * col_mapping_unparse(int val) {
  if(      val==COL_MAPPING_FIX    ) return "fix";
  else if( val==COL_MAPPING_CYCLE  ) return "cycle";
  else if( val==COL_MAPPING_RANDOM ) return "random";
  else return "?mapping?";
}

int col_mapping_parse() {
  const char * cfgid= "Mapping";
  char * val= cfg.getval(cfgid);
  if( strcmp(val,"fix"    )==0 ) return COL_MAPPING_FIX;
  if( strcmp(val,"cycle"  )==0 ) return COL_MAPPING_CYCLE;
  if( strcmp(val,"random" )==0 ) return COL_MAPPING_RANDOM;
  int dft= COL_MAPPING_CYCLE;
  Serial.printf("ltrs: ERROR in %s (%s->%s)\n",cfgid,val,col_mapping_unparse(dft));
  return dft;
}

const char * col_animation_unparse(int val) {
  if(      val==COL_ANIMATION_NONE ) return "none";
  else if( val==COL_ANIMATION_WIPE ) return "wipe";
  else if( val==COL_ANIMATION_DOTS ) return "dots";
  else return "?animation?";
}

int col_animation_parse() {
  const char * cfgid= "Animation";
  char * val= cfg.getval(cfgid);
  if( strcmp(val,"none"    )==0 ) return COL_ANIMATION_NONE;
  if( strcmp(val,"wipe"    )==0 ) return COL_ANIMATION_WIPE;
  if( strcmp(val,"dots"    )==0 ) return COL_ANIMATION_DOTS;
  int dft= COL_ANIMATION_WIPE;
  Serial.printf("ltrs: ERROR in %s (%s->%s)\n",cfgid,val,col_animation_unparse(dft));
  return dft;
}

void col_init() {
  col_palette.c1= col_parse( "Color.1" );
  col_palette.c2= col_parse( "Color.2" );
  col_palette.c3= col_parse( "Color.3" );
  col_palette.c4= col_parse( "Color.4" );
  col_palette.c5= col_parse( "Color.5" );
  Serial.printf("col : palette: %06X %06X %06X %06X %06X\n", col_palette.c1, col_palette.c2, col_palette.c3, col_palette.c4, col_palette.c5 );

  col_refresh= col_refresh_parse();
  col_mapping= col_mapping_parse();
  col_animation= col_animation_parse();
  Serial.printf("col : modes: %s %s %s\n", col_refresh_unparse(col_refresh), col_mapping_unparse(col_mapping), col_animation_unparse(col_animation) );
}

// Generating the right color

void col_random_check() {
  randomSeed( micros() );
  uint32_t r_hi= (col_palette.c1>>16) & 0xFF;
  uint32_t r_lo= (col_palette.c2>>16) & 0xFF;
  uint32_t g_hi= (col_palette.c1>> 8) & 0xFF;
  uint32_t g_lo= (col_palette.c2>> 8) & 0xFF;
  uint32_t b_hi= (col_palette.c1>> 0) & 0xFF;
  uint32_t b_lo= (col_palette.c2>> 0) & 0xFF;
  if( r_lo>=r_hi ) Serial.printf("ltrs: ERROR red lo>=hi (%02x,%02x)\n", r_lo,r_hi );
  if( g_lo>=g_hi ) Serial.printf("ltrs: ERROR green lo>=hi (%02x,%02x)\n", g_lo,g_hi );
  if( b_lo>=b_hi ) Serial.printf("ltrs: ERROR blue lo>=hi (%02x,%02x)\n", b_lo,b_hi );
}

uint32_t col_random() {
  // Use color 1 and 2 to get the max and min values from
  uint32_t r_hi= (col_palette.c1>>16) & 0xFF;
  uint32_t r_lo= (col_palette.c2>>16) & 0xFF;
  uint32_t g_hi= (col_palette.c1>> 8) & 0xFF;
  uint32_t g_lo= (col_palette.c2>> 8) & 0xFF;
  uint32_t b_hi= (col_palette.c1>> 0) & 0xFF;
  uint32_t b_lo= (col_palette.c2>> 0) & 0xFF;
  // If we do truly random, colors get a bit washed out and equal. Introduce bias to lo and hi.
  uint32_t r;
  switch( random(3) ) {
    case 0  : r= r_lo; break;
    default : r= random(r_lo, r_hi); break;
    case 2  : r= r_hi; break;
  }
  uint32_t g;
  switch( random(3) ) {
    case 0  : g= g_lo; break;
    default : g= random(g_lo, g_hi); break;
    case 2  : g= g_hi; break;
  }
  uint32_t b;
  switch( random(3) ) {
    case 0  : b= b_lo; break;
    default : b= random(b_lo, b_hi); break;
    case 2  : b= b_hi; break;
  }
  // Compose
  uint32_t col= (r<<16) + (g<<8) + (b<<0);
  return col;
}

palette4_t * col_next() {
  static palette4_t palette;
  static int first=1;

  switch( col_mapping ) {
    
    case COL_MAPPING_FIX    :
      if( first ) {
        palette.h = col_palette.c1;
        palette.m1= col_palette.c2;
        palette.m2= col_palette.c3;
        palette.p = col_palette.c4;
      } else {
        // No changes needed
      }
      break;
      
    case COL_MAPPING_CYCLE  :
      if( first ) {
        // Initialize from color palette
        palette.h = col_palette.c1;
        palette.m1= col_palette.c2;
        palette.m2= col_palette.c3;
        palette.p = col_palette.c4;
      } else {
        // Cycle
        if( col_palette.c2==col_palette.c3 ) { 
          // special case: c2==c3 -> 'half' belongs to minute category
          uint32_t tmp= palette.h;
          palette.h= palette.m2; // == m1
          palette.m1= palette.m2= palette.p;
          palette.p= tmp;        
        } else {
          // 'half' has own category
          uint32_t tmp= palette.h;
          palette.h= palette.m2;
          palette.m2= palette.p;
          palette.p= palette.m1;
          palette.m1= tmp;
        }    
      }
      break;
      
    case COL_MAPPING_RANDOM :
      if( first ) {
        // Check min,max makes sense
        col_random_check();
      }
      palette.h = col_random();
      palette.m1= col_random();
      palette.m2= col_random();
      palette.p = col_random();
      // special case: c2==c3 -> 'half' belongs to minute category
      if( col_palette.c2==col_palette.c3 ) palette.m2= palette.m1; 
      break;
  }

  // No longer first time
  first= 0;
  
  // Return (pointer to) locally statically stored palette
  return &palette;
}


// BUT (button) ==========================================================================
// Driver for the built-in button - used to force a screen refresh

#define BUT_PIN 0

int but_state_prev;
int but_state_cur;

void but_scan() {
  but_state_prev= but_state_cur;
  but_state_cur= digitalRead(BUT_PIN)==0;
}

int but_wentdown() {
  return (but_state_prev!=but_state_cur) && but_state_cur;
}

void but_init() {
  pinMode(BUT_PIN, INPUT);
  but_scan();
  but_scan();
  Serial.printf("but : init\n");
}


// LED (signal LED) ==========================================================================
// Driver for the built-in LED - used for simple signalling to the user

#define LED_PIN LED_BUILTIN

void led_off() {
  digitalWrite(LED_PIN, HIGH); // low active
}

void led_on() {
  digitalWrite(LED_PIN, LOW); // low active
}

void led_set(int on) {
  digitalWrite(LED_PIN, !on); // low active
}

void led_tgl() {
  digitalWrite(LED_PIN, !digitalRead(LED_PIN) );
}

void led_init() {
  pinMode(LED_PIN, OUTPUT);
  led_off();
  Serial.printf("led : init\n");
}


// WIFI (WiFi access points) =========================================================================
// Driver for WiFi and connection status

ESP8266WiFiMulti wifiMulti;

void wifi_init() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  Serial.printf("wifi: init:");

  // Get AP's from config. 1st is mandatory, others are optional
  if( cfg.getval("Ssid.1")[0]!='0' ) {
    wifiMulti.addAP( cfg.getval("Ssid.1"), cfg.getval("Password.1") );
    Serial.printf(" %s",cfg.getval("Ssid.1"));
  }
  if( cfg.getval("Ssid.2")[0]!='0' ) {
    wifiMulti.addAP( cfg.getval("Ssid.2"), cfg.getval("Password.2") );
    Serial.printf(" %s",cfg.getval("Ssid.2"));
  }
  if( cfg.getval("Ssid.3")[0]!='0' ) {
    wifiMulti.addAP( cfg.getval("Ssid.3"), cfg.getval("Password.3") );
    Serial.printf(" %s",cfg.getval("Ssid.3"));
  }

  Serial.printf("\n");
}

// Prints WiFi status to the user (only when changed)
bool wifi_check() {
  static bool wifi_on= false;
  if( wifiMulti.run()==WL_CONNECTED ) {
    if( !wifi_on ) Serial.printf("wifi: connected to %s, IP address %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str() );
    wifi_on= true;
  } else {
    if( wifi_on ) Serial.printf("wifi: disconnected\n" );
    wifi_on= false;    
  }
  led_set(!wifi_on); // signals wifi status to user (off=connected)
  return wifi_on;
}


// Clk (clock/time) ========================================================================
// Maintains local time, using NTP servers plus timezone settings

int clk_round;

void clk_init() {
  // Default NTP poll time is SNTP_UPDATE_DELAY (sntp.c, value 3600000 ms or 1 hour).
  // Change with sntp_set_update_delay(ms)

  // Setup the servers
  configTime( cfg.getval("Timezone"), cfg.getval("NTP.server.1"), cfg.getval("NTP.server.2"), cfg.getval("NTP.server.3"));
  Serial.printf("clk : init: %s %s %s\n", cfg.getval("NTP.server.1"), cfg.getval("NTP.server.2"), cfg.getval("NTP.server.3"));
  Serial.printf("clk : timezones: %s\n", cfg.getval("Timezone") );

  // Setup round
  clk_round= String(cfg.getval("Round")).toInt();
  Serial.printf("clk : round: %d sec\n", clk_round );
}

// Out parameters (hour,min,sec) have current time (rounded up).
// Return value indicates if time is already available.
// The `buf` out parameter (caller allocated >32) has unrounded complete time
bool clk_get(int*hour, int*min, int*sec, char*buf ) {
  // Get the current time
  time_t tnow= time(NULL); // Returns seconds - note `time_t` is just a `long`.
  
  // In `snow` the `tm_year` field is 1900 based, `tm_month` is 0 based, rest is as expected
  struct tm * snow= localtime(&tnow); // Returns a struct with time fields (https://www.tutorialspoint.com/c_standard_library/c_function_localtime.htm)
  bool avail= snow->tm_year + 1900 >= 2020; // If time is in the past, treat it as not yet set

  // Debug prints of current time (NTP with timezone and DST)
  if( buf ) sprintf(buf,"%d-%02d-%02d %02d:%02d:%02d (dst=%d)", snow->tm_year + 1900, snow->tm_mon + 1, snow->tm_mday, snow->tm_hour, snow->tm_min, snow->tm_sec, snow->tm_isdst );

  // Now, round up, as requested by user
  tnow+= clk_round;
  snow= localtime(&tnow); // Returns a struct with time fields (https://www.tutorialspoint.com/c_standard_library/c_function_localtime.htm)

  // Get time components
  if( hour ) *hour= snow->tm_hour;
  if( min  ) *min = snow->tm_min;
  if( sec  ) *sec = snow->tm_sec;

  // Return availability
  return avail;
}


// NEO (NeoPixel string) =========================================================================
// Driver for the NeoPixel board

#define NEO_DIN_PIN        D6
#define NEO_NUMPIXELS      64

// When we setup the NeoPixel library, we tell it how many pixels, which NEO_DIN_PIN to use to send signals, and some pixel type settings.
Adafruit_NeoPixel neo1 = Adafruit_NeoPixel(NEO_NUMPIXELS, NEO_DIN_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel neo2 = Adafruit_NeoPixel(NEO_NUMPIXELS, 0, NEO_GRB + NEO_KHZ800); // secondary buffer for animations

void neo_test(Adafruit_NeoPixel*neo) {
  for( int color=0x110000; color>0; color>>=8 ) {
    neo->clear();
    for( int lix=NEO_NUMPIXELS-1; lix>=0; lix-- ) {
      neo->setPixelColor(lix,color);
      neo->show();  
      delay(15);
    }
    delay(150);
  }
}

// A routine for quick off during startup
void neo_off() {
  neo1.begin();
  neo1.clear();
  neo1.show();
}

// Begin with blank display
void neo_init() {
  neo1.begin();
  Serial.printf("neo : init\n");
  neo_test(&neo1);
  neo1.clear();
  neo1.show();
  Serial.printf("neo : tested\n");
  // setup secondary
  neo2.begin();
}


// LTRS (letters) =====================================================================
// Mapping words (letters) to NeoPixels (indices)

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
const int ltrs_min_vijf[]    = {55,54,53,-1};
const int ltrs_min_kwart[]   = {63,62,61,60,59,-1};
const int ltrs_min_tien[]    = {59,58,57,56,-1};
const int ltrs_min_voor[]    = {51,50,49,48,-1};
const int ltrs_min_over[]    = {47,46,45,44,-1};
const int ltrs_min_half[]    = {43,42,41,40,-1};
const int ltrs_hour_elf[]    = {39,38,37,-1};
const int ltrs_hour_drie[]   = {35,34,33,32,-1};
const int ltrs_hour_zeven[]  = {36,27,18,9,0,-1};
const int ltrs_hour_tien[]   = {31,30,29,28,-1};
const int ltrs_hour_negen[]  = {28,27,26,25,24,-1};
const int ltrs_hour_vier[]   = {23,22,21,20,-1}; 
const int ltrs_hour_vijf[]   = {18,17,16,-1}; 
const int ltrs_hour_zes[]    = {19,10,1,-1};
const int ltrs_hour_acht[]   = {15,14,13,12,-1};
const int ltrs_hour_twee[]   = {12,11,10,9,-1};
const int ltrs_hour_een[]    = {10,9,8,-1};
const int ltrs_hour_twaalf[] = {7,6,5,4,3,2,-1};

// Array of the hours (note that indices 0 and 13 are filled, for 0:15 respectively 12:45)
const int*ltrs_hours[] = {ltrs_hour_twaalf,ltrs_hour_een,ltrs_hour_twee,ltrs_hour_drie,ltrs_hour_vier,ltrs_hour_vijf,ltrs_hour_zes,ltrs_hour_zeven,ltrs_hour_acht,ltrs_hour_negen,ltrs_hour_tien,ltrs_hour_elf,ltrs_hour_twaalf,ltrs_hour_een};

// Helpers function to print the current time in words over Serial
void ltrs_print( const int word[] ) {
  if( word==ltrs_min_vijf   ) Serial.print("vijf");
  if( word==ltrs_min_kwart  ) Serial.print("kwart");
  if( word==ltrs_min_tien   ) Serial.print("tien");
  if( word==ltrs_min_voor   ) Serial.print("voor");
  if( word==ltrs_min_over   ) Serial.print("over");
  if( word==ltrs_min_half   ) Serial.print("half");
  if( word==ltrs_hour_elf   ) Serial.print("elf");
  if( word==ltrs_hour_drie  ) Serial.print("drie");
  if( word==ltrs_hour_zeven ) Serial.print("zeven");
  if( word==ltrs_hour_tien  ) Serial.print("tien");
  if( word==ltrs_hour_negen ) Serial.print("negen");
  if( word==ltrs_hour_vier  ) Serial.print("vier");
  if( word==ltrs_hour_vijf  ) Serial.print("vijf");
  if( word==ltrs_hour_zes   ) Serial.print("zes");
  if( word==ltrs_hour_acht  ) Serial.print("acht");
  if( word==ltrs_hour_twee  ) Serial.print("twee");
  if( word==ltrs_hour_een   ) Serial.print("een");
  if( word==ltrs_hour_twaalf) Serial.print("twaalf");
}

// Sets all NeoPixels in `word` to color `color`.
void ltrs_add_word(Adafruit_NeoPixel * neo, uint32_t color, const int word[] ) {
  ltrs_print(word);
  Serial.printf("/%06X ",color);
  const int * letter= word; // Point to the first NeoPixel in `word`
  while( *letter>=0 ) {
    neo->setPixelColor(*letter,color);
    letter++; // Point to the next NeoPixel in `word`
  }
}

// Sets the NeoPixels for the time specified by `hour` and `min` to "on" (in the color from the palette).
// E.g. 7:35 -> vijf over half acht
void ltrs_add_time(Adafruit_NeoPixel * neo, palette4_t * palette, int hour, int min ) {
  switch( min ) {
    case 0: case 1: case 2: case 3: case 4: 
      ltrs_add_word(neo,palette->h,ltrs_hours[hour]);
      break;  
    case 5: case 6: case 7: case 8: case 9: 
      ltrs_add_word(neo,palette->m1,ltrs_min_vijf); ltrs_add_word(neo,palette->p,ltrs_min_over); ltrs_add_word(neo,palette->h,ltrs_hours[hour]);
      break;  
    case 10: case 11: case 12: case 13: case 14: 
      ltrs_add_word(neo,palette->m1,ltrs_min_tien); ltrs_add_word(neo,palette->p,ltrs_min_over); ltrs_add_word(neo,palette->h,ltrs_hours[hour]);
      break;  
    case 15: case 16: case 17: case 18: case 19: 
      ltrs_add_word(neo,palette->m1,ltrs_min_kwart); ltrs_add_word(neo,palette->p,ltrs_min_over); ltrs_add_word(neo,palette->h,ltrs_hours[hour]);
      break;  
    case 20: case 21: case 22: case 23: case 24: 
      ltrs_add_word(neo,palette->m1,ltrs_min_tien); ltrs_add_word(neo,palette->p,ltrs_min_voor); ltrs_add_word(neo,palette->m2,ltrs_min_half); ltrs_add_word(neo,palette->h,ltrs_hours[hour+1]);
      break;  
    case 25: case 26: case 27: case 28: case 29: 
      ltrs_add_word(neo,palette->m1,ltrs_min_vijf); ltrs_add_word(neo,palette->p,ltrs_min_voor); ltrs_add_word(neo,palette->m2,ltrs_min_half); ltrs_add_word(neo,palette->h,ltrs_hours[hour+1]);
      break;  
    case 30: case 31: case 32: case 33: case 34: 
      ltrs_add_word(neo,palette->m2,ltrs_min_half); ltrs_add_word(neo,palette->h,ltrs_hours[hour+1]);
      break;  
    case 35: case 36: case 37: case 38: case 39: 
      ltrs_add_word(neo,palette->m1,ltrs_min_vijf); ltrs_add_word(neo,palette->p,ltrs_min_over); ltrs_add_word(neo,palette->m2,ltrs_min_half); ltrs_add_word(neo,palette->h,ltrs_hours[hour+1]);
      break;  
    case 40: case 41: case 42: case 43: case 44: 
      ltrs_add_word(neo,palette->m1,ltrs_min_tien); ltrs_add_word(neo,palette->p,ltrs_min_over); ltrs_add_word(neo,palette->m2,ltrs_min_half); ltrs_add_word(neo,palette->h,ltrs_hours[hour+1]);
      break;  
    case 45: case 46: case 47: case 48: case 49: 
      ltrs_add_word(neo,palette->m1,ltrs_min_kwart); ltrs_add_word(neo,palette->p,ltrs_min_voor); ltrs_add_word(neo,palette->h,ltrs_hours[hour+1]);
      break;  
    case 50: case 51: case 52: case 53: case 54: 
      ltrs_add_word(neo,palette->m1,ltrs_min_tien); ltrs_add_word(neo,palette->p,ltrs_min_voor); ltrs_add_word(neo,palette->h,ltrs_hours[hour+1]);
      break;  
    case 55: case 56: case 57: case 58: case 59:
      ltrs_add_word(neo,palette->m1,ltrs_min_vijf); ltrs_add_word(neo,palette->p,ltrs_min_voor); ltrs_add_word(neo,palette->h,ltrs_hours[hour+1]);
      break;  
  }
}

void ltrs_init() {
  // Nothing to do yet
  Serial.printf("ltrs: init\n");  
}


// ANIM (animators) ===========================================================

// Wipe

class AnimWipe {
  public:
    AnimWipe();
    void start(palette4_t * p, int h, int m);
    void step();
  private:
    int _step;
    uint32_t _ms;
};

AnimWipe::AnimWipe() {
  _step=9;
}

void AnimWipe::start(palette4_t * p, int h, int m) {
  // Prepare the new image
  neo2.clear();
  ltrs_add_time(&neo2,p,h%12,m); // Remember to print PM hours to 0..11
  // Init animation state
  _step=0; 
  _ms=millis(); 
};

void AnimWipe::step() {
  uint32_t ms= millis();
  if( _step<9 && ms-_ms>50 ) {
    for( int lix=8*(8-_step)-1; lix>=8*(8-_step)-8; lix--) {
      if( lix+8<NEO_NUMPIXELS ) neo1.setPixelColor(lix+8,neo2.getPixelColor(lix+8));
      if( lix+8<NEO_NUMPIXELS ) neo1.setPixelColor(lix,col_palette.c5);
    }
    neo1.show();
    _step++;
    _ms= ms;
  }
}

AnimWipe animx;

// Dots

class AnimDots {
  public:
    AnimDots();
    void start(palette4_t * p, int h, int m);
    void step();
  private:
    int _phase; // 1=del-dots, 2=add-dots
    int _left;  // dots left to del or add
    uint32_t _ms;
};

AnimDots::AnimDots() {
  _phase=2;
  _left=0;
}

void AnimDots::start(palette4_t * p, int h, int m) {
  // Prepare the new image
  neo2.clear();
  ltrs_add_time(&neo2,p,h%12,m); // Remember to print PM hours to 0..11
  // Init animation state
  _phase=1; // start by deleting current pixels
  _left=0; for(int lix=0; lix<NEO_NUMPIXELS; lix++ ) _left+= neo1.getPixelColor(lix)!=0;
  _ms=millis(); 
};

void AnimDots::step() {
  // Phase done?
  if( _left==0 ) {
    if( _phase==1 ) {
      // Switch to adding pixels
      _left=0; for(int lix=0; lix<NEO_NUMPIXELS; lix++ ) _left+= neo2.getPixelColor(lix)!=0;  
      _phase= 2;
    } else {
      // End of phase 2, so do nothing
      return;
    }
  }
  // Timing
  //Serial.printf("animdots: %d %d\n",_phase,_left);
  uint32_t ms= millis();
  if( ms-_ms<100 ) return;
  // Execute a step  
  if( _phase==1 ) {
    // deleting
    int lix0= random(NEO_NUMPIXELS);
    int lix= lix0;
    do {
      if( neo1.getPixelColor(lix)!=0 ) { neo1.setPixelColor(lix,0); break; }
      lix= (lix+1) % NEO_NUMPIXELS;
    } while( lix!=lix0 );
  } else {
    // adding
    int lix0= random(NEO_NUMPIXELS);
    int lix= lix0;
    do {
      if( neo2.getPixelColor(lix)!=0 ) { neo1.setPixelColor(lix,neo2.getPixelColor(lix)); neo2.setPixelColor(lix,0); break; }
      lix= (lix+1) % NEO_NUMPIXELS;
    } while( lix!=lix0 );
  }
  neo1.show();
  _left--;
  _ms= ms;
}

AnimDots anim;


// MAIN ========================================================================
// The main functions

void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nmain: WordClockFull v%s\n\n",VERSION);
  neo_off(); // Switch pixels off (even when going to config mode)

  // On boot: check if config button is pressed
  delay(1000); // We need a bit of delay (due to the enormous capacitor?) to detect the button
  cfg.check(100,CFG_BUT_PIN); // Wait 100 flashes (of 50ms) for a change on pin CFG_BUT_PIN
  // if in config mode, do config setup (when config completes, it restarts the device)
  if( cfg.cfgmode() ) { cfg.setup(); return; }
  Serial.printf("main: No configuration requested, started WordClock\n\n");

  neo_init(); 
  but_init();
  led_init();
  wifi_init();
  clk_init();
  ltrs_init();
  col_init();

  Serial.printf("\n");
  led_on(); // Show the app is running, and local time not yet synced with NTP
}

int prevhour,prevmin,prevsec;
char buf[32];
void loop() {
  // if in config mode, do config loop (when config completes, it restarts the device)
  if( cfg.cfgmode() ) { cfg.loop(); return; }

  // In normal application mode
  wifi_check();
  but_scan();

  // Get the current time
  int curhour,curmin,cursec, avail= clk_get(&curhour,&curmin,&cursec,buf);
  if( curhour!=prevhour || curmin!=prevmin || cursec!=prevsec || but_wentdown() ) {
    // Time has changed, print to serial
    Serial.printf("clk: %s ", buf); 
    if( avail ) {
      Serial.printf("%02d:%02d ", curhour, curmin); 
      int refresh= but_wentdown();
      if( refresh ) Serial.printf("force "); 
      refresh|= curhour!=prevhour;
      if( col_refresh==COL_REFRESH_ONE  ) refresh|= curmin!=prevmin;
      if( col_refresh==COL_REFRESH_FIVE ) refresh|= (curmin%5==0) && (curmin!=prevmin);
      if( refresh ) {
        palette4_t * palette= col_next();
        anim.start(palette,curhour%12,curmin);
      }
    } else {
      Serial.printf("[no NTP sync yet]"); 
    }
    Serial.printf("\n"); 
    prevhour=curhour; prevmin=curmin; prevsec=cursec;
  }

  // Run any pending animation
  anim.step();
}
