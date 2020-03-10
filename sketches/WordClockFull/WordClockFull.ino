// WordClockFull.ino - A wordClock using NeoPixels, keeping time via NTP, with dynamic configuration, color
#define VERSION "4"

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
  {"Color.1"         , "120000"                     ,  6, "The first color; by default used for hour category (een-twaalf)." },
  {"Color.2"         , "001200"                     ,  6, "The second color; by default used for minute 1 category (vijf, tien, kwart)." },
  {"Color.3"         , "0C0C00"                     ,  6, "The third color; by default used for minute 2 category (half). If colors 2 and 3 are equal, 'half' belongs to 'minutes'." },
  {"Color.4"         , "000018"                     ,  6, "The fourth color; by default used for prepositions category (voor, over)." },
  {"Color.5"         , "222222"                     ,  6, "The fifth color; for some animations. " },
  
  {"Display"         , ""                           ,  0, "Which colors are used when. " },
  {"Refresh"         , "one"                        ,  8, "When is the display refreshed: <b>one</b> (every minute - useless for <b>fix</b>/<b>none</b>), <b>five</b> (every 5 minutes)." },
  {"Mapping"         , "cycle"                      ,  8, "Mapping: <b>fix</b> (fixed to default), <b>cycle</b> (colors cycle over categories), <b>random</b> (random, max from colors 1-4)." },  
  {"Animation"       , "wipe"                       ,  8, "The animation: <b>none</b>, <b>wipe</b> (wipe, using Color.5), <b>dots</b> (letter by letter off then on), <b>pulse</b> (dim down then up). " },  
  {0                 , 0                            ,  0, 0},  
};

#define CFG_BUT_PIN 0

Cfg cfg("WordClock", cfg_fields, CFG_SERIALLVL_USR, LED_BUILTIN);


// COL (Color) ================================================================================
// Color management (palettes)

typedef struct { // Colors for the words
  uint32_t h;  // Color for hours
  uint32_t m1; // Color for minutes (vijf, tien, kwart)
  uint32_t m2; // Color for minutes (half)
  uint32_t p;  // Color for prepositions (voor, over)
} palette4_t;

typedef struct { // Color component maxima for random
  uint32_t r; // The maximum r (red) value in palette5
  uint32_t g; // The maximum g (green) value in palette5
  uint32_t b; // The maximum b (blue) value in palette5
} palette3_t;

typedef struct { // Colors from config
  uint32_t c1; 
  uint32_t c2;
  uint32_t c3;
  uint32_t c4;
  uint32_t c5;
} palette5_t;


// Values retrieved from Cfg

palette5_t col_palette; 
palette3_t col_maxima;  // Maxima from col_palette - see col_init()

#define COL_REFRESH_ONE     1 // cfg string: one
#define COL_REFRESH_FIVE    5 // cfg string: five
int col_refresh;   

#define COL_MAPPING_FIX     1 // cfg string: fix
#define COL_MAPPING_CYCLE   2 // cfg string: cycle
#define COL_MAPPING_RANDOM  3 // cfg string: random
int col_mapping;   

#define COL_ANIMATION_NONE  1 // cfg string: none
#define COL_ANIMATION_WIPE  2 // cfg string: wipe
#define COL_ANIMATION_DOTS  3 // cfg string: dots
#define COL_ANIMATION_PULSE 4 // cfg string: pulse
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
  if(      val==COL_ANIMATION_NONE  ) return "none";
  else if( val==COL_ANIMATION_WIPE  ) return "wipe";
  else if( val==COL_ANIMATION_DOTS  ) return "dots";
  else if( val==COL_ANIMATION_PULSE ) return "pulse";
  else return "?animation?";
}

int col_animation_parse() {
  const char * cfgid= "Animation";
  char * val= cfg.getval(cfgid);
  if( strcmp(val,"none"    )==0 ) return COL_ANIMATION_NONE;
  if( strcmp(val,"wipe"    )==0 ) return COL_ANIMATION_WIPE;
  if( strcmp(val,"dots"    )==0 ) return COL_ANIMATION_DOTS;
  if( strcmp(val,"pulse"   )==0 ) return COL_ANIMATION_PULSE;
  int dft= COL_ANIMATION_WIPE;
  Serial.printf("ltrs: ERROR in %s (%s->%s)\n",cfgid,val,col_animation_unparse(dft));
  return dft;
}

#define R(u)       (((u)>>16)&0xFF)
#define G(u)       (((u)>> 8)&0xFF)
#define B(u)       (((u)>> 0)&0xFF)
#define RGB(r,g,b) ((r)<<16) | ((g)<<8) | ((b)<<0)

void col_init() {
  col_palette.c1= col_parse( "Color.1" );
  col_palette.c2= col_parse( "Color.2" );
  col_palette.c3= col_parse( "Color.3" );
  col_palette.c4= col_parse( "Color.4" );
  col_palette.c5= col_parse( "Color.5" );
  Serial.printf("col : palette: %06X %06X %06X %06X %06X\n", col_palette.c1, col_palette.c2, col_palette.c3, col_palette.c4, col_palette.c5 );

  col_maxima.r= max( max( R(col_palette.c1) , R(col_palette.c2) )  ,  max( R(col_palette.c3) , R(col_palette.c4) ) );
  col_maxima.g= max( max( G(col_palette.c1) , G(col_palette.c2) )  ,  max( G(col_palette.c3) , G(col_palette.c4) ) );
  col_maxima.b= max( max( B(col_palette.c1) , B(col_palette.c2) )  ,  max( B(col_palette.c3) , B(col_palette.c4) ) );
  Serial.printf("col : max: R%02X G%02X B%02X\n", col_maxima.r, col_maxima.g, col_maxima.b );

  col_refresh= col_refresh_parse();
  col_mapping= col_mapping_parse();
  col_animation= col_animation_parse();
  Serial.printf("col : modes: %s %s %s\n", col_refresh_unparse(col_refresh), col_mapping_unparse(col_mapping), col_animation_unparse(col_animation) );
}

// Generating the right color

uint32_t col_random() {
  // If we do truly random, colors get a bit washed out and equal, or even black. Introduce bias to hi.
  // We generate a 3 bit vector telling which of the (r,g,b) components are included
  int included= random(1,8); // Range [1..7], in binary (000,111], so black is not generated.
  uint32_t r= 0; // off
  if( included & 1 ) {
    r= col_maxima.r; // max
    if( random(4)==3 ) r= random(r/5)*5+5; // In steps of 5, minimum 5
  }
  uint32_t g= 0; // off
  if( included & 2 ) {
    g= col_maxima.g; // max
    if( random(4)==3 ) g= random(g/5)*5+5; // In steps of 5, minimum 5
  }
  uint32_t b= 0; // off
  if( included & 4 ) {
    b= col_maxima.b; // max
    if( random(4)==3 ) b= random(b/5)*5+5; // In steps of 5, minimum 5
  }
  // Compose
  uint32_t col= RGB(r,g,b);
  return col;
}

palette4_t * col_next() {
  static palette4_t palette;
  static int first=1;

  switch( col_mapping ) {
    
    case COL_MAPPING_FIX    :
        palette.h = col_palette.c1;
        palette.m1= col_palette.c2;
        palette.m2= col_palette.c3;
        palette.p = col_palette.c4;
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
      // Generate distinct colors
      palette.h = col_random();
      do { palette.m1= col_random(); } while( palette.h==palette.m1 );
      do { palette.m2= col_random(); } while( palette.h==palette.m2 || palette.m1==palette.m2 );
      do { palette.p = col_random(); } while( palette.h==palette.p  || palette.m1==palette.p  || palette.m2==palette.p );
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

#define  CLK_DEMO_MS   5000 // In demo mode, every CLK_DEMO_MS advance 1 minute
int      clk_demo_mode;
int      clk_demo_hour;
int      clk_demo_min;
uint32_t clk_demo_ms;

// This function is the equivalent of clk_get(): it generates time in demo mode (much faster)
bool clk_demo(int*hour, int*min, int*sec, char*buf ) {
  uint32_t now= millis();
  if( now-clk_demo_ms>CLK_DEMO_MS ) {
    clk_demo_min++;
    if( clk_demo_min==60 ) {
      clk_demo_min=0;
      clk_demo_hour++;
      if( clk_demo_hour==24 ) clk_demo_hour=0;
    }
    clk_demo_ms= now;
  }
  if( buf ) sprintf(buf,"(demo) %02d:%02d:%02d", clk_demo_hour, clk_demo_min, 0 );
  if( hour ) *hour= clk_demo_hour;
  if( min  ) *min = clk_demo_min;
  if( sec  ) *sec = 0;

  return true;
}

int clk_round;

void clk_init() {
  // Default NTP poll time is SNTP_UPDATE_DELAY (sntp.c, value 3600000 ms or 1 hour).
  // Change with sntp_set_update_delay(ms)

  // Setup the servers
  configTime( cfg.getval("Timezone"), cfg.getval("NTP.server.1"), cfg.getval("NTP.server.2"), cfg.getval("NTP.server.3"));
  Serial.printf("clk : init: %s %s %s\n", cfg.getval("NTP.server.1"), cfg.getval("NTP.server.2"), cfg.getval("NTP.server.3"));
  Serial.printf("clk : timezones: %s\n", cfg.getval("Timezone") );

  // Disable demo
  clk_demo_mode= 0;
  
  // Setup round
  clk_round= String(cfg.getval("Round")).toInt();
  Serial.printf("clk : round: %d sec\n", clk_round );
}

// Out parameters (hour,min,sec) have current time (rounded up).
// Return value indicates if time is already available.
// The `buf` out parameter (caller allocated >32) has unrounded complete time
bool clk_get(int*hour, int*min, int*sec, char*buf ) {
  // If in demo mode, return demo ("fast") time
  if( clk_demo_mode ) return clk_demo(hour,min,sec,buf);
  
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

// Get the status of the demo mode
int clk_demo_get() {
  return clk_demo_mode;
}

// Set the status of the demo mode
void clk_demo_set(int demo) {
  if( demo ) {
    clk_demo_mode= 0; // So the clk_get() works
    clk_get(&clk_demo_hour, &clk_demo_min, NULL, NULL);
    clk_demo_ms= millis();
    clk_demo_ms+= CLK_DEMO_MS; // trigger an update immediately
    Serial.printf("clk : demo: on %02d:%02d\n",clk_demo_hour, clk_demo_min);
  } else {
    Serial.printf("clk : demo: off\n");
  }
  clk_demo_mode= demo;
}


// NEO (NeoPixel string) =========================================================================
// Driver for the NeoPixel board

#define NEO_DIN_PIN        D6
#define NEO_NUMPIXELS      64

// When we setup the NeoPixel library, we tell it how many pixels, which NEO_DIN_PIN to use to send signals, and some pixel type settings.
Adafruit_NeoPixel neopixel = Adafruit_NeoPixel(NEO_NUMPIXELS, NEO_DIN_PIN, NEO_GRB + NEO_KHZ800);

void neo_test(Adafruit_NeoPixel*neo) {
  for( int color=0x110000; color>0; color>>=8 ) {
    neo->clear();
    for( int lix=NEO_NUMPIXELS-1; lix>=0; lix-- ) {
      neo->setPixelColor(lix,color);
      neo->show();  
      delay(10);
    }
    delay(100);
  }
}

// A routine for quick off during startup
void neo_off() {
  neopixel.begin();
  neopixel.clear();
  neopixel.show();
}

// Begin with blank display
void neo_init() {
  neopixel.begin();
  Serial.printf("neo : init\n");
  neo_test(&neopixel);
  neopixel.clear();
  neopixel.show();
  Serial.printf("neo : tested\n");
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


// None

class AnimNone {
  public:
    AnimNone();
    void start(palette4_t * p, int h, int m);
    void step();
    int _run;
};

AnimNone::AnimNone() {
  _run= 0;
}

void AnimNone::start(palette4_t * p, int h, int m) {
  Serial.printf("(none) ");
  neopixel.clear();
  ltrs_add_time(&neopixel,p,h%12,m); // Remember to print PM hours to 0..11
  neopixel.show();
  _run= 1;
};

void AnimNone::step() {
  if( _run ) {
    Serial.printf("anim: none - stop\n");
    _run= 0;
  }
}

AnimNone animn;

// Wipe

class AnimWipe {
  public:
    AnimWipe();
    void start(palette4_t * p, int h, int m);
    void step();
  private:
    Adafruit_NeoPixel * _neobuf;
    int _step;
    uint32_t _ms;
};

AnimWipe::AnimWipe() {
  _neobuf = new Adafruit_NeoPixel(NEO_NUMPIXELS, -1, NEO_GRB + NEO_KHZ800);
  _neobuf->begin();
  _step=9;
}

void AnimWipe::start(palette4_t * p, int h, int m) {
  Serial.printf("(wipe) ");
  // Prepare the new image
  _neobuf->clear();
  ltrs_add_time(_neobuf,p,h%12,m); // Remember to print PM hours to 0..11
  // Init animation state
  _step=0; 
  _ms=millis(); 
};

void AnimWipe::step() {
  uint32_t ms= millis();
  if( ms-_ms<200 ) return;
  _ms= ms; 

  if( _step<9  ) {
    for( int lix=7-_step; lix<NEO_NUMPIXELS; lix+=8) {
      if( _step>0 ) neopixel.setPixelColor(lix+1,_neobuf->getPixelColor(lix+1));
      if( _step<8 ) neopixel.setPixelColor(lix,col_palette.c5);
    }
    neopixel.show();
    _step++;

    if( _step==9 ) Serial.printf("anim: wipe - stop\n");
  }
}

AnimWipe animw;

// Dots

class AnimDots {
  public:
    AnimDots();
    void start(palette4_t * p, int h, int m);
    void step();
  private:
    Adafruit_NeoPixel * _neobuf;
    int _phase; // 0=idle, 1=del-dots, 2=add-dots
    uint32_t _ms;
};

AnimDots::AnimDots() {
  _neobuf = new Adafruit_NeoPixel(NEO_NUMPIXELS, -1, NEO_GRB + NEO_KHZ800);
  _neobuf->begin();
  _phase=0;
}

void AnimDots::start(palette4_t * p, int h, int m) {
  Serial.printf("(dots) ");
  // Prepare the new image
  _neobuf->clear();
  ltrs_add_time(_neobuf,p,h%12,m); // Remember to print PM hours to 0..11
  // Init animation state
  _phase=1; // start by deleting current pixels
  _ms=millis(); 
};

void AnimDots::step() {
  // Animation idle?
  if( _phase==0 ) return;
  
  // Timing
  uint32_t ms= millis();
  if( ms-_ms<75 ) return;
  _ms= ms; 
  
  // Execute a step: delete one pixel in phase 1
  if( _phase==1 ) {
    // deleting
    int lix0= random(NEO_NUMPIXELS);
    int lix= lix0;
    do {
      if( neopixel.getPixelColor(lix)!=0 ) { neopixel.setPixelColor(lix,0); neopixel.show(); return; }
      lix= (lix+7) % NEO_NUMPIXELS; // We jump in steps of 7 -- note gcd(64,7)==1 -- to mittigate that words are eaten from one side
    } while( lix!=lix0 );
    // No pixel found to delete. So screen is empty. Go to next phase
    _phase=2;
    return;
  } 
  
  // Execute a step: add one pixel in phase 2
  if( _phase==2 ) {
    // adding
    int lix0= random(NEO_NUMPIXELS);
    int lix= lix0;
    do {
      if( _neobuf->getPixelColor(lix)!=0 ) { neopixel.setPixelColor(lix,_neobuf->getPixelColor(lix)); _neobuf->setPixelColor(lix,0); neopixel.show(); return; }
      lix= (lix+7) % NEO_NUMPIXELS; // We jump in steps of 7 -- note gcd(64,7)==1 -- to mittigate that words are extended from one side
    } while( lix!=lix0 );
    // No pixel found to delete. So screen is empty. Go to next phase
    _phase=0;
    Serial.printf("anim: dots - stop\n");
    return;
  }
}

AnimDots anim;

// Pulse

#define ANIMPULSE_MAXSTEP 25

class AnimPulse {
  public:
    AnimPulse();
    void start(palette4_t * p, int h, int m);
    void step();
  private:
    Adafruit_NeoPixel * _neobuf1;
    Adafruit_NeoPixel * _neobuf2;
    int _step; // [0..ANIMPULSE_MAXSTEP) is pulse-down,  [ANIMPULSE_MAXSTEP..2*ANIMPULSE_MAXSTEP) is pulse up
    uint32_t _ms;
};

AnimPulse::AnimPulse() {
  _neobuf1 = new Adafruit_NeoPixel(NEO_NUMPIXELS, -1, NEO_GRB + NEO_KHZ800);
  _neobuf1->begin();
  _neobuf2 = new Adafruit_NeoPixel(NEO_NUMPIXELS, -1, NEO_GRB + NEO_KHZ800);
  _neobuf2->begin();
  _step= 2*ANIMPULSE_MAXSTEP;
}

void AnimPulse::start(palette4_t * p, int h, int m) {
  Serial.printf("(pulse) ");
  // Save the old image
  _neobuf1->clear();
  for(int lix=0; lix<NEO_NUMPIXELS; lix++) _neobuf1->setPixelColor(lix,neopixel.getPixelColor(lix));
  // Prepare the new image
  _neobuf2->clear();
  ltrs_add_time(_neobuf2,p,h%12,m); // Remember to print PM hours to 0..11
  // Init animation state
  _step=0;
  _ms=millis(); 
};

void AnimPulse::step() {
  // Timing
  uint32_t ms= millis();
  if( ms-_ms<50 ) return;
  _ms=ms;
  
  // Execute a step: pulse downn all pixels a small bit 
  if( _step<ANIMPULSE_MAXSTEP ) {
    // pulse down
    for(int lix=0; lix<NEO_NUMPIXELS; lix++) {
      int f= ANIMPULSE_MAXSTEP-1-_step;
      uint32_t col= _neobuf1->getPixelColor(lix);
      uint32_t r= R(col)*f/ANIMPULSE_MAXSTEP;
      uint32_t g= G(col)*f/ANIMPULSE_MAXSTEP;
      uint32_t b= B(col)*f/ANIMPULSE_MAXSTEP;
      neopixel.setPixelColor(lix,RGB(r,g,b)); 
    }
    neopixel.show();
    _step++;
  } else if( _step<2*ANIMPULSE_MAXSTEP ) {
    // pulse up
    for(int lix=0; lix<NEO_NUMPIXELS; lix++) {
      int f= 1 + _step - ANIMPULSE_MAXSTEP;
      uint32_t col= _neobuf2->getPixelColor(lix);
      uint32_t r= R(col)*f/ANIMPULSE_MAXSTEP;
      uint32_t g= G(col)*f/ANIMPULSE_MAXSTEP;
      uint32_t b= B(col)*f/ANIMPULSE_MAXSTEP;
      neopixel.setPixelColor(lix,RGB(r,g,b)); 
    }
    neopixel.show();
    _step++;
    if( _step==2*ANIMPULSE_MAXSTEP ) Serial.printf("anim: pulse - stop\n");
  } else {
    // nothing to do
  }
  
}

AnimPulse animp;


// MAIN ========================================================================
// The main functions

void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nmain: WordClockFull v%s\n\n",VERSION);
  neo_off(); // Switch pixels off (even when going to config mode)

  // On boot: check if config button is pressed
  delay(1000); // We need a bit of delay (due to the enormous capacitor?) to detect the button
  cfg.check(60,CFG_BUT_PIN); // Wait 60 flashes (of 50ms) for a change on pin CFG_BUT_PIN
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

  // If button pressed, toggle demo mode
  if( but_wentdown() ) clk_demo_set( !clk_demo_get() );
  
  // Get the current time
  int curhour,curmin,cursec, avail= clk_get(&curhour,&curmin,&cursec,buf);
  if( curhour!=prevhour || curmin!=prevmin || cursec!=prevsec ) {
    // Time has changed, print to serial
    Serial.printf("clk : %s ", buf); 
    if( avail ) {
      Serial.printf("%02d:%02d ", curhour, curmin); 
      int refresh= curhour!=prevhour;
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
