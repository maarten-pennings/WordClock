// TimeKeeping.ino - Keep track of time via NTP (local time zone and daylight saving time included)

#include <coredecls.h> // Only needed for settimeofday_cb()
#include <ESP8266WiFi.h>
#include <time.h>

#define SSID "GuestFamPennings"
#define PASS "there_is_no_password"

#define SVR1 "pool.ntp.org"
#define SVR2 "europe.pool.ntp.org"
#define SVR3 "north-america.pool.ntp.org"

#define TZ   "CET-1CEST,M3.5.0,M10.5.0/3" // Amsterdam
//#define TZ "EST+5EDT,M3.2.0/2,M11.1.0/2" // New York

// TZ specification, see https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
// TZ ::= <stdzone><dstzone>,<dststart>,<dstend>
// where
//   <stdzone>   ::= <stdname><stdoffset>     // Specifies the standard (non daylight saving) timezone (name,offset)
//   <stdname>   ::= <identifier>             // Three or more characters long name of the zone (I think it is not used)
//   <stdoffset> ::= [+|-]hh[:mm[:ss]]        // The offset to get a Universal Time Coordinated (so reverse sign)
//   <dstzone>   ::= <dstname>[<dstoffset>]   // Specifies the daylight saving timezone (name,offset)
//   <dstname>   ::= <identifier>             // Three or more characters long name of the zone (I think it is not used)
//   <dstoffset> ::= [+|-]hh[:mm[:ss]]        // The offset, defaults to 1 hour when omitted 
//   <dststart>  ::= <dayinyear> [ / <time> ] 
//   <dstend>    ::= <dayinyear> [ / <time> ]
//   <dayinyear> ::= J <num>                  // day, with <num> between 1 and 365. February 29 is never counted, even in leap years
//                 | <num>                    // day, with <num> between 0 and 365. February 29 is counted in leap years
//                 | M<m-num>.<w-num>.<d-num> // day <d-num> of week <w-num> of month <m-num>. Day <d-num> is 0(Sun)..6(Sat). Week <w-num> is 1..5. Month <m-num> 1..12.
//   <time>      ::= hh[:mm[:ss]]             // when, in the local time, the change to dst occurs. If omitted, the default is 02:00:00. 

void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nTimeKeeping - NTP with TZ and DST\n\n");

  configTime(TZ, SVR1, SVR2, SVR3);

  // Default NTP poll time is SNTP_UPDATE_DELAY (sntp.c, value 3600000 ms or 1 hour).
  // Change with sntp_set_update_delay(ms)

  // Track when time is actually set
  settimeofday_cb( [](){Serial.printf("SET\n");} );  // Pass lambda function to print SET when time is set
  
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASS);
}

int demo_loopcount = 0;

void loop() {
  time_t      tnow= time(NULL);       // Returns seconds (and writes to the passed pointer, when not NULL) - note `time_t` is just a `long`.
  struct tm * snow= localtime(&tnow); // Returns a struct with time fields (https://www.tutorialspoint.com/c_standard_library/c_function_localtime.htm)

  // In `snow` the `tm_year` field is 1900 based, `tm_month` is 0 based, rest is as expected
  if( snow->tm_year + 1900 < 2020 ) Serial.printf("Not yet synced - "); // We miss-uses old time as indication of " time not yet set"
  Serial.printf("%d-%02d-%02d %02d:%02d:%02d (dst=%d)\n", snow->tm_year + 1900, snow->tm_mon + 1, snow->tm_mday, snow->tm_hour, snow->tm_min, snow->tm_sec, snow->tm_isdst );

  // Demo: Switch WiFi off after 15 loop iterations, and note time continues
  if( demo_loopcount==15 ) {
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    Serial.println("DEMO >>> WiFi off: time continues");
  }

  // Demo: Reset internal clock after 20 loop iterations, and note time starts from reset val
  if( demo_loopcount==20 ) {
    // Example of how to set the time to some known value
    struct tm sday; // time as a struct (fields year..sec)
    sday.tm_year =  2000 - 1900; // fill all fields (year and month have non standard offsets)
    sday.tm_mon = 11 - 1;
    sday.tm_mday = 22;
    sday.tm_hour = 11; 
    sday.tm_min = 22;
    sday.tm_sec = 33;
    time_t tday= mktime(&sday); // time as seconds since 1900
    if( tday==(time_t)-1 ) Serial.printf("error\n");
    timeval vday = { tday, 0 }; // time as a pair of time in seconds and usec (set to 0 here)
    struct timezone tz = {0,0}; // zone offset and DST offset
    settimeofday(&vday, &tz );  // If I pass NULL for tz (https://linux.die.net/man/2/settimeofday), my hours are DST mangled
    Serial.println("DEMO >>> Clock reset: time continues from reset val");
  }

  // Demo: Enable WiFi after 25 loop iterations, and note time syncs again
  if( demo_loopcount==25 ) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASS);
    Serial.println("DEMO >>> WiFi on: time syncs again");
  }

  demo_loopcount++;
  delay(1000);
}

/*
time-keeping - NTP with TZ and DST

Not yet synced - 1970-01-01 09:00:00 (dst=0)
Not yet synced - 1970-01-01 09:00:01 (dst=0)
Not yet synced - 1970-01-01 09:00:02 (dst=0)
Not yet synced - 1970-01-01 09:00:03 (dst=0)
SET
2020-02-17 12:43:48 (dst=0)
2020-02-17 12:43:49 (dst=0)
2020-02-17 12:43:50 (dst=0)
2020-02-17 12:43:51 (dst=0)
2020-02-17 12:43:52 (dst=0)
2020-02-17 12:43:53 (dst=0)
2020-02-17 12:43:54 (dst=0)
2020-02-17 12:43:55 (dst=0)
2020-02-17 12:43:56 (dst=0)
2020-02-17 12:43:57 (dst=0)
2020-02-17 12:43:58 (dst=0)
2020-02-17 12:43:59 (dst=0)
DEMO >>> WiFi off: time continues
2020-02-17 12:44:00 (dst=0)
2020-02-17 12:44:01 (dst=0)
2020-02-17 12:44:02 (dst=0)
2020-02-17 12:44:03 (dst=0)
2020-02-17 12:44:04 (dst=0)
DEMO >>> Clock reset: time continues from reset val
SET
Not yet synced - 2000-11-22 11:22:34 (dst=0)
Not yet synced - 2000-11-22 11:22:35 (dst=0)
Not yet synced - 2000-11-22 11:22:36 (dst=0)
Not yet synced - 2000-11-22 11:22:37 (dst=0)
Not yet synced - 2000-11-22 11:22:38 (dst=0)
DEMO >>> WiFo on: time syncs again
Not yet synced - 2000-11-22 11:22:39 (dst=0)
Not yet synced - 2000-11-22 11:22:40 (dst=0)
Not yet synced - 2000-11-22 11:22:41 (dst=0)
SET
2020-02-17 12:44:14 (dst=0)
2020-02-17 12:44:15 (dst=0)
2020-02-17 12:44:16 (dst=0)
2020-02-17 12:44:17 (dst=0)
... many lines deleted
2020-02-17 13:44:10 (dst=0)
2020-02-17 13:44:11 (dst=0)
2020-02-17 13:44:12 (dst=0)
2020-02-17 13:44:13 (dst=0)
SET
2020-02-17 13:44:15 (dst=0)
2020-02-17 13:44:16 (dst=0)
2020-02-17 13:44:17 (dst=0)
2020-02-17 13:44:18 (dst=0)
... many lines deleted
2020-02-17 14:44:11 (dst=0)
2020-02-17 14:44:12 (dst=0)
2020-02-17 14:44:13 (dst=0)
2020-02-17 14:44:14 (dst=0)
SET
2020-02-17 14:44:15 (dst=0)
2020-02-17 14:44:16 (dst=0)
2020-02-17 14:44:17 (dst=0)
2020-02-17 14:44:18 (dst=0)
... many lines deleted
2020-02-17 15:44:11 (dst=0)
2020-02-17 15:44:12 (dst=0)
2020-02-17 15:44:13 (dst=0)
2020-02-17 15:44:14 (dst=0)
SET
2020-02-17 15:44:15 (dst=0)
2020-02-17 15:44:16 (dst=0)
2020-02-17 15:44:17 (dst=0)
2020-02-17 15:44:18 (dst=0)
...
*/
